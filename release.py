"""
release.py -- Erhoeht die Version und stoesst ein GitHub-Release an.

Ablauf:
  1. Prueft, dass das Repository sauber ist: keine geaenderten oder neuen
     Dateien, Branch main, auf demselben Stand wie origin/main.
  2. Bestimmt die neue Version aus dem letzten Tag vX.Y.Z.
  3. Setzt VERSION in megasim/sim.py und megasound/megasound.py auf die neue
     Version und committet das, damit sich die Programme mit der Version des
     Releases melden.
  4. Legt den Tag vX.Y.Z an und pusht Commit und Tag. Der Tag startet den
     Workflow .github/workflows/release.yml, der das Release baut.

Verwendung:
    python release.py               # 1.2.3 -> 1.2.4; beim ersten Release die
                                    # Version aus den Programmen, z.B. 1.0.0
    python release.py minor         # 1.2.3 -> 1.3.0
    python release.py major         # 1.2.3 -> 2.0.0
    python release.py 1.5.0         # feste Version
    python release.py --probe       # nur anzeigen, nichts aendern
"""

import argparse
import os
import re
import subprocess
import sys

WURZEL = os.path.dirname(os.path.abspath(__file__))
HAUPTZWEIG = "main"
VERSIONSDATEIEN = (
    os.path.join("megasim", "sim.py"),
    os.path.join("megasound", "megasound.py"),
)
VERSION_MUSTER = re.compile(rb"^VERSION = '(\d+)\.(\d+)\.(\d+)'(?=\r?$)", re.MULTILINE)
TAG_MUSTER = re.compile(r"^v(\d+)\.(\d+)\.(\d+)$")


def abbruch(text):
    print("[release] " + text, file=sys.stderr)
    sys.exit(1)


def git(*argumente, pruefen=True):
    ergebnis = subprocess.run(["git"] + list(argumente), cwd=WURZEL,
                              capture_output=True, text=True, encoding="utf-8")
    if pruefen and ergebnis.returncode != 0:
        abbruch("git " + " ".join(argumente) + " fehlgeschlagen:\n" + ergebnis.stderr.strip())
    return ergebnis.stdout.strip()


def sauber_pruefen():
    """Bricht ab, wenn das Repository nicht releasefaehig ist."""
    offen = git("status", "--porcelain")
    if offen:
        abbruch("Repository ist nicht sauber. Erst committen oder verwerfen:\n" + offen)

    zweig = git("rev-parse", "--abbrev-ref", "HEAD")
    if zweig != HAUPTZWEIG:
        abbruch(f"Release nur von {HAUPTZWEIG} aus, aktuell: {zweig}")

    git("fetch", "--quiet", "--tags", "origin")
    lokal = git("rev-parse", "HEAD")
    fern = git("rev-parse", f"origin/{HAUPTZWEIG}")
    if lokal != fern:
        basis = git("merge-base", "HEAD", f"origin/{HAUPTZWEIG}")
        if basis == lokal:
            abbruch(f"origin/{HAUPTZWEIG} ist weiter. Erst git pull.")
        if basis == fern:
            abbruch(f"Lokale Commits sind noch nicht auf origin. Erst git push.")
        abbruch(f"{HAUPTZWEIG} und origin/{HAUPTZWEIG} sind auseinandergelaufen.")


def letzte_version():
    """Hoechster Tag vX.Y.Z, oder None, wenn es noch keinen gibt."""
    versionen = []
    for tag in git("tag", "--list", "v*").splitlines():
        treffer = TAG_MUSTER.match(tag.strip())
        if treffer:
            versionen.append(tuple(int(t) for t in treffer.groups()))
    return max(versionen, default=None)


def dateiversion():
    """Hoechste VERSION aus den Programmen."""
    versionen = []
    for relativ in VERSIONSDATEIEN:
        with open(os.path.join(WURZEL, relativ), "rb") as f:
            treffer = VERSION_MUSTER.search(f.read())
        if treffer:
            versionen.append(tuple(int(t) for t in treffer.groups()))
    return max(versionen, default=(0, 0, 0))


def neue_version(alt, angabe):
    major, minor, patch = alt
    if angabe == "major":
        return (major + 1, 0, 0)
    if angabe == "minor":
        return (major, minor + 1, 0)
    if angabe == "patch":
        return (major, minor, patch + 1)
    treffer = re.fullmatch(r"v?(\d+)\.(\d+)\.(\d+)", angabe)
    if not treffer:
        abbruch(f"Unbekannte Angabe '{angabe}'. Erlaubt: patch, minor, major oder X.Y.Z")
    neu = tuple(int(t) for t in treffer.groups())
    if neu <= alt:
        abbruch("Version %d.%d.%d ist nicht groesser als die letzte, %d.%d.%d." % (neu + alt))
    return neu


def versionen_setzen(version):
    """Setzt VERSION in den Programmen. Liefert die geaenderten Dateien."""
    text = b"VERSION = '%d.%d.%d'" % version
    geaendert = []
    for relativ in VERSIONSDATEIEN:
        pfad = os.path.join(WURZEL, relativ)
        # Bytes: BOM und Zeilenenden bleiben, wie sie sind
        with open(pfad, "rb") as f:
            inhalt = f.read()
        if not VERSION_MUSTER.search(inhalt):
            abbruch(f"In {relativ} fehlt eine Zeile VERSION = 'x.y.z'.")
        neu = VERSION_MUSTER.sub(text, inhalt, count=1)
        if neu != inhalt:
            with open(pfad, "wb") as f:
                f.write(neu)
            geaendert.append(relativ)
    return geaendert


def main():
    parser = argparse.ArgumentParser(description="Version erhoehen und Release anstossen.")
    parser.add_argument("angabe", nargs="?", default=None,
                        help="patch (Standard), minor, major oder X.Y.Z")
    parser.add_argument("--probe", action="store_true",
                        help="nur pruefen und anzeigen, nichts aendern")
    argumente = parser.parse_args()

    sauber_pruefen()
    alt = letzte_version()
    if alt is None:
        # Erstes Release: ohne Angabe gilt die Version, die in den
        # Programmen steht. patch/minor/major zaehlen von dort weiter,
        # eine feste Version gilt, wie sie ist.
        print("[release] Erstes Release, noch kein Tag vorhanden.")
        if argumente.angabe is None:
            neu = dateiversion()
        elif argumente.angabe in ("patch", "minor", "major"):
            neu = neue_version(dateiversion(), argumente.angabe)
        else:
            neu = neue_version((0, 0, 0), argumente.angabe)
        alt = (0, 0, 0)
    else:
        neu = neue_version(alt, argumente.angabe or "patch")
    tag = "v%d.%d.%d" % neu

    if git("tag", "--list", tag):
        abbruch(f"Tag {tag} gibt es schon.")

    print("[release] %d.%d.%d -> %d.%d.%d" % (alt + neu))
    if argumente.probe:
        print("[release] --probe: nichts geaendert.")
        return

    geaendert = versionen_setzen(neu)
    if geaendert:
        git("add", *geaendert)
        git("commit", "--quiet", "-m", f"Version {tag[1:]}")
        print("[release] Version gesetzt in: " + ", ".join(geaendert))

    git("tag", "-a", tag, "-m", f"MegaKit {tag}")
    git("push", "--quiet", "--atomic", "origin", HAUPTZWEIG, tag)
    print(f"[release] {tag} gepusht. Der Workflow baut jetzt das Release,")
    print("[release] Fortschritt im Tab Actions des Repositorys.")


if __name__ == "__main__":
    main()
