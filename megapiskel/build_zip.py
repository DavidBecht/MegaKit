"""
build_zip.py -- Packt die Browser-Erweiterung MegaPiskel als ZIP fuer das Release.

Inhalt des ZIP, alles im Ordner MegaPiskel/:
    manifest.json             Beschreibung der Erweiterung
    <Skripte aus manifest>    nur die Dateien, die manifest.json wirklich laedt
    README.md                 vollstaendige Anleitung
    INSTALLIEREN.txt          Installationsschritte als reiner Text, erzeugt aus
                              dem Abschnitt "## Installation" der README

Vor dem Packen wird geprueft, ob manifest.json gueltiges JSON ist, eine Version
X.Y[.Z] traegt und jede dort genannte Datei existiert.

Verwendung:
    python megapiskel/build_zip.py
Ergebnis: megapiskel/dist/MegaPiskel_V<version aus manifest.json>.zip
"""

import json
import os
import re
import sys
import zipfile

HIER     = os.path.dirname(os.path.abspath(__file__))
MANIFEST = os.path.join(HIER, "manifest.json")
README   = os.path.join(HIER, "README.md")
DIST_DIR = os.path.join(HIER, "dist")
ORDNER   = "MegaPiskel"       # Ordnername im ZIP, dieser wird im Browser geladen


def abbruch(text):
    print("[megapiskel] FEHLER: " + text, file=sys.stderr)
    sys.exit(1)


def manifest_lesen():
    """Liest manifest.json und liefert (version, Liste der geladenen Dateien)."""
    try:
        with open(MANIFEST, encoding="utf-8") as f:
            m = json.load(f)
    except (OSError, ValueError) as e:
        abbruch(f"manifest.json nicht lesbar: {e}")

    version = m.get("version", "")
    if not re.fullmatch(r"\d+\.\d+(\.\d+)?", version):
        abbruch(f"Version in manifest.json ungueltig: {version!r}")

    dateien = []
    for skript in m.get("content_scripts", []):
        for art in ("js", "css"):
            for name in skript.get(art, []):
                if name not in dateien:
                    dateien.append(name)
    if not dateien:
        abbruch("manifest.json laedt keine Skripte")
    for name in dateien:
        if not os.path.isfile(os.path.join(HIER, name)):
            abbruch(f"manifest.json nennt {name}, die Datei fehlt")
    return version, dateien


def installation_als_text(version):
    """Schneidet den Abschnitt '## Installation' aus der README und entfernt
    das Markdown, damit die Datei in jedem Editor lesbar ist."""
    with open(README, encoding="utf-8") as f:
        text = f.read()
    m = re.search(r"^## Installation\n(.*?)(?=^## )", text, re.S | re.M)
    if not m:
        abbruch("README.md hat keinen Abschnitt '## Installation'")
    abschnitt = m.group(1).strip()
    abschnitt = re.sub(r"\*\*(.+?)\*\*", r"\1", abschnitt)        # fett
    abschnitt = re.sub(r"`([^`]+)`", r"\1", abschnitt)            # Code
    abschnitt = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", abschnitt) # Links

    titel = f"MegaPiskel {version}: Installation"
    return "\r\n".join([
        titel,
        "=" * len(titel),
        "",
        *abschnitt.splitlines(),
        "",
        "Benutzung, Export, Import und Fehlersuche: siehe README.md in diesem Ordner.",
        "",
    ])


def main():
    version, dateien = manifest_lesen()
    os.makedirs(DIST_DIR, exist_ok=True)
    ziel = os.path.join(DIST_DIR, f"MegaPiskel_V{version}.zip")

    with zipfile.ZipFile(ziel, "w", zipfile.ZIP_DEFLATED) as z:
        for name in ["manifest.json", *dateien, "README.md"]:
            z.write(os.path.join(HIER, name), f"{ORDNER}/{name}")
        z.writestr(f"{ORDNER}/INSTALLIEREN.txt", installation_als_text(version).encode("utf-8-sig"))

    with zipfile.ZipFile(ziel) as z:
        inhalt = z.namelist()
    print(f"[megapiskel] {os.path.relpath(ziel)}")
    for name in inhalt:
        print(f"  {name}")


if __name__ == "__main__":
    main()
