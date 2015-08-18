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

while 1:

  sock.sendall(bytes("getdps\r\n", "ascii"))

  header = getline(sock)

  jobnum = int(header.split()[0])
  plen = int(header.split()[1])

  print(header)

  if plen == 0:
    time.sleep(1)
  else:

    d = getdata(sock, plen)

    # convert to mutable bytearray, some swig magic
    x = bytearray(d)

    #delta.submitblocks(x)

    # now the kernel is processing our request

    print("submit")
    delta.ncq_submit(x)

    print("process")
    delta.ncq_read(x)

    sendascii(sock, "putstart %i %i\r\n"%(jobnum, plen))

    sendblob(sock, x)


