#!/usr/bin/python3

# Oclvankus is a client/worker that computes chains.

import pyopencl as cl
import numpy as np
import time, socket, os, sys, struct, threading, queue
from collections import namedtuple

# Advance functions
from tables import rft

from libdeka import *

HOST, PORT = "localhost", 1578

mf = cl.mem_flags
# just some context... You can define it with environment variable (you will be asked on first run)
ctx = cl.create_some_context()

#platform = cl.get_platforms()
#my_gpu_devices = platform[0].get_devices(device_type=cl.device_type.CPU)
#ctx = cl.Context(devices=my_gpu_devices)
cmdq = cl.CommandQueue(ctx)

# tables we have
# yeah it's in this silly order
mytables = [380, 220, 100,108,116,124,132,140,148,156,164,172,180,188,196,204,212,230,238,250,260,268,276,292,324,332,340,348,356,364,372,388,396,404,412,420,428,436,492,500]
table_offset = -100
# how many colors are there in each table
colors = 8
# length of one GSM burst
burstlen = 114
# length of one keystream sample
samplelen = 64
# samples in one burst (moving window)
samples = burstlen - samplelen + 1

# how many kernels to run in parallel
kernels = 2048
# slices per kernel
slices = 64
# fragments in cl blob
clblobfrags = kernels * slices

# size of one fragment in longs (we need fragment + RF + challenge + flags)
onefrag = 4

# how many times to submit one chain to OpenCL
maxiters = 20

Fragment = namedtuple("Fragment", "prng job pos iters table start color stop challenge")

# finished partial submits to be forwarded to our master
partjobs = {}

# cracked keys to be forwarded to our master
cracked = queue.Queue()

# queue of fragments to be processed by OpenCL
frags_q = queue.Queue()

# 64 ones
mask64 = 2**64-1

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Reverse bits in integer
def revbits(x):
  return int(bin(x)[2:].zfill(64)[::-1], 2)


# Background thread for network communication
def network_thr():

  master_connect()

  while True:
    print("Network!")
    get_keystream()
    time.sleep(1)
    get_startpoints()
    put_work()
    put_cracked()
    time.sleep(1)


def master_connect():
  global sock

  sock.connect((HOST, PORT))

# Ask our master for keystream to crack
def get_keystream():

  sock.sendall(bytes("getkeystream\r\n", "ascii"))

  l = getline(sock)

  jobnum = int(l.split()[0])

  if jobnum == -1:
    return

  keystream = l.split()[1]

  part_add(jobnum, keystream)


# Ask our master for startpoints to finish
def get_startpoints():

  sock.sendall(bytes("getstart\r\n", "ascii"))

  l = getline(sock)

  jobnum = int(l.split()[0])

  if jobnum == -1:
    return

  keystream = l.split()[1]
  plen = int(l.split()[2])

  d = getdata(sock, plen)

  complete_add(jobnum, keystream, d)


# Post data package to our master
def put_result(command, jobnum, data):

  sendascii(sock, "%s %i %i\r\n"%(command, jobnum, len(data)))

  sendblob(sock, data)

# Post computed endpoints to our master
def put_dps(frags):

  jobnum = frags[0].job

  data = bytes()

  for frag in frags:
    data += struct.pack("<1Q", frag.prng)

  print("reporting job %i len %i"%(jobnum, len(data)))

  put_result("putdps", jobnum, data)

def put_cracked():

  while not cracked.empty():
    put_result("putkey", 666, bytes(cracked.get(), "ascii"))
  


# Add partial job (reconstructing the keyspace to the nearest endpoint)
def part_add(jobnum, bdata):

  # add fragments to queue
  bint = int(bdata,2)

  # initialize empty target
  partjobs[jobnum] = []

  # Generate keystream samples for all possible fragment combinations
  for table in mytables:
    for pos in range(0, samples):
      for color in range(0, colors):

        sample = (bint >> (samples-pos-1)) & mask64
        #print("bint %X, sample %X, shift %i"%(bint, sample, (samples-pos-1)))

        fragment = Fragment(prng = sample,
                            job = jobnum,
                            pos = pos,
                            iters = 0,
                            table = table,
                            color = color,
                            start = color,
                            stop = 8,
                            challenge = 0)

        #print("Adding ",fragment)
        frags_q.put(fragment)


# Add complete job (from the starting point towards the keystream sample)
def complete_add(jobnum, keystream, blob):

  startpoints = struct.unpack("<%iQ"%(len(blob)/8), blob)

  bint = int(keystream,2)

  # Generate keystream samples for all possible combinations, however,
  # using the sample as a challenge lookup and the starting point as PRNG input
  for table in mytables:
    for pos in range(0, samples):
      for color in range(0, colors):

        sample = (bint >> (samples-pos-1)) & mask64

        fragment = Fragment(prng = startpoints[abs_idx(pos, table, color)],
                            job = jobnum,
                            pos = pos,
                            iters = 0,
                            table = table,
                            color = 0,
                            start = 0,
                            stop = color+1,
                            challenge = sample)

        if fragment.prng != 0:
          frags_q.put(fragment)


# Translate color index to a reduction function value
def getrf(table, color):
  return rft[table + table_offset][color]


# Generate blob to be sent to OpenCL
def generate_clblob():

  # Database to keep on host for further lookups
  fragdb = []

  # Raw OpenCL buffer binary
  clblob = np.zeros(clblobfrags * onefrag, dtype=np.uint64)

  # Fill it both with fragments
  for i in range(0,clblobfrags):
    if frags_q.empty():
      break
    fragment = frags_q.get()
    #print("Kernel -> ",fragment)
    fragdb.append(fragment)

    clblob[i * onefrag + 0] = fragment.prng
    clblob[i * onefrag + 1] = getrf(fragment.table, fragment.color)
    clblob[i * onefrag + 2] = fragment.challenge

  return (fragdb, clblob)


# Submit work to OpenCL & wait for results
def krak():

  (fragdb, clblob) = generate_clblob()

  # Generate device buffer
  a = np.zeros(len(clblob), dtype=np.uint64)

  a_dev = cl.Buffer(ctx, cl.mem_flags.READ_ONLY | cl.mem_flags.COPY_HOST_PTR, hostbuf=clblob)

  s = np.uint32(a.shape)/4

  # Compile the kernel
  FILE_NAME="slice.c"
  f=open(FILE_NAME,"r")
  SRC = ''.join(f.readlines())
  f.close()

  prg = cl.Program(ctx, SRC).build()

  x = time.time()

  # How many kernels to execute
  kernels = (len(fragdb)//64+1,)

  # Launch the kernel
  print("Launching kernel, fragments %i, kernels %i"%(len(fragdb),kernels[0]))
  event = prg.krak(cmdq, kernels, None, a_dev, s)
  event.wait()
 
  # copy the output from the context to the Python process
  cl.enqueue_copy(cmdq, a, a_dev)

  print("lag=%f"%(time.time()-x))
  report(fragdb, a)


# Process blob returned from OpenCL
def report(fragdb, a):

  for i in range(0, len(fragdb)):

    old = fragdb[i]
    # If we reached end of color
    if a[i * onefrag + 3] & np.uint64(0x01):

      #print("cl_endpoint %i %x"%(i, a[i * onefrag]))

      # There are still colors to go - update color and resubmit
      if old.color < old.stop - 1:
        # We need to XOR the PRNG with the next RF, as the slice machine does
        # one more XOR than we need so they just cancel out
        fragment = Fragment(
                    prng = a[i * onefrag] ^ np.uint64(getrf(old.table, old.color+1)),
                    job = old.job,
                    pos = old.pos,
                    iters = 0,
                    table = old.table,
                    color = old.color+1,
                    start = old.start,
                    stop = 8,
                    challenge = old.challenge)

        frags_q.put(fragment)

      # There are no more colors - final endpoint was reached
      else:
        fragment = Fragment(prng = a[i * onefrag],
                    job = old.job,
                    pos = old.pos,
                    iters = 0,
                    table = old.table,
                    color = old.color,
                    start = old.start,
                    stop = 8,
                    challenge = old.challenge)

        # If this was a partial lookup, we will forward it to master,
        # otherwise the challenge was not found in table and we silently discard it.
        if fragment.challenge == 0:
          partjobs[old.job].append(fragment)


    # We have not reached distinguished point, so we need to iterate more.
    else:
      fragment = Fragment(prng = a[i * onefrag],
                  job = old.job,
                  pos = old.pos,
                  iters = old.iters + 1,
                  table = old.table,
                  color = old.color,
                  start = old.start,
                  stop = 8,
                  challenge = old.challenge)

      if fragment.iters < maxiters:
        frags_q.put(fragment)
      else:
        print("Kicking out frag %X with RF %i:%i after %i iters - cipher loop?"
               %(fragment.prng, fragment.table, fragment.color, maxiters))

    # Whee, we found a key!
    if a[i * onefrag + 3] & np.uint64(0x02):
      key=revbits(a[i * onefrag])
      cracked.put("putkey %i found %x @ %i  #%i\r\n"%(old.job, key, old.pos, old.job))
      print("cl_keyfound %i %x"%(i, key))


# Return absolute index of fragment in burst blob
def abs_idx(pos, table, color):
  return mytables.index(table) * samples * colors + pos * colors + color


# Post finished work to our master
def put_work():
  k = list(partjobs.keys())
  for i in k:
    if len(partjobs[i]) >= len(mytables) * samples * colors: # the job is finished
      reportjob(i)


# Post single finished job to our master
def reportjob(idx):
  partjobs[idx].sort(key=lambda x: abs_idx(x.pos, x.table, x.start))
  put_dps(partjobs[idx])
  #print("Reporting finish")
  partjobs.pop(idx)


# Start the net thread
net_thr = threading.Thread(target=network_thr, args=())
net_thr.start()


# Start processing
while (1):
  if not net_thr.is_alive():
    print("Network thread died :-(")
    sys.exit(1)
  if not frags_q.empty():
    krak()
  else:
    time.sleep(1)
