#!/usr/bin/python3.7

# Oclvankus is a client/worker that computes chains.

#from __future__ import print_function

import pyopencl as cl
import numpy as np
import time, socket, os, sys, struct, threading

from libdeka import *

import libvankus

from vankusconf import mytables, HOST, PORT, kernels, slices

mf = cl.mem_flags

# --- Selection automatique du GPU NVIDIA ------------------------------------
# Lance par deka-start.sh (via pkexec), on n a pas de terminal interactif :
# create_some_context() poserait une question et bloquerait. On choisit donc
# nous-memes le premier GPU NVIDIA (RTX 4090 ici) et on retombe, dans l ordre,
# sur n importe quel GPU puis n importe quel device si NVIDIA manque.
# Forcable a la main via PYOPENCL_CTX (ex: PYOPENCL_CTX=0) qui court-circuite tout.
def _pick_nvidia_context():
    if os.environ.get("PYOPENCL_CTX"):
        return cl.create_some_context(interactive=False)
    gpus, nvidia = [], []
    for plat in cl.get_platforms():
        is_nv = "nvidia" in (plat.vendor + plat.name).lower()
        for dev in plat.get_devices():
            if dev.type & cl.device_type.GPU:
                gpus.append(dev)
                if is_nv or "nvidia" in (dev.vendor + dev.name).lower():
                    nvidia.append(dev)
    chosen = nvidia or gpus
    if not chosen:
        raise RuntimeError("aucun device OpenCL GPU trouve")
    dev = chosen[0]
    print("OpenCL device: %s (%s)" % (dev.name, dev.vendor))
    return cl.Context(devices=[dev])

ctx = _pick_nvidia_context()
cmdq = cl.CommandQueue(ctx)

# how many colors are there in each table
colors = 8
# length of one GSM burst
burstlen = 114
# length of one keystream sample
samplelen = 64
# samples in one burst (moving window)
samples = burstlen - samplelen + 1

# fragments in cl blob
clblobfrags = kernels * slices

# size of one fragment in longs (we need fragment + RF + challenge + flags)
onefrag = 4

# 64 ones
mask64 = 2**64-1

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

x = 0

# Reverse bits in integer
def revbits(x):
  return int(bin(x)[2:].zfill(64)[::-1], 2)


# Background thread for network communication
def network_thr():

  master_connect()

  while True:
    #print("Network!")

    n = libvankus.getnumfree()

    print("%i free slots"%n)

    while libvankus.getnumfree() > 2:
      if not get_startpoints():
        break

    while libvankus.getnumfree() > 2:
      if not get_keystream():
        break

    put_work()
    put_work()
    put_cracked()
    put_cracked()
    time.sleep(0.2)


def master_connect():
  global sock

  # Lance par deka-start, oclvankus peut demarrer avant que paplon (le master)
  # n ait ouvert son port : un connect() sec echouerait en "connection refused"
  # et tuerait le worker. On reessaie donc pendant ~30 s avant d abandonner.
  deadline = time.time() + 30
  while True:
    try:
      sock.connect((HOST, PORT))
      return
    except (ConnectionRefusedError, OSError) as e:
      if time.time() >= deadline:
        raise
      print("master %s:%s pas encore la (%s), nouvel essai..." % (HOST, PORT, e))
      sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      time.sleep(1)

# Ask our master for keystream to crack
def get_keystream():
  sock.sendall(bytes("getkeystream\r\n", "ascii"))

  l = getline(sock)

  # Master pas encore pret (reponse vide) : on n insiste pas ce tour-ci plutot
  # que de crasher le thread reseau sur un None.split(). Il reste vivant et
  # reessaie au cycle suivant - indispensable quand deka-start lance paplon et
  # oclvankus quasi en meme temps.
  if l is None:
    return False

  jobnum = int(l.split()[0])

  if jobnum == -1:
    return False

  keystream = l.split()[1]

  part_add(jobnum, keystream)

  return True


# Ask our master for startpoints to finish
def get_startpoints():
  sock.sendall(bytes("getstart\r\n", "ascii"))

  l = getline(sock)

  if l is None:
    return False

  jobnum = int(l.split()[0])

  if jobnum == -1:
    return False

  keystream = l.split()[1]
  plen = int(l.split()[2])

  d = getdata(sock, plen)

  print("Adding start")

  complete_add(jobnum, keystream, d)

  return True


# Post data package to our master
def put_result(command, jobnum, data):
  sendascii(sock, "%s %i %i\r\n"%(command, jobnum, len(data)*8))

  #print("len data %i"%len(data))

  sendblob(sock, data)

# Post computed endpoints to our master
def put_dps(burst, n):
  #print("reporting job %i len %i"%(n, len(burst)))

  put_result("putdps", n, burst)

# Post cracked keys or finished jobs to our master
def put_cracked():

  buf = np.zeros(100, dtype=np.uint8)

  n = libvankus.pop_solution(buf)

  if n >= 0:

    s = buf.tobytes()
    pieces = s.split()

    if pieces[0] == b'Found':

      jobnum = int(pieces[4][1:])

      sendascii(sock, "putkey %i %s\r\n"%(jobnum, toascii(s.rstrip(b'\x00'))))

    if pieces[0] == b'crack':
      jobnum = int(pieces[1][1:])
      sendascii(sock, "finished %i\r\n"%jobnum)



# Add partial job (reconstructing the keyspace to the nearest endpoint)
def part_add(jobnum, bdata):

  # add fragments to queue
  bint = int(bdata,2)

  fragbuf = np.zeros(9*colors*len(mytables)*samples, dtype=np.uint64)

  ind = 0

  # Generate keystream samples for all possible fragment combinations
  for table in mytables:
    for pos in range(0, samples):
      for color in range(0, colors):

        sample = (bint >> (samples-pos-1)) & mask64
        #print("bint %X, sample %X, shift %i"%(bint, sample, (samples-pos-1)))

        fragbuf[ind] = sample
        fragbuf[ind+1] = jobnum
        fragbuf[ind+2] = pos
        fragbuf[ind+3] = 0
        fragbuf[ind+4] = table
        fragbuf[ind+5] = color
        fragbuf[ind+6] = color
        fragbuf[ind+7] = 8
        fragbuf[ind+8] = 0

        ind += 9

        #print("Adding ",fragment)
        #frags_q.put(fragment)

  libvankus.burst_load(fragbuf)

# Add complete job (from the starting point towards the keystream sample)
def complete_add(jobnum, keystream, blob):

  startpoints = struct.unpack("<%iQ"%(len(blob)/8), blob)

  bint = int(keystream,2)

  fragbuf = np.zeros(9*colors*len(mytables)*samples, dtype=np.uint64)

  ind = 0

  # Generate keystream samples for all possible combinations, however,
  # using the sample as a challenge lookup and the starting point as PRNG input
  for table in mytables:
    for pos in range(0, samples):
      for color in range(0, colors):

        sample = (bint >> (samples-pos-1)) & mask64

        fragbuf[ind] = startpoints[abs_idx(pos, table, color)]
        fragbuf[ind+1] = jobnum
        fragbuf[ind+2] = pos
        fragbuf[ind+3] = 0
        fragbuf[ind+4] = table
        fragbuf[ind+5] = 0
        fragbuf[ind+6] = 0
        fragbuf[ind+7] = color+1
        fragbuf[ind+8] = sample

        ind += 9

  libvankus.burst_load(fragbuf)

# Generate blob to be sent to OpenCL
def generate_clblob():

  # Raw OpenCL buffer binary
  clblob = np.zeros(clblobfrags * onefrag, dtype=np.uint64)

  n = libvankus.frag_clblob(clblob)

  return (n,clblob)

# Submit work to OpenCL & wait for results
def krak():
  global x

  (n,clblob) = generate_clblob()

  if n == 0:
    time.sleep(0.3)
    return

  # Generate device buffer
  a = np.zeros(len(clblob), dtype=np.uint64)


  s = np.uint32(a.shape)/4

  # How many kernels to execute
  kernelstoe = (n//(slices)+1,)

  # Launch the kernel
  print("Launching kernel, fragments %i, kernels %i"%(n,kernelstoe[0]))

  print("Host lag %.3f s"%(time.time()-x))
  x = time.time()

  a_dev = cl.Buffer(ctx, cl.mem_flags.READ_ONLY | cl.mem_flags.COPY_HOST_PTR, hostbuf=clblob)
  event = krak_kernel(cmdq, kernelstoe, None, a_dev, s)
  event.wait()
 
  # copy the output from the context to the Python process
  cl.enqueue_copy(cmdq, a, a_dev)

  print("GPU computing: %.3f"%(time.time()-x))
  x = time.time()

  libvankus.report(a)


# Return absolute index of fragment in burst blob
def abs_idx(pos, table, color):
  return mytables.index(table) * samples * colors + pos * colors + color


# Post finished work to our master
def put_work():

  a = np.zeros(colors*len(mytables)*samples, dtype=np.uint64)

  n = libvankus.pop_result(a)

  if n >= 0:
    put_dps(a, n)


# Start the net thread
net_thr = threading.Thread(target=network_thr, args=())
net_thr.start()


# Compile the kernel
FILE_NAME="slice.c"
f=open(FILE_NAME,"r")
SRC = ''.join(f.readlines())
f.close()

prg = cl.Program(ctx, SRC).build()

# FIX: recuperer le kernel UNE seule fois (sinon pyopencl en recree un a chaque
# lancement -> fuite de ressources GPU -> CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES et
# degradation apres le 1er crack). cf. warning RepeatedKernelRetrieval.
krak_kernel = cl.Kernel(prg, "krak")


# Start processing
while (1):
  if not net_thr.is_alive():
    print("Network thread died :-(")
    sys.exit(1)
  krak()

