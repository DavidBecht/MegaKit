"""
release.py -- Erhoeht die Version und stoesst ein GitHub-Release an.

Ablauf:
  1. Prueft, dass das Repository releasefaehig ist: Branch main, auf
     demselben Stand wie origin/main, keine offenen Aenderungen.
  2. Bestimmt die neue Version aus dem letzten Tag vX.Y.Z.
  3. Traegt die neue Version in megasim/sim.py, megasound/megasound.py und
     megalib/megalib/megalib_version.h ein und committet das, damit sich
     Programme und Bibliothek mit der Version des Releases melden.
  4. Legt den Tag vX.Y.Z an und pusht Commit und Tag. Der Tag startet den
     Workflow .github/workflows/release.yml, der das Release baut.

Offene Dateien im Arbeitsverzeichnis:
    Das Release entsteht auf GitHub aus dem Tag, nicht aus diesem Rechner.
    Was hier nicht committet ist, kann also gar nicht darin landen.

    Nicht versionierte Dateien sind deshalb nie ein Hindernis; sie werden
    nur aufgezaehlt. Geaenderte versionierte Dateien halten das Skript an,
    weil sie meist bedeuten, dass noch etwas ins Release gehoert haette;
    mit --unsauber wird trotzdem released.

    Eine Ausnahme bleibt hart: offene Aenderungen in einer der Dateien, in
    denen die Version steht (siehe VERSIONSDATEIEN). Die schreibt und
    committet das Skript selbst, fremde Aenderungen darin liefen ungewollt
    mit.

Verwendung:
    python release.py               # 1.2.3 -> 1.2.4; beim ersten Release die
                                    # Version aus den Programmen, z.B. 1.0.0
    python release.py minor         # 1.2.3 -> 1.3.0
    python release.py major         # 1.2.3 -> 2.0.0
    python release.py 1.5.0         # feste Version
    python release.py --probe       # nur anzeigen, nichts aendern
    python release.py --unsauber    # auch mit offenen Aenderungen
"""

import argparse
import os
import re
import subprocess
import sys

WURZEL = os.path.dirname(os.path.abspath(__file__))
HAUPTZWEIG = "main"
PY_MUSTER = re.compile(rb"^VERSION = '(\d+)\.(\d+)\.(\d+)'(?=\r?$)", re.MULTILINE)
H_MUSTER = re.compile(rb'^#define MEGALIB_VERSION "(\d+)\.(\d+)\.(\d+)"(?=\r?$)',
                      re.MULTILINE)

# Jede Datei, in der die Version steht: Pfad, Muster ihrer Zeile und die
# Vorlage, die an deren Stelle tritt.
VERSIONSDATEIEN = (
    (os.path.join("megasim", "sim.py"), PY_MUSTER, b"VERSION = '%d.%d.%d'"),
    (os.path.join("megasound", "megasound.py"), PY_MUSTER, b"VERSION = '%d.%d.%d'"),
    (os.path.join("megalib", "megalib", "megalib_version.h"), H_MUSTER,
     b'#define MEGALIB_VERSION "%d.%d.%d"'),
)
TAG_MUSTER = re.compile(r"^v(\d+)\.(\d+)\.(\d+)$")


def abbruch(text):
    sys.stdout.flush()          # sonst steht die Meldung vor dem, was schon lief
    print("[release] " + text, file=sys.stderr)
    sys.exit(1)


def git(*argumente, pruefen=True, roh=False):
    """Fuehrt git aus und liefert die Ausgabe ohne umgebende Leerzeichen.

    roh=True laesst sie unveraendert. Noetig bei git status --porcelain: Dort
    steht in den ersten beiden Spalten der Zustand, und ein fuehrendes
    Leerzeichen bedeutet "nur im Arbeitsverzeichnis geaendert".
    """
    ergebnis = subprocess.run(["git"] + list(argumente), cwd=WURZEL,
                              capture_output=True, text=True, encoding="utf-8")
    if pruefen and ergebnis.returncode != 0:
        abbruch("git " + " ".join(argumente) + " fehlgeschlagen:\n" + ergebnis.stderr.strip())
    return ergebnis.stdout if roh else ergebnis.stdout.strip()


def offene_dateien():
    """git status, aufgeteilt in nicht versionierte und geaenderte Dateien."""
    unversioniert, geaendert = [], []
    for zeile in git("status", "--porcelain", roh=True).splitlines():
        (unversioniert if zeile.startswith("??") else geaendert).append(zeile)
    return unversioniert, geaendert


def sauber_pruefen(unsauber_erlaubt):
    """Bricht ab, wenn das Repository nicht releasefaehig ist."""
    unversioniert, geaendert = offene_dateien()

    # Das Release baut der Workflow aus dem Tag. Was hier nicht versioniert
    # ist, kann deshalb nicht darin landen und haelt auch nichts auf.
    if unversioniert:
        print("[release] Nicht versionierte Dateien, nicht Teil des Releases:")
        for zeile in unversioniert:
            print("          " + zeile[3:])

    if geaendert:
        # Diese Dateien schreibt und committet das Skript gleich selbst.
        # Stehen dort eigene Aenderungen, kaemen sie ungewollt mit.
        versionsdateien = {d[0].replace(os.sep, "/") for d in VERSIONSDATEIEN}
        betroffen = [z for z in geaendert if z[3:] in versionsdateien]
        if betroffen:
            abbruch("Offene Aenderungen in einer Versionsdatei; sie wuerden im "
                    "Versions-Commit mitlaufen.\nErst committen oder verwerfen:\n"
                    + "\n".join(betroffen))
        if not unsauber_erlaubt:
            abbruch("Repository ist nicht sauber. Erst committen, verwerfen oder "
                    "mit --unsauber\ntrotzdem releasen:\n" + "\n".join(geaendert))
        print("[release] --unsauber: Diese Aenderungen bleiben liegen und sind")
        print("[release] NICHT im Release, das aus dem Tag gebaut wird:")
        for zeile in geaendert:
            print(f"          {zeile[:2]}  {zeile[3:]}")

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
    """Hoechste Version, die in den Dateien steht."""
    versionen = []
    for relativ, muster, _ in VERSIONSDATEIEN:
        with open(os.path.join(WURZEL, relativ), "rb") as f:
            treffer = muster.search(f.read())
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
    """Setzt die Version in allen Dateien. Liefert die geaenderten."""
    geaendert = []
    for relativ, muster, vorlage in VERSIONSDATEIEN:
        pfad = os.path.join(WURZEL, relativ)
        # Bytes: BOM und Zeilenenden bleiben, wie sie sind
        with open(pfad, "rb") as f:
            inhalt = f.read()
        if not muster.search(inhalt):
            abbruch(f"In {relativ} fehlt die Zeile mit der Version, Muster:\n"
                    + (vorlage % (0, 0, 0)).decode())
        neu = muster.sub(vorlage % version, inhalt, count=1)
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
    parser.add_argument("--unsauber", action="store_true",
                        help="auch releasen, wenn versionierte Dateien geaendert "
                             "sind; sie bleiben liegen und sind nicht im Release")
    argumente = parser.parse_args()

    sauber_pruefen(argumente.unsauber)
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

    gesetzt = versionen_setzen(neu)
    if gesetzt:
        # --only committet genau diese Dateien, auch wenn im Index noch
        # anderes vorgemerkt ist. Vorgemerktes bleibt unberuehrt liegen.
        git("commit", "--quiet", "--only", "-m", f"Version {tag[1:]}", "--", *gesetzt)
        print("[release] Version gesetzt in: " + ", ".join(gesetzt))

    git("tag", "-a", tag, "-m", f"MegaKit {tag}")
    git("push", "--quiet", "--atomic", "origin", HAUPTZWEIG, tag)
    print(f"[release] {tag} gepusht. Der Workflow baut jetzt das Release,")
    print("[release] Fortschritt im Tab Actions des Repositorys.")


if __name__ == "__main__":
    main()
