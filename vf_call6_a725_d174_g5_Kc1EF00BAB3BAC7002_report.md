# Compte-rendu A5/1 crack — `crack_all.py`

## Méthode

**Principe :** A5/1 est un chiffrement par flot. En exploitant du plaintext connu
(SI5/SI6 diffusés en clair sur le BCCH, ou bourrage LAPDm), on récupère le keystream
via `chiffré XOR plaintext`. Kraken/deka retrouvent alors l'état interne A5/1
(le Kc) à partir d'un segment de keystream >=64 bits sans erreur. `find_kc` valide
le Kc en testant sur une 2e trame.

**Étapes :**

1. Auto-détection du sample-rate (sonde BCCH_SDCCH4/t0)
2. Extraction des bursts chiffrés (sans clé)
3. Dérivation du plaintext (SI en clair, TA=0)
4. Groupement en blocs de 4 trames (XOR -> keystreams)
5. Soumission à deka/Kraken
6. Vérification des états candidats avec `find_kc`

_Voir `METHODE.md` pour les détails complets._

## Résultats

| Paramètre | Valeur |
|---|---|
| **Source** | `vf_call6_a725_d174_g5_Kc1EF00BAB3BAC7002.cfile` |
| **Date/Heure** | 2026-09-02 15:01:10 |
| **Durée** | 39.5 s |
| **ARFCN** | 514 |
| **Sample-rate détecté** | 574712 Hz |
| **Mode/TS détectés** | SDCCH8 / 1 |
| **Mode/TS effectif** | SDCCH8 / 1 |
| **SI dérivé** | si5 FN 862242 |
|  |  |
|  ├─ si5 (avant TA=0) | `0001030349061d9f6d1810800000000000000000000000` |
|  └─ si5 (TA=0) | `0000030349061d9f6d1810800000000000000000000000` |
| **Plaintext(s)** | si5 |
| **Blocs crackés** | 4 |
| **Keystreams** | 16 |
| **Candidats trouvés (deka)** | 0 |
| | |
| **Kc non trouvé** | |
| Raison | 4 blocs essayés, aucun match find_kc |
| **Conseil** | Augmenter `--maxblocks` ou essayer un autre plaintext |
| **Log détaillé** | `crack_all.log` |
