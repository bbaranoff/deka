#!/usr/bin/python3
"""
tsv2dlt.py - normalise toy_table.tsv (crack_toy.py) vers le format binaire
deka/Kraken lu par delta_binary.h : produit

    toy_table.dlt   les blocs de 4096 o delta-encodes (les donnees, "behemoth")
    toy_table.idx   l'index d'endpoints uint64 lu par load_idx()

L'encodeur est l'inverse EXACT du decodeur de delta_binary.h
(load_idx / StartEndpointSearch / CompleteEndpointSearch), et un test
round-trip re-decode les 65536 entrees avec un portage fidele du decodeur
pour prouver que delta.c INCHANGE retrouvera bien suffixe = f(endpoint).

--- Contrainte de format (importante) ------------------------------------
Le format Kraken code, par bloc :
  * un endpoint uint64 (base du bloc) dans le .idx ;
  * dans le .dlt, la 1re chaine du bloc = un index 34 bits (le "startpoint"),
    d'endpoint == base ; les chaines suivantes ajoutent delta<<12 a `here`.
load_idx() calcule mStepSize = (2^52-1)/(num+1) et REJETTE (exit 1) tout
bloc dont l'ecart d'endpoints devie de mStepSize de plus de 2^31
(offset = (end>>12) - last - mStepSize, |offset| < 0x7fffffff).

Avec seulement 65536 entrees, des endpoints = sortie COMP128 brute (aleatoires
sur 2^64) donnent des ecarts de variance ~2^36 >> 2^31 : ~98 % des blocs
seraient "corrupt". Le delta intra-bloc (30 bits d'echappement, cap 2^30 en
unites de 2^12) est lui aussi < a l'ecart moyen 2^36. Le format suppose des
MILLIONS de chaines ; a l'echelle toy le seul agencement valide est :
UNE chaine par bloc, endpoints sur un reseau regulier e_i = i*mStepSize
(offset = 0 partout). C'est ce que fait ce script.

=> endpoint_i = (i * mStepSize) << 12   (multiple de 4096, low12=0)
   index_i (34 bits) = suffixe XXYY de la ligne i (16 bits, tient large)

La correspondance sortie_COMP128 -> i (le rang) est ecrite dans le sidecar
toy_table.map.tsv (out_hex \t i \t endpoint_hex \t suffixe_hex) : c'est ce
qui reste a cabler cote requete (calcul de l'endpoint a interroger).

Cablage cote delta.c (a la charge de l'appelant) :
  files[tbl]   = ".../toy_table.idx"
  devpaths[d]  = ".../toy_table.dlt"   ; devs[tbl]=d ; offsets[tbl]=0
  NB: delta.c post-traite l'index par rev(ApplyIndexFunc(idx,34)). Le toy
  stocke le suffixe BRUT ; desactive/adapte ce post-traitement cote toy.

Usage:
  python3 tsv2dlt.py build   [-t toy_table.tsv] [--dlt toy_table.dlt] [--idx toy_table.idx]
  python3 tsv2dlt.py verify  [--dlt ...] [--idx ...] [-t toy_table.tsv]  (round-trip complet)
"""

import argparse
import struct
import sys

BLOCK = 4096
STEP_SPACE = 0xfffffffffffff  # 2^52-1, la constante de load_idx (mStepSize base)

DEFAULT_TSV = "toy_table.tsv"
DEFAULT_DLT = "toy_table.dlt"
DEFAULT_IDX = "toy_table.idx"
DEFAULT_MAP = "toy_table.map.tsv"


# ---------------------------------------------------------------------------
# Lecture du TSV
# ---------------------------------------------------------------------------
def read_tsv(path):
    rows = []  # (out_hex, out_u64, suffix)
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            out_hex, suf_hex = line.split("\t")
            ob = bytes.fromhex(out_hex)
            out_u64 = int.from_bytes(ob[:8], "big")  # 64 premiers bits de la sortie
            rows.append((out_hex, out_u64, int(suf_hex, 16)))
    return rows


def step_size(n):
    return STEP_SPACE // (n + 1)


# ---------------------------------------------------------------------------
# ENCODEUR
# ---------------------------------------------------------------------------
def encode_first_record(index34):
    """5 octets encodant l'index 34 bits lu par la 1re chaine du bloc :
       READ8 x4 -> 32 bits, READN(2) -> +2 bits.
       index34 = (p0<<26)|(p1<<18)|(p2<<10)|(p3<<2)|(p4>>6)."""
    p0 = (index34 >> 26) & 0xFF
    p1 = (index34 >> 18) & 0xFF
    p2 = (index34 >> 10) & 0xFF
    p3 = (index34 >> 2) & 0xFF
    p4 = (index34 & 0x3) << 6
    return bytes((p0, p1, p2, p3, p4))


# Groupe de blocs sentinelles ajoute apres les vraies chaines : sans lui, la
# navigation de StartEndpointSearch pour le tout dernier endpoint calcule
# bid//256 == (nb_groupes) et lit un index primaire au-dela du dernier groupe
# reel. Un groupe de rab (256 blocs) garantit primary[bid//256] present.
PAD = 256


def build(tsv_path, dlt_path, idx_path, map_path):
    rows = read_tsv(tsv_path)
    n = len(rows)
    m = n + PAD                       # blocs totaux (vrais + sentinelles)
    ms = step_size(m)                 # mStepSize calcule sur num = m
    # tri par sortie -> le sidecar est cherchable par sortie ; l'endpoint
    # reste un reseau regulier fonction du rang.
    rows.sort(key=lambda r: r[1])

    # E_i = (i+1)*ms (en unites de end>>12) -> offset = 0 partout, y compris
    # le 1er bloc (last part de 0). endpoint = E_i << 12 (multiple de 4096).
    endpoints = [((i + 1) * ms) << 12 for i in range(m)]

    # .dlt : un bloc de 4096 o par chaine, ecrit en creux (le reste = 0).
    # Les n premiers blocs portent le suffixe ; les PAD derniers = index 0.
    with open(dlt_path, "wb") as f:
        for i in range(m):
            suffix = rows[i][2] if i < n else 0
            f.seek(i * BLOCK)
            f.write(encode_first_record(suffix))  # 5 o utiles, reste = trou (0)
        f.seek(m * BLOCK - 1)
        f.write(b"\x00")  # etend le fichier a m*4096 (trou final)

    # .idx : m endpoints (bases de bloc) + 1 mHighEndpoint. Entiers natifs LE.
    with open(idx_path, "wb") as f:
        for e in endpoints:
            f.write(struct.pack("<Q", e))
        f.write(struct.pack("<Q", endpoints[-1]))  # mHighEndpoint = max

    # sidecar sortie -> rang / endpoint / suffixe
    with open(map_path, "w") as f:
        for i, (out_hex, _u64, suffix) in enumerate(rows):
            f.write(f"{out_hex}\t{i}\t{endpoints[i]:016x}\t{suffix:04x}\n")

    # controle de la contrainte offset de load_idx
    max_off = 0
    last = 0
    for e in endpoints:
        off = (e >> 12) - last - ms
        last = e >> 12
        max_off = max(max_off, abs(off))
    print(f"entrees        : {n}")
    print(f"mStepSize      : {ms} (0x{ms:x})")
    print(f"offset max     : {max_off}  (limite load_idx: {0x7fffffff})  -> "
          f"{'OK' if max_off < 0x7fffffff else 'CORRUPT'}")
    print(f"delta intra-bloc: 0 (1 chaine/bloc, pas de contrainte 30 bits)")
    print(f".dlt ecrit     : {dlt_path}  ({n*BLOCK} o logiques, ecrit en creux)")
    print(f".idx ecrit     : {idx_path}  ({(n+1)*8} o)")
    print(f"sidecar        : {map_path}")


# ---------------------------------------------------------------------------
# DECODEUR (portage fidele de delta_binary.h) - pour le round-trip
# ---------------------------------------------------------------------------
def load_idx_py(idx_path):
    with open(idx_path, "rb") as f:
        data = f.read()
    vals = list(struct.unpack("<%dQ" % (len(data) // 8), data))
    num = len(vals) // 8 - 1 if False else len(vals) - 1  # num = size/8 - 1
    ms = STEP_SPACE // (num + 1)
    block_index = [0] * (num + 1)
    primary = {}
    last = 0
    for bl in range(num):
        end = vals[bl]
        off = (end >> 12) - last - ms
        last = end >> 12
        assert -0x7fffffff < off < 0x7fffffff, f"index corrupt bl={bl} off={off}"
        block_index[bl] = off
        if (bl & 0xFF) == 0:
            primary[bl >> 8] = end
    block_index[num] = 0x7fffffff
    low = primary[0]
    high = vals[num]
    return dict(num=num, ms=ms, bi=block_index, prim=primary, low=low, high=high)


class BitReader:
    def __init__(self, buf):
        self.b = buf
        self.pos = 0
        self.buf = self.b[self.pos]
        self.pos = 1
        self.cnt = 8

    def read8(self):
        bits = (self.buf >> (self.cnt - 8)) & 0xFF
        nxt = self.b[self.pos] if self.pos < len(self.b) else 0
        self.buf = ((self.buf << 8) | nxt) & 0xFFFFFFFF
        self.pos += 1
        return bits

    def readn(self, n):
        bits = (self.buf >> (self.cnt - n)) & ((1 << n) - 1)
        self.cnt -= n
        if self.cnt < 8:
            nxt = self.b[self.pos] if self.pos < len(self.b) else 0
            self.buf = ((self.buf << 8) | nxt) & 0xFFFFFFFF
            self.pos += 1
            self.cnt += 8
        return bits


def complete_endpoint_search(block, here, end):
    r = BitReader(block)
    tmp = r.read8()
    tmp = (tmp << 8) | r.read8()
    tmp = (tmp << 8) | r.read8()
    tmp = (tmp << 8) | r.read8()
    tmp = (tmp << 2) | r.readn(2)
    if here == end:
        return tmp
    # (chaines suivantes non utilisees dans le toy : 1 chaine/bloc)
    return 0


def start_endpoint_search(idx, block_reader, end):
    if end < idx["low"]:
        return None
    if end > idx["high"]:
        return None
    ms = idx["ms"]
    bid = (end >> 12) // ms
    bl = (bid & 0xFFFFFFFF) // 256
    while bl and idx["prim"].get(bl, 0) > end:
        bl -= 1
    here = idx["prim"][bl]
    bl = bl * 256
    delta = (ms + idx["bi"][bl + 1]) << 12
    while (here + delta) <= end and bl < idx["num"] + 1:
        here += delta
        bl += 1
        delta = (ms + idx["bi"][bl + 1]) << 12
    block = block_reader(bl)
    return complete_endpoint_search(block, here, end)


def verify(tsv_path, dlt_path, idx_path):
    rows = read_tsv(tsv_path)
    rows.sort(key=lambda r: r[1])
    n = len(rows)
    m = n + PAD
    ms = step_size(m)
    idx = load_idx_py(idx_path)
    assert idx["num"] == m, f"num={idx['num']} != {m}"

    fdlt = open(dlt_path, "rb")

    def block_reader(bl):
        fdlt.seek(bl * BLOCK)
        return fdlt.read(BLOCK)

    ok = 0
    fail = 0
    for i, (_out, _u64, suffix) in enumerate(rows):
        end = ((i + 1) * ms) << 12
        got = start_endpoint_search(idx, block_reader, end)
        if got == suffix:
            ok += 1
        else:
            fail += 1
            if fail <= 5:
                print(f"  FAIL i={i} end={end:x} attendu={suffix:04x} obtenu={got}")
    fdlt.close()
    print(f"round-trip: {ok}/{n} OK, {fail} echecs")
    return fail == 0


def _cli():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    pb = sub.add_parser("build", help="genere .dlt + .idx + sidecar")
    pb.add_argument("-t", "--tsv", default=DEFAULT_TSV)
    pb.add_argument("--dlt", default=DEFAULT_DLT)
    pb.add_argument("--idx", default=DEFAULT_IDX)
    pb.add_argument("--map", default=DEFAULT_MAP)

    pv = sub.add_parser("verify", help="round-trip complet via le decodeur porte")
    pv.add_argument("-t", "--tsv", default=DEFAULT_TSV)
    pv.add_argument("--dlt", default=DEFAULT_DLT)
    pv.add_argument("--idx", default=DEFAULT_IDX)

    args = ap.parse_args()
    if args.cmd == "build":
        build(args.tsv, args.dlt, args.idx, args.map)
    elif args.cmd == "verify":
        sys.exit(0 if verify(args.tsv, args.dlt, args.idx) else 1)


if __name__ == "__main__":
    _cli()
