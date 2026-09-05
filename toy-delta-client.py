#!/usr/bin/python3
"""
toy-delta-client.py - client reseau pour le banc COMP128v1 "toy".

Se connecte au banc en TCP (meme esprit que delta_client.py pour le
job-server A5/1), envoie le defi RAND=0 fixe, recupere la reponse
(SRES||Kc, 12 octets) et retrouve les 2 derniers octets de Ki en
consultant la table generee par crack_toy.py (voir `crack_toy.py build`).

!!! Le protocole reseau ci-dessous (fonction query_bench) est un
placeholder raisonnable, pas une norme : adaptez-le a ce que repond
reellement votre banc. Tel quel il :
  - envoie une ligne ASCII "AUTH <rand_hex_32>\r\n"
  - lit une ligne d'entete "<longueur>\r\n"
  - lit exactement <longueur> octets bruts (attendu: 12 = sres(4)+kc(8))

Usage:
  python3 crack_toy.py build                     # une seule fois
  python3 toy-delta-client.py --host 127.0.0.1 --port 6667 [--expect 0123]
"""

import argparse
import socket
import sys

from libdeka import getline, getdata, sendascii

import crack_toy


def query_bench(sock: socket.socket, rand_hex: str) -> bytes:
    """Envoie le defi RAND au banc, retourne les octets de reponse (sres+kc).
    A adapter au protocole reel du banc si different."""
    sendascii(sock, f"AUTH {rand_hex}\r\n")

    header = getline(sock)
    if header is None:
        raise ConnectionError("le banc a ferme la connexion avant d'envoyer l'entete")

    plen = int(header.split()[0])
    data = getdata(sock, plen)
    return data


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default="localhost")
    ap.add_argument("--port", type=int, default=6667)
    ap.add_argument("-t", "--table", default=crack_toy.DEFAULT_TABLE_PATH)
    ap.add_argument("--rand", default=crack_toy.RAND_HEX, help="RAND en hexa (32 caracteres), defaut = 0")
    ap.add_argument("--expect", help="suffixe XXYY attendu sur le banc, pour verification")
    args = ap.parse_args()

    try:
        table = crack_toy.load_table(args.table)
    except FileNotFoundError:
        print(f"table introuvable: {args.table} -- lancez d'abord: python3 crack_toy.py build -o {args.table}")
        sys.exit(1)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((args.host, args.port))

    try:
        response = query_bench(sock, args.rand)
    finally:
        sock.close()

    observed_hex = response.hex()
    print(f"reponse du banc (sres+kc): {observed_hex}")

    matches = crack_toy.identify(observed_hex, table)
    if not matches:
        print("aucune correspondance dans la table toy -- Ki hors du motif attendu, "
              "RAND different de 0, ou protocole query_bench a adapter")
        sys.exit(1)

    for suffix_hex in matches:
        ki = crack_toy.ki_for_suffix(bytes.fromhex(suffix_hex))
        print(f"Ki retrouve: {ki.hex()} (suffixe {suffix_hex})")

    if args.expect:
        expect = args.expect.strip().lower()
        if expect in matches:
            print(f"MATCH: le banc a bien Ki=...{expect}")
        else:
            print(f"MISMATCH: attendu ...{expect}, trouve {matches}")
            sys.exit(1)


if __name__ == "__main__":
    main()
