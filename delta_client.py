#!/usr/bin/python3

import socket
import sys
import delta
import time

from libdeka import *

import delta

HOST, PORT = "localhost", 1578

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

sock.connect((HOST, PORT))

delta.delta_init()

solved=0

while 1:

  sock.sendall(bytes("getdps\r\n", "ascii"))

  header = getline(sock)

  jobnum = int(header.split()[0])
  plen = int(header.split()[1])


  if plen == 0:
    time.sleep(1)
  else:
    print(header)
 
    d = getdata(sock, plen)

    # convert to mutable bytearray, some swig magic
    y = bytearray(d)

    #delta.submitblocks(x)

    # now the kernel is processing our request

    x=time.time()
    print("submit")
    delta.ncq_submit(y)
    print("%f s"%(time.time()-x))
    x=time.time()

    print("process")
    delta.ncq_read(y)
    print("%f s"%(time.time()-x))

    sendascii(sock, "putstart %i %i\r\n"%(jobnum, plen))

    sendblob(sock, y)

    solved += 1
    if solved > 1700:
      sys.exit(0)


