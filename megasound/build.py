"""
build.py -- Baut megasound.exe, ein eigenstaendiges Programm ohne Python.

Die Versionsnummer steht als VERSION in megasound.py und wird von dort
gelesen. Sie erscheint im Fenster, im Kopf der erzeugten C-Dateien und in
den Dateieigenschaften der exe (Rechtsklick -> Eigenschaften -> Details).

Verwendung:
    python build.py                 # bauen mit der aktuellen Version
    python build.py --clean         # Zwischenstand von PyInstaller vorher loeschen
    python build.py --bump patch    # Version erhoehen (patch|minor|major), dann bauen
    python build.py --bump minor --no-build   # nur die Version erhoehen

Voraussetzung: pip install pyinstaller
Ergebnis:      dist/megasound.exe
"""

import argparse
import os
import re
import shutil
import subprocess
import sys

HIER      = os.path.dirname(os.path.abspath(__file__))
SKRIPT    = os.path.join(HIER, 'megasound.py')
DIST_DIR  = os.path.join(HIER, 'dist')
WORK_DIR  = os.path.join(HIER, 'build')
NAME      = 'megasound'

VERSION_MUSTER = re.compile(r"^VERSION = '(\d+)\.(\d+)\.(\d+)'(?=\r?$)", re.MULTILINE)


def abbruch(text):
    print('[build] FEHLER: ' + text, file=sys.stderr)
    sys.exit(1)


def version_lesen():
    """Liefert die Version aus megasound.py als Tupel (major, minor, patch)."""
    with open(SKRIPT, encoding='utf-8') as f:
        treffer = VERSION_MUSTER.search(f.read())
    if treffer is None:
        abbruch("In megasound.py fehlt eine Zeile der Form VERSION = 'x.y.z'.")
    return tuple(int(t) for t in treffer.groups())


def version_erhoehen(teil):
    """Erhoeht die Version in megasound.py und liefert die neue."""
    major, minor, patch = version_lesen()
    if teil == 'major':
        neu = (major + 1, 0, 0)
    elif teil == 'minor':
        neu = (major, minor + 1, 0)
    else:
        neu = (major, minor, patch + 1)

    with open(SKRIPT, encoding='utf-8', newline='') as f:
        inhalt = f.read()
    inhalt = VERSION_MUSTER.sub("VERSION = '%d.%d.%d'" % neu, inhalt, count=1)
    with open(SKRIPT, 'w', encoding='utf-8', newline='') as f:
        f.write(inhalt)
    return neu


def versionsdatei_schreiben(version):
    """Erzeugt die Versionsressource fuer die exe im Format von PyInstaller."""
    text = '%d.%d.%d' % version
    zahlen = version + (0,)
    inhalt = f"""# Von build.py erzeugt. Nicht von Hand aendern.
VSVersionInfo(
  ffi=FixedFileInfo(
    filevers={zahlen},
    prodvers={zahlen},
    mask=0x3f, flags=0x0, OS=0x40004, fileType=0x1, subtype=0x0, date=(0, 0)
  ),
  kids=[
    StringFileInfo([
      StringTable('040704b0', [
        StringStruct('CompanyName', 'HTL Rankweil'),
        StringStruct('FileDescription', 'megasound - MIDI in Tontabelle fuer die MEGACARD'),
        StringStruct('FileVersion', '{text}'),
        StringStruct('InternalName', '{NAME}'),
        StringStruct('OriginalFilename', '{NAME}.exe'),
        StringStruct('ProductName', 'MegaKit megasound'),
        StringStruct('ProductVersion', '{text}')
      ])
    ]),
    VarFileInfo([VarStruct('Translation', [0x0407, 1200])])
  ]
)
"""
    pfad = os.path.join(WORK_DIR, 'version_info.txt')
    os.makedirs(WORK_DIR, exist_ok=True)
    with open(pfad, 'w', encoding='utf-8') as f:
        f.write(inhalt)
    return pfad


def bauen(version, sauber):
    try:
        import PyInstaller  # noqa: F401
    except ImportError:
        abbruch('PyInstaller fehlt -> pip install pyinstaller')

    if sauber:
        for ordner in (WORK_DIR, DIST_DIR):
            if os.path.isdir(ordner):
                shutil.rmtree(ordner)

    ziel = os.path.join(DIST_DIR, NAME + '.exe')
    # Eine laufende exe ist gesperrt. PyInstaller meldet dann einen Fehler
    # erst am Ende; die Pruefung vorab spart den ganzen Durchlauf.
    if os.path.isfile(ziel):
        try:
            os.rename(ziel, ziel)
        except OSError:
            abbruch(ziel + ' ist gesperrt. Laeuft megasound noch?')

    befehl = [
        sys.executable, '-m', 'PyInstaller',
        '--onefile',
        '--windowed',                      # kein Konsolenfenster neben der Oberflaeche
        '--noconfirm',
        '--name', NAME,
        '--distpath', DIST_DIR,
        '--workpath', WORK_DIR,
        '--specpath', WORK_DIR,            # keine .spec-Datei im Quellordner
        '--version-file', versionsdatei_schreiben(version),
        SKRIPT,
    ]
    print('[build] megasound %d.%d.%d wird gebaut ...' % version)
    ergebnis = subprocess.run(befehl, cwd=HIER)
    if ergebnis.returncode != 0:
        abbruch('PyInstaller ist mit Code %d abgebrochen.' % ergebnis.returncode)

    if not os.path.isfile(ziel):
        abbruch('PyInstaller meldet Erfolg, aber ' + ziel + ' fehlt.')
    groesse = os.path.getsize(ziel) / 1_048_576
    print('[build] fertig: %s (%.1f MB)' % (ziel, groesse))


def main():
    parser = argparse.ArgumentParser(description='Baut megasound.exe.')
    parser.add_argument('--clean', action='store_true',
                        help='build/ und dist/ vorher loeschen')
    parser.add_argument('--bump', choices=('patch', 'minor', 'major'),
                        help='Version in megasound.py vor dem Bauen erhoehen')
    parser.add_argument('--no-build', action='store_true',
                        help='nur die Version aendern, nicht bauen')
    argumente = parser.parse_args()

    if argumente.bump:
        alt = version_lesen()
        neu = version_erhoehen(argumente.bump)
        print('[build] Version %d.%d.%d -> %d.%d.%d' % (alt + neu))
    version = version_lesen()

    if not argumente.no_build:
        bauen(version, argumente.clean)


if __name__ == '__main__':
    main()
