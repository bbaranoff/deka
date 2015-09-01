#!/usr/bin/python3

# Oclvankus is a client/worker that computes chains.

#from __future__ import print_function

import pyopencl as cl
import numpy as np
import time, socket, os, sys, struct, threading
import queue #as queue
from collections import namedtuple

from libdeka import *

import libvankus

import pickle
from datetime import datetime

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
kernels = 4095
# slices per kernel
slices = 32
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

fragdb=[]

# 64 ones
mask64 = 2**64-1

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

x = 0

mywork = 0

# Reverse bits in integer
def revbits(x):
  return int(bin(x)[2:].zfill(64)[::-1], 2)


# Background thread for network communication
def network_thr():
  global frags_q, fragdb

  master_connect()

  while True:
    #print("Network!")

    n = libvankus.getnumfree()

    print("%i free slots"%n)

    if n > 2:
      get_keystream()
      get_startpoints()
    put_work()
    put_cracked()
    time.sleep(0.3)


def master_connect():
  global sock

  sock.connect((HOST, PORT))

# Ask our master for keystream to crack
def get_keystream():
  global mywork

  sock.sendall(bytes("getkeystream\r\n", "ascii"))

  l = getline(sock)

  jobnum = int(l.split()[0])

  if jobnum == -1:
    return

  keystream = l.split()[1]

  mywork += 1
  part_add(jobnum, keystream)


# Ask our master for startpoints to finish
def get_startpoints():
  global mywork

  sock.sendall(bytes("getstart\r\n", "ascii"))

  l = getline(sock)

  jobnum = int(l.split()[0])

  if jobnum == -1:
    return

  keystream = l.split()[1]
  plen = int(l.split()[2])

  d = getdata(sock, plen)

  mywork += 1

  print("Adding start")

  complete_add(jobnum, keystream, d)


# Post data package to our master
def put_result(command, jobnum, data):
  global mywork

  sendascii(sock, "%s %i %i\r\n"%(command, jobnum, len(data)*8))

  #print("len data %i"%len(data))

  mywork -= 1
  sendblob(sock, data)

# Post computed endpoints to our master
def put_dps(burst, n):
  global mywork

  #print("reporting job %i len %i"%(n, len(burst)))

  mywork -= 1

  put_result("putdps", n, burst)

def put_cracked():

  buf = np.zeros(100, dtype=np.uint8)

  n = libvankus.pop_solution(buf)

  if n >= 0:

    s = buf.tostring()
    pieces = s.split()

    if pieces[0] == b'Found':

      jobnum = int(pieces[4][1:])

      sendascii(sock, "putkey %i %s\r\n"%(jobnum, toascii(s.rstrip(b'\x00'))))

    if pieces[0] == b'crack':
      jobnum = int(pieces[1][1:])
      sendascii(sock, "finished %i\r\n"%jobnum)

    #put_result("putkey", 666, buf.tostring())
  


# Add partial job (reconstructing the keyspace to the nearest endpoint)
def part_add(jobnum, bdata):

  # add fragments to queue
  bint = int(bdata,2)

  # initialize empty target
  partjobs[jobnum] = []

  fragbuf = np.zeros(9*16320, dtype=np.uint64)

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

  fragbuf = np.zeros(9*16320, dtype=np.uint64)

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

        #if fragment[0] != 0:
          #frags_q.put(fragment)
  libvankus.burst_load(fragbuf)


# Translate color index to a reduction function value
def getrf(table, color):
  return rft[table + table_offset][color]


# Generate blob to be sent to OpenCL
def generate_clblob():

  # Raw OpenCL buffer binary
  clblob = np.zeros(clblobfrags * onefrag, dtype=np.uint64)

  n = libvankus.frag_clblob(clblob)

  #print("Got %i clblob"%n)

  #for u in clblob:
  #  print("%X"%u)
  #sys.exit(0)
  return (n,clblob)

# Submit work to OpenCL & wait for results
def krak():
  global x

  (n,clblob) = generate_clblob()

  if n == 0:
    time.sleep(0.3)
    return

  #wow=datetime.now().strftime("%Y-%m-%dT%H-%M-%S.%f")[:23]
  #print("Saving blob to %s"%wow)

  #o=open(wow+'.clblob', 'wb')
  #pickle.dump(clblob, o, pickle.HIGHEST_PROTOCOL)
  #o.close()

  #for lo in clblob:
  #  print("%X"%lo)

  # Generate device buffer
  a = np.zeros(len(clblob), dtype=np.uint64)


  s = np.uint32(a.shape)/4

  # How many kernels to execute
  kernelstoe = (n//(32)+1,)

  # Launch the kernel
  print("Launching kernel, fragments %i, kernels %i"%(n,kernelstoe[0]))

  print("Host lag %.3f s"%(time.time()-x))
  x = time.time()

  a_dev = cl.Buffer(ctx, cl.mem_flags.READ_ONLY | cl.mem_flags.COPY_HOST_PTR, hostbuf=clblob)
  event = prg.krak(cmdq, kernelstoe, None, a_dev, s)
  event.wait()
 
  # copy the output from the context to the Python process
  cl.enqueue_copy(cmdq, a, a_dev)

  print("GPU computing: %.3f"%(time.time()-x))
  x = time.time()

  #o=open(wow+'.a', 'wb')
  #pickle.dump(a, o, pickle.HIGHEST_PROTOCOL)
  #o.close()

  #for lo in a:
  #  print("%08X"%lo)

  report(a)


# Process blob returned from OpenCL
def report( a):

  libvankus.report(a)

  #key=revbits(a[i * onefrag])
  #cracked.put("putkey %i found %x @ %i  #%i (table:%i)\r\n"%(old[1], key, old[2], old[1], old[4]))
  #print("cl_keyfound %i %x"%(i, key))


# Return absolute index of fragment in burst blob
def abs_idx(pos, table, color):
  return mytables.index(table) * samples * colors + pos * colors + color


# Post finished work to our master
def put_work():

  a = np.zeros(16320, dtype=np.uint64)

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


# Start processing
while (1):
  if not net_thr.is_alive():
    print("Network thread died :-(")
    sys.exit(1)
  #if not frags_q.empty():
  krak()
 # else:
  #time.sleep(0.3)
