#!/usr/bin/python3
# delta_toy_client.py - copie directe de delta_client.py : meme protocole
# (getdps/putstart avec paplon.py), meme boucle. Seule la partie "tables"
# change : au lieu de "import delta" + delta.delta_init() (mmap des 40
# fichiers .idx, plusieurs To) et delta.ncq_submit()/ncq_read() (recherche
# dans ces tables), on charge toy_table.tsv (crack_toy.py) - RAND=0, Ki
# connu sauf 2 octets.
#
# NB: le format binaire echange avec paplon a ce stade (getdps/putstart) est
# un lot de points distingues issus de la marche de chaine TMTO (voir
# delta.c/delta_binary.h), pas le keystream ni le compteur de trame. Ce
# script ne peut donc pas verifier Ki avec ces donnees - il pompe la
# structure de delta_client.py comme demande, la table toy chargee restant
# disponible pour un usage futur.

import socket
import sys
import time

from libdeka import *

from vankusconf import HOST, PORT

import crack_toy

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

sock.connect((HOST, PORT))

try:
    table = crack_toy.load_table()
except FileNotFoundError:
    crack_toy.build_table()
    table = crack_toy.load_table()

d = bytes()
y = bytearray()

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

    x=time.time()

    # toy: pas de mmap/recherche sur les tables reelles (table.tsv deja
    # chargee en memoire ci-dessus)
    print("submit took: %f s"%(time.time()-x))
    x=time.time()

    print("process")

    # toy: rien a lire depuis les tables (voir note en tete de fichier)
    print("process took: %f s"%(time.time()-x))

    sendascii(sock, "putstart %i %i\r\n"%(jobnum, plen))

    sendblob(sock, y)
