#!/bin/bash
# =============================================================================
# deka-start.sh - montage des tables deka et lancement des workers.
#
# Lance par l icone deka (via pkexec), ou a la main :
#     sudo /root/deka/deka-start.sh
#
# Ce que fait ce script, dans l ordre qui compte :
#   1. vgchange -ay : active le groupe de volumes, sans quoi /dev/tables/*
#      n existe pas et les montages echouent.
#   2. les quatre points de montage, puis les quatre volumes deka
#      (1_10, 11_20, 21_30, 31_40) - un par tranche de tables.
#   3. les trois workers python, depuis le repertoire du script (delta.pyc y vit).
# =============================================================================
set -u

DIR="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
LOG=/var/log/deka.log
exec >>"$LOG" 2>&1
echo "=== $(date -Is) deka-start ==="

[ "$(id -u)" -eq 0 ] || { echo "root requis : sudo $0"; exit 1; }

# python3.7 est le shebang des workers ; on le prend s il est la, python3 sinon.
PY="$(command -v python3.7 || command -v python3)"
[ -n "$PY" ] || { echo "aucun python3 trouve"; exit 1; }

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
for w in paplon.py oclvankus.py delta_client.py; do
    [ -f "$w" ] || { echo "worker absent: $w"; continue; }
    echo "lancement: $PY $DIR/$w"
    "$PY" "$DIR/$w" >>"/var/log/deka-${w%.py}.log" 2>&1 &
    echo "  pid $!"
done

echo "deka-start termine."
