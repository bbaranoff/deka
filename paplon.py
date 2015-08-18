#!/usr/bin/python3

import struct
import time
import sys
import queue
import os
import math
import socket
import select
import re
import threading

from collections import namedtuple

from libdeka import *

import socketserver

HOST = "localhost"
PORT = 1578

jobstages = [ "submitted",   # user submitted keystream
              "dpsearch",    # worker is searching for distinguished points
              "endpoints",   # worker submitted computed endpoints
              "startsearch", # worker is looking startpoints up in tables
              "startpoints", # worker submitted startpoints
              "collsearch",  # worker is recomputing chain from the beginning, 
                             #  trying to find collisions
              "finished"     # everything done
            ]

jobptr = 0

JobT = Struct("Job", "num time stage keystream blob plaintext")
def Job(time=time.time(), stage="submitted", keystream="", blob=bytes(), plaintext=[]):
  global jobptr
  jobptr += 1
  return JobT(jobptr-1, time, stage, keystream, blob, plaintext)

jobs = []

def getfjob(stage):
  global jobs

  for job in jobs:
    if job.stage == stage:
      return job
  return None

def rq_crack(req, header):
  global jobs

  keystream = header.split()[1]
  if not re.search("^[01]{114}$", keystream):
    sendascii(req, "Keystream must be exactly one GSM burst, i.e. 114 bits\r\n")
    return
  job = Job(keystream = keystream)
  sendascii(req, "Cracking #%i %s\r\n"%(job.num, job.keystream))
  jobs.append(job)

def rq_getkeystream(req, header):
  job = getfjob("submitted")

  if job == None:
    sendascii(req, "-1 0\r\n")
  else:
    sendascii(req, "%i %s\r\n"%(job.num, job.keystream))
    jobs[job.num].stage = "dpsearch"

def rq_putdps(req, header):
  jobnum = int(header.split()[1])
  plen = int(header.split()[2])

  payload = getdata(req, plen)

  jobs[jobnum].blob = payload
  jobs[jobnum].stage = "endpoints"

def rq_getdps(req, header):
  job = getfjob("endpoints")

  if job == None:
    sendascii(req, "-1 0\r\n")
  else:
    sendascii(req, "%i %i\r\n"%(job.num, len(job.blob)))
    sendblob(req, job.blob)
    jobs[job.num].stage = "startsearch"

def rq_putstart(req, header):
  jobnum = int(header.split()[1])
  plen = int(header.split()[2])

  payload = getdata(req, plen)

  jobs[jobnum].blob = payload
  jobs[jobnum].stage = "startpoints"

def rq_getstart(req, header):
  job = getfjob("startpoints")

  if job == None:
    sendascii(req, "-1 0\r\n")
  else:
    sendascii(req, "%i %s %i\r\n"%(job.num, job.keystream, len(job.blob)))
    sendblob(req, job.blob)
    jobs[job.num].stage = "collsearch"

def rq_putkey(req, header):
  jobnum = int(header.split()[1])
  keyinfo = ''.join(header.split()[2:])

  print("Found %s"%(keyinfo))

def rq_finished(req, header):
  jobnum = int(header.split()[1])
  jobs[job.num].stage = "finished"
  # FIXME garbage collection

def rq_stats(req, header):
  global jobs

  cnts = {}
  for stage in jobstages:
    cnts[stage] = 0

  for job in jobs:
    cnts[job.stage] += 1

  for stage in jobstages:
    sendascii(req, "%s: %i\r\n"%(stage, cnts[stage]))

def rq_unknown(req, header):
  cmd = ""
  if len(header.split()) > 0:
    cmd = header.split()[0]
  sendascii(req, "Unknown command %s\r\n"%cmd)

class ThreadedTCPRequestHandler(socketserver.BaseRequestHandler):

  # this creates new thread for each client
  def handle(self):
    print("Connect from %s:%i"%self.request.getpeername())

    # just process requests from client infinitely
    while 1:

      # read request header
      header = getline(self.request)

      if not header:
        print("Disconnect %s:%i"%self.request.getpeername())
        self.request.close()
        break

      # decide what type it is and process accordingly
      rtype = ""
      if len(header.split()) > 0:
        rtype = header.split()[0]

      if rtype == "crack":
        rq_crack(self.request, header)
      elif rtype == "getkeystream":
        rq_getkeystream(self.request, header)
      elif rtype == "putdps":
        rq_putdps(self.request, header)
      elif rtype == "getdps":
        rq_getdps(self.request, header)
      elif rtype == "putstart":
        rq_putstart(self.request, header)
      elif rtype == "getstart":
        rq_getstart(self.request, header)
      elif rtype == "putkey":
        rq_putkey(self.request, header)
      elif rtype == "finished":
        rq_finished(self.request, header)
      elif rtype == "stats":
        rq_stats(self.request, header)
      else:
        rq_unknown(self.request, header)

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
  pass

# bind to socket and start accepting clients
server = ThreadedTCPServer((HOST, PORT), ThreadedTCPRequestHandler)
server.serve_forever()


