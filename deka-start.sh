#!/bin/bash
# =============================================================================
# deka-start.sh - montage des tables deka et lancement des workers.
#
# Lance par l icone deka (via pkexec), ou a la main :
#     sudo /root/deka/deka-start.sh            demarre (montages + workers)
#     sudo /root/deka/deka-start.sh --stop     arrete (workers + demontages)
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
# Sortie a l ECRAN, et copie dans le log (tee) : lance a la main comme par
# l icone, on veut VOIR ce qui se passe (montages, PID des workers), pas le
# chercher dans un fichier. Le log reste, pour retrouver un lancement passe.
exec > >(tee -a "$LOG") 2>&1

[ "$(id -u)" -eq 0 ] || { echo "root requis : sudo $0"; exit 1; }

# ── Arret (workers + demontages), factorise ──────────────────────────────────
# Utilise par '--stop' ET par le demarrage (--stop implicite, voir plus bas).
# Rejouable : ce qui est deja arrete/demonte est signale, pas une erreur.

# Tue les workers par leur CHEMIN (motif "[ /]$w", pour ne toucher qu eux) :
# TERM d abord, puis KILL de secours pour ceux qui ne repondent plus (un worker
# bloque peut tenir le GPU, cf. deadlock futex, ou garder un mount occupe).
arret_workers() {
    for w in paplon.py oclvankus.py delta_client.py; do
        if pkill -f "[ /]$w" 2>/dev/null; then echo "arrete: $w"; else echo "(pas en cours: $w)"; fi
    done
    sleep 2
    for w in paplon.py oclvankus.py delta_client.py; do
        if pkill -9 -f "[ /]$w" 2>/dev/null; then echo "force (KILL): $w"; fi
    done
}

# Arret complet : workers d abord (sinon le mount reste occupe et umount
# echoue), puis demontage des quatre tables, puis desactivation du groupe LVM
# 'tables'.
do_stop() {
    arret_workers
    for pt in /mnt /mnt1 /mnt2 /mnt3; do
        if mountpoint -q "$pt" 2>/dev/null; then
            umount "$pt" && echo "demonte: $pt" || echo "ECHEC demontage: $pt (occupe ?)"
        else
            echo "non monte: $pt"
        fi
    done
    vgchange -an tables 2>/dev/null && echo "groupe 'tables' desactive" || echo "(groupe 'tables' absent ou deja inactif)"
}

# ── --stop : l inverse exact du demarrage ────────────────────────────────────
if [ "${1:-}" = "--stop" ] || [ "${1:-}" = "stop" ]; then
    echo "=== $(date -Is) deka-stop ==="
    do_stop
    echo "deka-stop termine."
    exit 0
fi

echo "=== $(date -Is) deka-start ==="

# ── --stop implicite : on repart toujours d un etat propre ───────────────────
# PANNE VECUE : relance sans arret prealable, delta_client demarrait mal sur des
# tables encore montees (et des workers encore vivants) du lancement precedent -
# "error: failed to open /dev/indexes/lvol4", lookups dans le vide. On force donc
# un arret complet AVANT de remonter : workers tues, tables demontees, LVM
# desactive, puis tout le demarrage ci-dessous reconstruit l etat de zero.
echo "--- arret prealable (--stop implicite) ---"
do_stop
echo "--- demarrage ---"

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
    PY="$(command -v python3.7 || command -v python3 || true)"
fi
[ -n "$PY" ] || { echo "aucun python3 trouve"; exit 1; }
echo "python: $PY"

# ── 1. Activation LVM ────────────────────────────────────────────────────────
# Non fatal : si le groupe est deja actif, vgchange rend 0 et le dit ; s il
# echoue, les montages plus bas le signaleront table par table.
vgchange -ay || echo "ATTENTION: vgchange -ay a echoue (groupe absent ?)"

# ── 1bis. Rafraichissement des mappings device-mapper ────────────────────────
# PANNE VECUE : apres un reboot, l enumeration SCSI peut rebattre les lettres de
# disque (sdc <-> sdd : dmesg plein de DID_NO_CONNECT / "Synchronize Cache
# failed"). LVM relit bien ses metadonnees sur le nouveau disque, MAIS les
# tables dm DEJA ACTIVES restent figees sur l ANCIEN major:minor - qui n existe
# plus. Resultat : toute lecture de /dev/indexes/lvol* rend "Input/output
# error" (dmesg : "Buffer I/O error on dev dm-N"), alors que le disque brut lit
# parfaitement. Les lookups delta ne trouvent alors RIEN, oclvankus tourne a
# vide ("40 free slots") et aucun Kc n est jamais casse - deka "marche plus"
# sans la moindre erreur visible cote workers.
#
# 'vgchange -ay' NE corrige PAS ca : un volume deja actif n est pas reconstruit,
# donc un simple redemarrage des workers ne repare jamais rien. Il faut
# '--refresh', qui suspend/reprend chaque LV et RECHARGE sa table dm depuis les
# metadonnees courantes -> le linear target repointe sur le bon disque, quel que
# soit son nouveau nom. Non destructif, sur meme LV ouvert (mmap d un worker) ou
# systeme monte (suspend/resume). On le fait a CHAQUE demarrage : si rien n a
# bouge c est un quasi no-op, et le jour ou les disques bougent, c est repare
# tout seul.
for vg in indexes tables; do
    if vgs "$vg" >/dev/null 2>&1; then
        vgchange --refresh "$vg" >/dev/null 2>&1 \
            && echo "dm rafraichi: $vg" \
            || echo "ATTENTION: refresh dm $vg a echoue"
    fi
done

# Verification des index que delta_client ouvre REELLEMENT : lvol0..lvol7
# (delta_config.h, devpaths[8]). delta les mmap et les lit en O_DIRECT ; si l un
# manque ou est illisible, l open rend fd -1, le mmap echoue (fd -1, result
# 0xffff...) et le worker meurt a l init - ou lit dans le vide.
#
# On teste le DEBUT **ET LA FIN** de chaque volume. Un test du seul debut ne voit
# pas une table dm figee/tronquee apres rebattage SCSI : la lecture passe au
# debut mais echoue plus loin (I/O error, ou "attempt to access beyond end of
# device" cote noyau -> delta recoit un SIGBUS et meurt sans trace Python).
# lvol8 existe mais n est pas dans devpaths : on ne le teste pas.
verifier_indexes() {
    local ko=0 i dev sz last
    for i in 0 1 2 3 4 5 6 7; do
        dev=/dev/indexes/lvol$i
        if [ ! -e "$dev" ]; then
            echo "ABSENT: $dev (index exige par delta_client)"; ko=$((ko+1)); continue
        fi
        sz=$(blockdev --getsize64 "$dev" 2>/dev/null || echo 0)
        last=$(( sz/4096 - 1 ))
        if ! dd if="$dev" bs=4096 count=1 of=/dev/null >/dev/null 2>&1; then
            echo "ILLISIBLE (debut): $dev"; ko=$((ko+1))
        elif [ "$last" -gt 0 ] && ! dd if="$dev" bs=4096 skip="$last" count=1 of=/dev/null >/dev/null 2>&1; then
            echo "ILLISIBLE (fin): $dev - table dm figee/tronquee ?"; ko=$((ko+1))
        fi
    done
    return "$ko"
}

# En cas d echec, --refresh a deja ete tente plus haut ; on passe au cran du
# dessus : une reactivation COMPLETE du groupe (deactivate + activate) qui
# DETRUIT et RECONSTRUIT entierement les tables dm depuis les metadonnees
# courantes. Possible ici car le --stop implicite a deja tue les workers : aucun
# LV n est ouvert (mmap), donc le 'vgchange -an' ne sera pas refuse.
if ! verifier_indexes; then
    echo "ATTENTION: index KO - reactivation complete du groupe 'indexes'..."
    vgchange -an indexes >/dev/null 2>&1
    vgchange -ay indexes >/dev/null 2>&1
    udevadm settle 2>/dev/null || true
    if verifier_indexes; then
        echo "index repares (reactivation complete)."
    else
        echo "ECHEC: index toujours KO - delta_client va planter/lire dans le vide."
    fi
fi

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

# ── Lancement des workers (sous superviseur) ─────────────────────────────────
# Les anciens workers ont deja ete tues par le --stop implicite ; on lance les
# nouveaux. Chacun tourne sous un petit SUPERVISEUR qui le RELANCE s il meurt :
# delta_client peut recevoir un SIGBUS en plein travail ("attempt to access
# beyond end of device") quand une entree d index pointe au-dela du volume
# courant - sans superviseur, deka s arretait net sur un seul mauvais job. Le
# superviseur le repart et deka continue de cracker les jobs valides.
#
# GARDE-FOU anti-boucle : si un worker meurt 5 fois en se tenant moins de 60 s
# a chaque fois, c est casse pour de bon (index incoherent avec la taille des
# volumes, module manquant...) - on ABANDONNE en l ecrivant fort dans son log,
# plutot que de tourner en boucle a vide.
#
# -u : sortie Python NON tamponnee (le log defile en direct, pas par blocs 8 Ko).
# setsid + </dev/null : le worker survit a la fermeture de la fenetre GTK du
# lanceur (osmo-gtk-run) qui, en se fermant, tue son propre groupe de processus.
# setsid met le superviseur (et donc le worker) dans leur propre session.
# pkill -f "[ /]$w" (demarrage/--stop) les retrouve quand meme : le motif matche
# le NOM du worker precede d espace/slash, present AUSSI BIEN dans la ligne du
# python ("$PY -u /root/deka/delta_client.py") que dans celle du superviseur
# (bash -c ... delta_client.py) - les deux sont donc tues ensemble.
WPIDS=""
for w in paplon.py oclvankus.py delta_client.py; do
    [ -f "$w" ] || { echo "worker absent: $w"; continue; }
    sleep 2
    echo "lancement (supervise): $PY $DIR/$w"
    setsid bash -c '
        w="$1"; py="$2"; dir="$3"; log="$4"; n=0
        while :; do
            t0=$(date +%s)
            "$py" -u "$dir/$w" >>"$log" 2>&1 </dev/null
            rc=$?
            [ $(( $(date +%s) - t0 )) -ge 60 ] && n=0
            n=$((n+1))
            echo "[$(date -Is)] $w arrete (statut $rc) - relance #$n" >>"$log"
            if [ "$n" -ge 5 ]; then
                echo "[$(date -Is)] $w : 5 morts rapprochees, ABANDON. Si dmesg dit \"beyond end of device\" : index a regenerer pour la taille des lvol0..7." >>"$log"
                break
            fi
            sleep 2
        done
    ' deka-sup "$w" "$PY" "$DIR" "/var/log/deka-${w%.py}.log" >/dev/null 2>&1 </dev/null &
    echo "  superviseur pid $!"
    WPIDS="$WPIDS $w:$!"
done

# ── Controle de demarrage ────────────────────────────────────────────────────
# On laisse quelques secondes puis on verifie que chaque superviseur tient et
# que le worker tourne vraiment (pgrep sur le python). Un worker qui meurt
# AUSSITOT (import KO, index absent) est signale FORT : fini le "termine" affiche
# par-dessus un deka mort en silence. NB: un crash plus tardif (SIGBUS sur un
# job, 30 s+ apres) ne se voit pas ici - c est le superviseur qui le rattrape,
# et le garde-fou qui tranche si ca se repete.
sleep 6
for item in $WPIDS; do
    w=${item%%:*}; sp=${item##*:}
    if pgrep -f "[ /]$w" >/dev/null 2>&1; then
        echo "OK demarre: $w"
    else
        echo "!! $w NE TOURNE PAS (superviseur pid $sp deja a bout) :"
        tail -n 4 "/var/log/deka-${w%.py}.log" 2>/dev/null | sed 's/^/!!   /'
        dmh=$(dmesg 2>/dev/null | grep -i "beyond end of device" | tail -1)
        [ -n "$dmh" ] && echo "!!   dmesg: $dmh  => index incoherent avec la taille des volumes, a regenerer"
    fi
done

echo "deka-start termine."
