#!/usr/bin/python3
"""
crack_toy.py - COMP128v1 toy Ki finisher.

Contexte : sur le banc de test, Ki est programme avec un motif connu
  Ki = 00 11 22 33 44 55 66 77 88 99 aa bb cc dd XX YY
et seuls les 2 derniers octets (XX YY) sont inconnus/a verifier (65536
possibilites). Le banc repond a un defi RAND avec le triplet COMP128v1
(SRES || Kc, 12 octets). Comme l'espace inconnu est minuscule (2^16),
on n'a pas besoin des tables de partitionnement (les "4 To" de disques
utilises pour l'attaque COMP128v1 generale) : une table de correspondance
"sortie -> XX YY" tient en quelques Mo et se genere en quelques secondes.

Ce script :
  - contient une implementation pure Python de COMP128v1, portee et
    verifiee (voir `selftest`) depuis src/gsm/comp128.c de libosmocore
    (tables identiques a la reecriture FreeRADIUS/CryptoMobile - deux
    implementations independantes, meme constantes).
  - genere/charge la table de correspondance pour RAND fixe = 0.
  - retrouve XX YY (et donc le Ki complet) a partir d'une sortie observee.

Usage:
  python3 crack_toy.py selftest
  python3 crack_toy.py build   [-o toy_table.tsv]
  python3 crack_toy.py lookup <sres+kc hex 24 chars> [-t toy_table.tsv] [--expect XXYY]
"""

import argparse
import sys

# ---------------------------------------------------------------------------
# COMP128v1 (A3/A8), pure python port of libosmocore's src/gsm/comp128.c
# Tables cross-checked against the independent FreeRADIUS/CryptoMobile
# rewrite (byte-for-byte identical) and against a local C compile of the
# libosmocore source for 3 test vectors (see selftest()).
# ---------------------------------------------------------------------------

TABLE_0 = [
 102, 177, 186, 162,   2, 156, 112,  75,  55,  25,   8,  12, 251, 193, 246, 188,
 109, 213, 151,  53,  42,  79, 191, 115, 233, 242, 164, 223, 209, 148, 108, 161,
 252,  37, 244,  47,  64, 211,   6, 237, 185, 160, 139, 113,  76, 138,  59,  70,
  67,  26,  13, 157,  63, 179, 221,  30, 214,  36, 166,  69, 152, 124, 207, 116,
 247, 194,  41,  84,  71,   1,  49,  14,  95,  35, 169,  21,  96,  78, 215, 225,
 182, 243,  28,  92, 201, 118,   4,  74, 248, 128,  17,  11, 146, 132, 245,  48,
 149,  90, 120,  39,  87, 230, 106, 232, 175,  19, 126, 190, 202, 141, 137, 176,
 250,  27, 101,  40, 219, 227,  58,  20,  51, 178,  98, 216, 140,  22,  32, 121,
  61, 103, 203,  72,  29, 110,  85, 212, 180, 204, 150, 183,  15,  66, 172, 196,
  56, 197, 158,   0, 100,  45, 153,   7, 144, 222, 163, 167,  60, 135, 210, 231,
 174, 165,  38, 249, 224,  34, 220, 229, 217, 208, 241,  68, 206, 189, 125, 255,
 239,  54, 168,  89, 123, 122,  73, 145, 117, 234, 143,  99, 129, 200, 192,  82,
 104, 170, 136, 235,  93,  81, 205, 173, 236,  94, 105,  52,  46, 228, 198,   5,
  57, 254,  97, 155, 142, 133, 199, 171, 187,  50,  65, 181, 127, 107, 147, 226,
 184, 218, 131,  33,  77,  86,  31,  44,  88,  62, 238,  18,  24,  43, 154,  23,
  80, 159, 134, 111,   9, 114,   3,  91,  16, 130,  83,  10, 195, 240, 253, 119,
 177, 102, 162, 186, 156,   2,  75, 112,  25,  55,  12,   8, 193, 251, 188, 246,
 213, 109,  53, 151,  79,  42, 115, 191, 242, 233, 223, 164, 148, 209, 161, 108,
  37, 252,  47, 244, 211,  64, 237,   6, 160, 185, 113, 139, 138,  76,  70,  59,
  26,  67, 157,  13, 179,  63,  30, 221,  36, 214,  69, 166, 124, 152, 116, 207,
 194, 247,  84,  41,   1,  71,  14,  49,  35,  95,  21, 169,  78,  96, 225, 215,
 243, 182,  92,  28, 118, 201,  74,   4, 128, 248,  11,  17, 132, 146,  48, 245,
  90, 149,  39, 120, 230,  87, 232, 106,  19, 175, 190, 126, 141, 202, 176, 137,
  27, 250,  40, 101, 227, 219,  20,  58, 178,  51, 216,  98,  22, 140, 121,  32,
 103,  61,  72, 203, 110,  29, 212,  85, 204, 180, 183, 150,  66,  15, 196, 172,
 197,  56,   0, 158,  45, 100,   7, 153, 222, 144, 167, 163, 135,  60, 231, 210,
 165, 174, 249,  38,  34, 224, 229, 220, 208, 217,  68, 241, 189, 206, 255, 125,
  54, 239,  89, 168, 122, 123, 145,  73, 234, 117,  99, 143, 200, 129,  82, 192,
 170, 104, 235, 136,  81,  93, 173, 205,  94, 236,  52, 105, 228,  46,   5, 198,
 254,  57, 155,  97, 133, 142, 171, 199,  50, 187, 181,  65, 107, 127, 226, 147,
 218, 184,  33, 131,  86,  77,  44,  31,  62,  88,  18, 238,  43,  24,  23, 154,
 159,  80, 111, 134, 114,   9,  91,   3, 130,  16,  10,  83, 240, 195, 119, 253,
]
TABLE_1 = [
  19,  11,  80, 114,  43,   1,  69,  94,  39,  18, 127, 117,  97,   3,  85,  43,
  27, 124,  70,  83,  47,  71,  63,  10,  47,  89,  79,   4,  14,  59,  11,   5,
  35, 107, 103,  68,  21,  86,  36,  91,  85, 126,  32,  50, 109,  94, 120,   6,
  53,  79,  28,  45,  99,  95,  41,  34,  88,  68,  93,  55, 110, 125, 105,  20,
  90,  80,  76,  96,  23,  60,  89,  64, 121,  56,  14,  74, 101,   8,  19,  78,
  76,  66, 104,  46, 111,  50,  32,   3,  39,   0,  58,  25,  92,  22,  18,  51,
  57,  65, 119, 116,  22, 109,   7,  86,  59,  93,  62, 110,  78,  99,  77,  67,
  12, 113,  87,  98, 102,   5,  88,  33,  38,  56,  23,   8,  75,  45,  13,  75,
  95,  63,  28,  49, 123, 120,  20, 112,  44,  30,  15,  98, 106,   2, 103,  29,
  82, 107,  42, 124,  24,  30,  41,  16, 108, 100, 117,  40,  73,  40,   7, 114,
  82, 115,  36, 112,  12, 102, 100,  84,  92,  48,  72,  97,   9,  54,  55,  74,
 113, 123,  17,  26,  53,  58,   4,   9,  69, 122,  21, 118,  42,  60,  27,  73,
 118, 125,  34,  15,  65, 115,  84,  64,  62,  81,  70,   1,  24, 111, 121,  83,
 104,  81,  49, 127,  48, 105,  31,  10,   6,  91,  87,  37,  16,  54, 116, 126,
  31,  38,  13,   0,  72, 106,  77,  61,  26,  67,  46,  29,  96,  37,  61,  52,
 101,  17,  44, 108,  71,  52,  66,  57,  33,  51,  25,  90,   2, 119, 122,  35,
]
TABLE_2 = [
 52,  50,  44,   6,  21,  49,  41,  59,  39,  51,  25,  32,  51,  47,  52,  43,
 37,   4,  40,  34,  61,  12,  28,   4,  58,  23,   8,  15,  12,  22,   9,  18,
 55,  10,  33,  35,  50,   1,  43,   3,  57,  13,  62,  14,   7,  42,  44,  59,
 62,  57,  27,   6,   8,  31,  26,  54,  41,  22,  45,  20,  39,   3,  16,  56,
 48,   2,  21,  28,  36,  42,  60,  33,  34,  18,   0,  11,  24,  10,  17,  61,
 29,  14,  45,  26,  55,  46,  11,  17,  54,  46,   9,  24,  30,  60,  32,   0,
 20,  38,   2,  30,  58,  35,   1,  16,  56,  40,  23,  48,  13,  19,  19,  27,
 31,  53,  47,  38,  63,  15,  49,   5,  37,  53,  25,  36,  63,  29,   5,   7,
]
TABLE_3 = [
  1,   5,  29,   6,  25,   1,  18,  23,  17,  19,   0,   9,  24,  25,   6,  31,
 28,  20,  24,  30,   4,  27,   3,  13,  15,  16,  14,  18,   4,   3,   8,   9,
 20,   0,  12,  26,  21,   8,  28,   2,  29,   2,  15,   7,  11,  22,  14,  10,
 17,  21,  12,  30,  26,  27,  16,  31,  11,   7,  13,  23,  10,   5,  22,  19,
]
TABLE_4 = [
 15,  12,  10,   4,   1,  14,  11,   7,   5,   0,  14,   7,   1,   2,  13,   8,
 10,   3,   4,   9,   6,   0,   3,   2,   5,   6,   8,   9,  11,  13,  15,  12,
]
TABLES = [TABLE_0, TABLE_1, TABLE_2, TABLE_3, TABLE_4]


def _compression_round(x, n, tbl):
    m = 4 - n
    for i in range(1 << n):
        for j in range(1 << m):
            a = j + i * (2 << m)
            b = a + (1 << m)
            y = (x[a] + (x[b] << 1)) & ((32 << m) - 1)
            z = ((x[a] << 1) + x[b]) & ((32 << m) - 1)
            x[a] = tbl[y]
            x[b] = tbl[z]


def _compression(x):
    for n in range(5):
        _compression_round(x, n, TABLES[n])


def _bits_from_bytes(x):
    bits = [0] * 128
    for i in range(128):
        if x[i >> 2] & (1 << (3 - (i & 3))):
            bits[i] = 1
    return bits


def _permutation(x, bits):
    for i in range(16, 32):
        x[i] = 0
    for i in range(128):
        x[(i >> 3) + 16] |= bits[(i * 17) & 127] << (7 - (i & 7))


def comp128v1(ki: bytes, rand: bytes):
    """Retourne (sres: 4 octets, kc: 8 octets)."""
    if len(ki) != 16 or len(rand) != 16:
        raise ValueError("ki et rand doivent faire 16 octets")
    x = bytearray(32)
    x[16:32] = rand
    for _ in range(7):
        x[0:16] = ki
        _compression(x)
        bits = _bits_from_bytes(x)
        _permutation(x, bits)
    x[0:16] = ki
    _compression(x)

    sres = bytearray(4)
    for i in range(0, 8, 2):
        sres[i >> 1] = (x[i] << 4 | x[i + 1]) & 0xFF

    kc = bytearray(8)
    for i in range(0, 12, 2):
        kc[i >> 1] = ((x[i + 18] << 6) | (x[i + 19] << 2) | (x[i + 20] >> 2)) & 0xFF
    kc[6] = ((x[30] << 6) | (x[31] << 2)) & 0xFF
    kc[7] = 0

    return bytes(sres), bytes(kc)


# ---------------------------------------------------------------------------
# Self-test: vecteurs recalcules localement avec le comp128.c de reference
# de libosmocore (compile et execute pendant le developpement de ce script).
# A relancer si vous touchez a la partie COMP128 ci-dessus.
# ---------------------------------------------------------------------------

_SELFTEST_VECTORS = [
    # (ki_hex, rand_hex, expected sres+kc hex)
    ("00112233445566778899aabbccdd0123", "00" * 16, "7b83e0e0249b485a49f12000"),
    ("00" * 16, "00" * 16, "09e55da4174757783dc40400"),
    ("00112233445566778899aabbccddffff", "00" * 16, "dacdb4d4886f74d8ccaef400"),
]


def selftest() -> bool:
    ok = True
    for ki_hex, rand_hex, expected in _SELFTEST_VECTORS:
        sres, kc = comp128v1(bytes.fromhex(ki_hex), bytes.fromhex(rand_hex))
        got = (sres + kc).hex()
        status = "OK" if got == expected else "FAIL"
        if got != expected:
            ok = False
        print(f"[{status}] ki={ki_hex} rand={rand_hex} -> {got} (attendu {expected})")
    return ok


# ---------------------------------------------------------------------------
# Toy scenario: RAND fixe a 0, Ki = prefixe connu + 2 octets inconnus.
# ---------------------------------------------------------------------------

RAND_HEX = "00" * 16                       # RAND = 0 (16 octets)
KI_PREFIX = bytes(i * 0x11 for i in range(14))  # 00 11 22 33 44 55 66 77 88 99 aa bb cc dd
DEFAULT_TABLE_PATH = "toy_table.tsv"


def rand_bytes() -> bytes:
    return bytes.fromhex(RAND_HEX)


def ki_for_suffix(suffix: bytes) -> bytes:
    if len(suffix) != 2:
        raise ValueError("le suffixe doit faire 2 octets (XX YY)")
    return KI_PREFIX + suffix


def build_table(path: str = DEFAULT_TABLE_PATH) -> int:
    """Genere la table output(sres+kc hex) -> suffixe(XXYY hex) pour les
    65536 valeurs possibles des 2 derniers octets de Ki, RAND=0.
    Remplace les tables de partitionnement (les '4 To') par une table
    triviale puisque l'espace inconnu ici est minuscule (2**16)."""
    rand = rand_bytes()
    collisions = 0
    with open(path, "w") as f:
        for suffix_int in range(0x10000):
            suffix = suffix_int.to_bytes(2, "big")
            ki = ki_for_suffix(suffix)
            sres, kc = comp128v1(ki, rand)
            f.write(f"{(sres + kc).hex()}\t{suffix.hex()}\n")
    print(f"table ecrite: {path} (65536 entrees)")
    return collisions


def load_table(path: str = DEFAULT_TABLE_PATH) -> dict:
    table = {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            out_hex, suffix_hex = line.split("\t")
            table.setdefault(out_hex, []).append(suffix_hex)
    return table


def identify(observed_hex: str, table: dict):
    """Retourne la liste des suffixes XXYY (hex) dont la sortie COMP128v1
    correspond a observed_hex (sres+kc, 24 caracteres hex), ou [] si aucun."""
    observed_hex = observed_hex.strip().lower()
    return table.get(observed_hex, [])


def _cli():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("selftest", help="verifie le portage COMP128v1 sur des vecteurs connus")

    p_build = sub.add_parser("build", help="genere la table toy (65536 entrees)")
    p_build.add_argument("-o", "--output", default=DEFAULT_TABLE_PATH)

    p_lookup = sub.add_parser("lookup", help="retrouve XXYY a partir d'une sortie observee")
    p_lookup.add_argument("observed", help="sres+kc en hexa (24 caracteres)")
    p_lookup.add_argument("-t", "--table", default=DEFAULT_TABLE_PATH)
    p_lookup.add_argument("--expect", help="suffixe XXYY attendu, pour verification")

    args = ap.parse_args()

    if args.cmd == "selftest":
        sys.exit(0 if selftest() else 1)

    elif args.cmd == "build":
        build_table(args.output)

    elif args.cmd == "lookup":
        table = load_table(args.table)
        matches = identify(args.observed, table)
        if not matches:
            print("aucune correspondance dans la table")
            sys.exit(1)
        for suffix_hex in matches:
            ki = ki_for_suffix(bytes.fromhex(suffix_hex))
            print(f"suffixe={suffix_hex} -> Ki={ki.hex()}")
        if args.expect:
            expect = args.expect.strip().lower()
            if expect in matches:
                print(f"MATCH: le banc a bien Ki=...{expect}")
            else:
                print(f"MISMATCH: attendu {expect}, trouve {matches}")
                sys.exit(1)


if __name__ == "__main__":
    _cli()
