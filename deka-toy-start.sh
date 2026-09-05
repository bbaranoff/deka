#!/bin/bash
# =============================================================================
# deka-toy-start.sh - clone de deka-start.sh : seul le dernier worker change
# (toy-delta-client.py au lieu de delta_client.py), et crack_toy.py build est
# lance avant les workers pour que toy_table.tsv existe.
#
# Lance par l icone deka toy (via pkexec), ou a la main :
#     sudo /root/deka/deka-toy-start.sh            demarre (montages + workers)
#     sudo /root/deka/deka-toy-start.sh --stop     arrete (workers + demontages)
#
# Ce que fait ce script, dans l ordre qui compte :
#   1. vgchange -ay : active le groupe de volumes, sans quoi /dev/tables/*
#      n existe pas et les montages echouent.
#   2. les quatre points de montage, puis les quatre volumes deka
#      (1_10, 11_20, 21_30, 31_40) - un par tranche de tables.
#   3. crack_toy.py build : genere toy_table.tsv s il manque (65536 entrees,
#      RAND=0, Ki connu sauf 2 octets - remplace les tables de 4 To).
#   4. les trois workers python, depuis le repertoire du script : paplon.py,
#      oclvankus.py, puis toy-delta-client.py (au lieu de delta_client.py).
# =============================================================================
set -u

DIR="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
LOG=/var/log/deka-toy.log
# Sortie a l ECRAN, et copie dans le log (tee) : lance a la main comme par
# l icone, on veut VOIR ce qui se passe (montages, PID des workers), pas le
# chercher dans un fichier. Le log reste, pour retrouver un lancement passe.
exec > >(tee -a "$LOG") 2>&1

[ "$(id -u)" -eq 0 ] || { echo "root requis : sudo $0"; exit 1; }

# ── --stop : l inverse exact du demarrage ────────────────────────────────────
# Tue les workers (par leur CHEMIN, pour ne toucher qu eux), demonte les quatre
# tables, puis desactive le groupe LVM 'tables'. Rejouable : ce qui est deja
# arrete/demonte est signale, pas une erreur.
if [ "${1:-}" = "--stop" ] || [ "${1:-}" = "stop" ]; then
    echo "=== $(date -Is) deka-toy-stop ==="
    for w in paplon.py oclvankus.py toy-delta-client.py; do
        if pkill -f "$DIR/$w" 2>/dev/null; then echo "arrete: $w"; else echo "(pas en cours: $w)"; fi
    done
    for pt in /mnt /mnt1 /mnt2 /mnt3; do
        if mountpoint -q "$pt" 2>/dev/null; then
            umount "$pt" && echo "demonte: $pt" || echo "ECHEC demontage: $pt (occupe ?)"
        else
            echo "non monte: $pt"
        fi
    done
    vgchange -an tables 2>/dev/null && echo "groupe 'tables' desactive" || echo "(groupe 'tables' absent ou deja inactif)"
    echo "deka-toy-stop termine."
    exit 0
fi

echo "=== $(date -Is) deka-toy-start ==="

# Le venv /root/.env porte pyopencl (et gnuradio/grgsm) : c est LUI qu il faut,
# pas le python systeme. Sous pkexec/sudo le PATH est nettoye, /root/.env/bin n y
# est jamais - on ne compte donc pas sur "activate" pour trouver l interpreteur,
# on le nomme par son chemin. L activation reste utile pour les binaires du venv
# (grgsm_*, uhd_*) qu un worker lancerait en sous-processus.
VENV=/root/.env
if [ -x "$VENV/bin/python3" ]; then
    # shellcheck disable=SC1091
    source "$VENV/bin/activate"
    PY="$VENV/bin/python3"
else
    echo "ATTENTION: venv $VENV absent - repli sur le python systeme (pas de pyopencl)"
    PY="$(command -v python3.7 || command -v python3)"
fi
[ -n "$PY" ] || { echo "aucun python3 trouve"; exit 1; }
echo "python: $PY"

# ── 1. Activation LVM ────────────────────────────────────────────────────────
# Non fatal : si le groupe est deja actif, vgchange rend 0 et le dit ; s il
# echoue, les montages plus bas le signaleront table par table.
vgchange -ay || echo "ATTENTION: vgchange -ay a echoue (groupe absent ?)"

# ── 2. Points de montage puis volumes ────────────────────────────────────────
# /mnt existe deja sur tout systeme ; on cree les trois autres. Chaque montage
# est saute si le point est deja monte - le script est rejouable a volonte.
mkdir -p /mnt1 /mnt2 /mnt3

monter() {
    local dev="$1" pt="$2"
    if mountpoint -q "$pt"; then
        echo "deja monte: $pt"
        return 0
    fi
    if [ ! -e "$dev" ]; then
        echo "ABSENT: $dev - non monte sur $pt"
        return 1
    fi
    mount "$dev" "$pt" && echo "monte: $dev -> $pt" \
        || echo "ECHEC montage: $dev -> $pt"
}

monter /dev/tables/1_10  /mnt
monter /dev/tables/11_20 /mnt1
monter /dev/tables/21_30 /mnt2
monter /dev/tables/31_40 /mnt3

# ── 3. Les workers ───────────────────────────────────────────────────────────
# Lances en arriere-plan depuis le repertoire du script (import delta /
# delta.pyc). Leur sortie va dans un log par worker, pas dans celui-ci.
cd "$DIR" || exit 1

# Le kernel OpenCL (slice.c) est genere depuis genkernel32.sh : oclvankus.py le
# lit au demarrage (cl.Program(... slice.c)). On le (re)genere ici pour qu il
# soit toujours a jour avant de lancer le worker.
if [ -f genkernel32.sh ]; then
    if bash genkernel32.sh > slice.c; then
        echo "genere: slice.c ($(wc -l < slice.c) lignes)"
    else
        echo "ECHEC generation slice.c (genkernel32.sh)"
    fi
else
    echo "genkernel32.sh absent - slice.c non regenere"
fi

# crack_toy.py build : toy_table.tsv doit exister AVANT toy-delta-client.py
# (RAND=0, Ki connu sauf 2 octets - remplace les tables de 4 To). Rejouable :
# saute si deja construite.
if [ -f crack_toy.py ]; then
    if [ -f toy_table.tsv ]; then
        echo "deja construite: toy_table.tsv"
    elif "$PY" crack_toy.py build; then
        echo "construite: toy_table.tsv"
    else
        echo "ECHEC construction toy_table.tsv (crack_toy.py build)"
    fi
else
    echo "crack_toy.py absent - toy_table.tsv non construite"
fi

# ── Nettoyage des anciens workers ────────────────────────────────────────────
# Un worker d un lancement precedent peut trainer (voire etre bloque et tenir le
# GPU, cf. deadlock futex). On les tue par leur CHEMIN (pour ne toucher qu eux),
# TERM d abord, puis KILL de secours pour ceux qui ne repondent plus.
for w in paplon.py oclvankus.py toy-delta-client.py; do
    if pkill -f "$DIR/$w" 2>/dev/null; then echo "ancien arrete: $w"; fi
done
sleep 2
for w in paplon.py oclvankus.py toy-delta-client.py; do
    if pkill -9 -f "$DIR/$w" 2>/dev/null; then echo "ancien force (KILL): $w"; fi
done

for w in paplon.py oclvankus.py toy-delta-client.py; do
    [ -f "$w" ] || { echo "worker absent: $w"; continue; }
    sleep 2
    echo "lancement: $PY $DIR/$w"
    # -u : sortie NON tamponnee. Sans lui, Python ecrivant dans un fichier (et
    # non un terminal) bufferise par blocs de ~8 Ko : le log semble fige alors
    # que le worker tourne. Avec -u, les "free slots" defilent en direct, comme
    # avec "python oclvankus.py" dans un terminal (tail -f /var/log/deka-*.log).
    "$PY" -u "$DIR/$w" >>"/var/log/deka-${w%.py}.log" 2>&1 &
    echo "  pid $!"
done

echo "deka-toy-start termine."
