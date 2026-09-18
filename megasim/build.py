"""
build.py — Baut megasim.exe (Python + pygame + GCC in einer Datei).

Die mitgelieferte GCC ist gcc_minimal/, ein ausgeduennter Auszug aus WinLibs,
der versioniert wird. Neu zusammengestellt wird er nur mit --gcc-neu, dafuer
muss die vollstaendige WinLibs-GCC in winlibs/ liegen (install_gcc.ps1).

Die Versionsnummer steht als VERSION in sim.py und wird von dort gelesen. Sie
erscheint in der Konsole beim Start, im Fenstertitel, im Ladebalken und in den
Dateieigenschaften der exe.

Verwendung:
    python build.py                 # normaler Build
    python build.py --clean         # PyInstaller-Cache vorher loeschen
    python build.py --bump patch    # Version erhoehen (patch|minor|major), dann bauen
    python build.py --bump minor --no-build   # nur die Version erhoehen
    python build.py --gcc-neu       # gcc_minimal/ aus winlibs/ neu zusammenstellen
"""

import os
import re
import sys
import glob
import shutil
import zipfile
import argparse
import subprocess

ROOT     = os.path.dirname(os.path.abspath(__file__))
SIM_PY   = os.path.join(ROOT, "sim.py")
GCC_SRC  = os.path.join(ROOT, "winlibs")
GCC_TMP  = os.path.join(ROOT, "gcc_minimal")
DIST_DIR = os.path.join(ROOT, "dist_onefile")
WORK_DIR = os.path.join(ROOT, "pyinstaller_build_onefile")
GCC_ZIP  = os.path.join(WORK_DIR, "gcc.zip")
EXE      = os.path.join(DIST_DIR, "megasim.exe")

VERSION_MUSTER = re.compile(r"^VERSION = '(\d+)\.(\d+)\.(\d+)'(?=\r?$)", re.MULTILINE)

KEEP_LIBS = re.compile(
    r"libmingw32|libmingwex|libmsvcrt|libucrt|libucrtbase|libkernel32"
    r"|libgcc_s|libpthread|libwinpthread|libadvapi32|libuser32"
    r"|libshell32|libntdll|dllcrt2|crt2|mcrt1|gcrt1",
    re.IGNORECASE,
)

KEEP_DLLS = re.compile(
    r"libgcc|libwinpthread|libstdc|libzstd|libiconv|libintl|libz\.dll",
    re.IGNORECASE,
)

# Header, die ein Programm fuer die MEGACARD einbinden kann: die C-Header,
# die auch avr-libc und avr-gcc anbieten. Nur sie, die Header der
# Simulatorquellen und alles, was diese nachziehen, kommen in gcc_minimal.
# Der Rest der WinLibs-Header (Windows-SDK, DirectX, COM ...) ist auf der
# Hardware ohnehin nicht verfuegbar. Der Simulator ist damit genauso streng
# wie der Build fuer die MEGACARD.
AVR_HEADER = (
    "alloca", "assert", "ctype", "errno", "float", "inttypes", "iso646",
    "limits", "math", "setjmp", "stdalign", "stdarg", "stdbool", "stddef",
    "stdint", "stdio", "stdlib", "stdnoreturn", "string", "time",
)
FAKE_AVR = os.path.join(ROOT, "fake_avr")
FAKE_SRC = os.path.join(ROOT, "fake_src")

# Aus lib/*.a wird nur diese Bibliothek gebraucht. Daneben liegen dort unter
# anderem libpython, libstdc++ und libgfortran.
LIB_BEHALTEN = {"libgcc_s.a"}


def die(msg):
    print(f"[build] FEHLER: {msg}", file=sys.stderr)
    sys.exit(1)


def cp(src, dst_dir):
    if os.path.isfile(src):
        shutil.copy2(src, dst_dir)


def cp_tree(src, dst):
    if os.path.isdir(src):
        shutil.copytree(src, dst, dirs_exist_ok=True)


def dir_size_mb(path):
    total = sum(
        os.path.getsize(os.path.join(dp, f))
        for dp, _, files in os.walk(path)
        for f in files
    )
    return round(total / 1_048_576, 1)


def build_gcc_minimal():
    print("Minimales GCC-Subset zusammenstellen …")

    if not os.path.isfile(os.path.join(GCC_SRC, "bin", "gcc.exe")):
        die(f"WinLibs-GCC nicht gefunden: {GCC_SRC}\\bin\\gcc.exe\n"
            "  -> powershell -ExecutionPolicy Bypass -File install_gcc.ps1")

    if os.path.isdir(GCC_TMP):
        def _rm_readonly(func, path, _):
            os.chmod(path, 0o666)
            func(path)
        shutil.rmtree(GCC_TMP, onexc=_rm_readonly)

    # GCC-Version ermitteln
    ver_dir = os.path.join(GCC_SRC, "lib", "gcc", "x86_64-w64-mingw32")
    versions = [d for d in os.listdir(ver_dir)
                if os.path.isdir(os.path.join(ver_dir, d))]
    if not versions:
        die("Keine GCC-Version in lib/gcc/x86_64-w64-mingw32 gefunden.")
    ver = versions[0]
    print(f"  GCC-Version: {ver}")

    # bin/: gcc.exe, ld.exe, as.exe … + Runtime-DLLs
    bin_dst = os.path.join(GCC_TMP, "bin")
    os.makedirs(bin_dst)
    for f in ("gcc.exe", "ld.exe", "as.exe", "collect2.exe"):
        cp(os.path.join(GCC_SRC, "bin", f), bin_dst)
    for dll in glob.glob(os.path.join(GCC_SRC, "bin", "*.dll")):
        if KEEP_DLLS.search(os.path.basename(dll)):
            shutil.copy2(dll, bin_dst)

    # lib/*.a  (nur libgcc_s.a)
    lib_dst = os.path.join(GCC_TMP, "lib")
    os.makedirs(lib_dst)
    for name in LIB_BEHALTEN:
        cp(os.path.join(GCC_SRC, "lib", name), lib_dst)

    # libexec/gcc/<target>/<ver>/  (cc1.exe, collect2.exe, lto-wrapper.exe + DLLs)
    le_src = os.path.join(GCC_SRC, "libexec", "gcc", "x86_64-w64-mingw32", ver)
    le_dst = os.path.join(GCC_TMP, "libexec", "gcc", "x86_64-w64-mingw32", ver)
    if os.path.isdir(le_src):
        os.makedirs(le_dst)
        for f in ("cc1.exe", "collect2.exe", "lto-wrapper.exe"):
            cp(os.path.join(le_src, f), le_dst)
        for dll in glob.glob(os.path.join(le_src, "*.dll")):
            shutil.copy2(dll, le_dst)

    # lib/gcc/<target>/<ver>/  (crt-Objekte, libgcc.a, Compiler-Header)
    lg_src = os.path.join(GCC_SRC, "lib", "gcc", "x86_64-w64-mingw32", ver)
    lg_dst = os.path.join(GCC_TMP, "lib", "gcc", "x86_64-w64-mingw32", ver)
    os.makedirs(lg_dst)
    # libgcc_eh.a braucht nur das Linken einer exe (tests/run_tests.py),
    # nicht die DLL des Simulators
    for f in ("libgcc.a", "libgcc_eh.a", "libgcov.a", "crtbegin.o", "crtend.o",
              "crtfastmath.o", "liblto_plugin.dll"):
        cp(os.path.join(lg_src, f), lg_dst)
    cp_tree(os.path.join(lg_src, "include"),       os.path.join(lg_dst, "include"))
    cp_tree(os.path.join(lg_src, "include-fixed"), os.path.join(lg_dst, "include-fixed"))

    # x86_64-w64-mingw32/include/  (stdlib.h, string.h …), ausgeduennt unten
    cp_tree(
        os.path.join(GCC_SRC, "x86_64-w64-mingw32", "include"),
        os.path.join(GCC_TMP, "x86_64-w64-mingw32", "include"),
    )

    # x86_64-w64-mingw32/lib/  (CRT-Startup-Objekte + Import-Libs)
    x_src = os.path.join(GCC_SRC, "x86_64-w64-mingw32", "lib")
    x_dst = os.path.join(GCC_TMP, "x86_64-w64-mingw32", "lib")
    os.makedirs(x_dst)
    for pattern in ("*.a", "*.o"):
        for f in glob.glob(os.path.join(x_src, pattern)):
            base = os.path.splitext(os.path.basename(f))[0]
            if KEEP_LIBS.search(base):
                shutil.copy2(f, x_dst)

    header_ausduennen(GCC_TMP)
    print(f"  Groesse: {dir_size_mb(GCC_TMP)} MB")


def header_ermitteln(gcc_dir):
    """Liefert die Header, die AVR_HEADER und die Simulatorquellen erreichen.

    Die GCC selbst loest die Abhaengigkeiten auf: -H gibt jeden gelesenen
    Header aus, -fsyntax-only spart das Uebersetzen. Ergebnis sind Pfade
    relativ zu gcc_dir. Bricht ab, wenn ein Header fehlt."""
    gcc = os.path.join(gcc_dir, "bin", "gcc.exe")
    env = os.environ.copy()
    env["PATH"] = os.path.dirname(gcc) + os.pathsep + env.get("PATH", "")

    os.makedirs(WORK_DIR, exist_ok=True)
    probe = os.path.join(WORK_DIR, "header_probe.c")
    with open(probe, "w", encoding="utf-8") as f:
        f.write("/* Von build.py erzeugt: alle Header, die ein MEGACARD-Programm nutzen kann */\n")
        for name in AVR_HEADER:
            # Nicht jede Bibliothek hat jeden Header (MinGW etwa kein alloca.h)
            f.write(f"#if __has_include(<{name}.h>)\n#include <{name}.h>\n#endif\n")
        for name in ("avr/io.h", "avr/interrupt.h", "avr/pgmspace.h",
                     "avr/eeprom.h", "util/delay.h"):
            f.write(f"#include <{name}>\n")

    basis = os.path.normpath(gcc_dir)
    gefunden = set()
    for quelle in [probe] + sorted(glob.glob(os.path.join(FAKE_SRC, "*.c"))):
        # sim.py bindet stdint.h und stdbool.h per -include vorab ein. So
        # eingebundene Header listet -H aber nicht auf; die Probe laeuft
        # deshalb ohne und bindet beide selbst ein.
        vorab = [] if quelle == probe else ["-include", "stdint.h", "-include", "stdbool.h"]
        ergebnis = subprocess.run(
            [gcc, "-fsyntax-only", "-H", "-std=gnu11",
             "-I", FAKE_AVR, "-I", FAKE_SRC] + vorab + [quelle],
            capture_output=True, text=True, encoding="utf-8", errors="replace", env=env)
        if ergebnis.returncode != 0:
            die(f"Header fuer {os.path.basename(quelle)} fehlen in {gcc_dir}:\n"
                f"{ergebnis.stderr}\n"
                "  -> install_gcc.ps1 ausfuehren, dann python build.py --gcc-neu")
        for zeile in ergebnis.stderr.splitlines():
            if not zeile.startswith("."):
                continue
            pfad = os.path.normpath(zeile.lstrip(".").strip())
            if os.path.normcase(pfad).startswith(os.path.normcase(basis) + os.sep):
                gefunden.add(os.path.normcase(os.path.relpath(pfad, basis)))
    return gefunden


def header_ausduennen(gcc_dir):
    """Entfernt aus den Header-Ordnern alles, was header_ermitteln() nicht
    auffuehrt, auch .idl-, .c- und andere Dateien."""
    benoetigt = header_ermitteln(gcc_dir)
    ordner = [os.path.join(gcc_dir, "x86_64-w64-mingw32", "include")]
    ordner += glob.glob(os.path.join(gcc_dir, "lib", "gcc", "x86_64-w64-mingw32", "*", "include*"))

    behalten = entfernt = 0
    for wurzel in ordner:
        for pfad, _, dateien in os.walk(wurzel, topdown=False):
            for datei in dateien:
                voll = os.path.join(pfad, datei)
                if os.path.normcase(os.path.relpath(voll, gcc_dir)) in benoetigt:
                    behalten += 1
                else:
                    os.remove(voll)
                    entfernt += 1
            if not os.listdir(pfad):
                try:
                    os.rmdir(pfad)
                except OSError:
                    # OneDrive sperrt frisch geleerte Ordner mitunter kurz.
                    # Ein leerer Ordner stoert nicht, ins Archiv kommen nur Dateien.
                    pass
    print(f"  Header: {behalten} behalten, {entfernt} entfernt")

    # Gegenprobe: alles Noetige muss nach dem Ausduennen noch da sein
    header_ermitteln(gcc_dir)


def version_lesen():
    """Liefert die Version aus sim.py als Tupel (major, minor, patch)."""
    with open(SIM_PY, encoding="utf-8-sig") as f:
        treffer = VERSION_MUSTER.search(f.read())
    if treffer is None:
        die("In sim.py fehlt eine Zeile der Form VERSION = 'x.y.z'.")
    return tuple(int(t) for t in treffer.groups())


def version_erhoehen(teil):
    """Erhoeht die Version in sim.py und liefert die neue."""
    major, minor, patch = version_lesen()
    if teil == "major":
        neu = (major + 1, 0, 0)
    elif teil == "minor":
        neu = (major, minor + 1, 0)
    else:
        neu = (major, minor, patch + 1)
    # Bytes statt Text: BOM und Zeilenenden der Datei bleiben unveraendert
    with open(SIM_PY, "rb") as f:
        inhalt = f.read()
    muster = re.compile(VERSION_MUSTER.pattern.encode(), re.MULTILINE)
    inhalt = muster.sub(b"VERSION = '%d.%d.%d'" % neu, inhalt, count=1)
    with open(SIM_PY, "wb") as f:
        f.write(inhalt)
    return neu


def versionsdatei_schreiben(version):
    """Erzeugt die Versionsressource fuer die exe im Format von PyInstaller."""
    text = "%d.%d.%d" % version
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
        StringStruct('FileDescription', 'megasim - Simulator fuer die MEGACARD'),
        StringStruct('FileVersion', '{text}'),
        StringStruct('InternalName', 'megasim'),
        StringStruct('OriginalFilename', 'megasim.exe'),
        StringStruct('ProductName', 'MegaKit megasim'),
        StringStruct('ProductVersion', '{text}')
      ])
    ]),
    VarFileInfo([VarStruct('Translation', [0x0407, 1200])])
  ]
)
"""
    os.makedirs(WORK_DIR, exist_ok=True)
    pfad = os.path.join(WORK_DIR, "version_info.txt")
    with open(pfad, "w", encoding="utf-8") as f:
        f.write(inhalt)
    return pfad


def gcc_packen():
    """Packt gcc_minimal/ in ein Archiv.

    Als Einzeldateien entpackt die exe die rund 2300 Dateien bei jedem Start,
    das kostet mehrere Sekunden. Das Archiv ist dagegen eine einzige Datei;
    sim.py packt es beim ersten Start einmal nach %LOCALAPPDATA%\\megasim aus."""
    print("GCC packen …")
    os.makedirs(WORK_DIR, exist_ok=True)
    if os.path.isfile(GCC_ZIP):
        os.remove(GCC_ZIP)
    with zipfile.ZipFile(GCC_ZIP, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archiv:
        for ordner, _, dateien in os.walk(GCC_TMP):
            for datei in dateien:
                pfad = os.path.join(ordner, datei)
                archiv.write(pfad, os.path.relpath(pfad, GCC_TMP))
    print(f"  Archiv: {round(os.path.getsize(GCC_ZIP) / 1_048_576, 1)} MB")


def startbild_erzeugen():
    """Zeichnet den Startbildschirm, den die exe beim Entpacken zeigt.

    Erzeugt wird er mit der Ladeanzeige aus sim.py selbst, ohne sichtbares
    Fenster. Er gleicht damit dem ersten Bild des Simulators, der ihn nach dem
    Entpacken an derselben Stelle abloest."""
    print("Startbild zeichnen …")
    os.environ["SDL_VIDEODRIVER"] = "dummy"
    sys.path.insert(0, ROOT)
    import pygame
    import sim
    anzeige = sim.Ladeanzeige(4, "megasim")
    anzeige._zeichnen(0.0, "entpacke")
    pfad = os.path.join(WORK_DIR, "startbild.png")
    pygame.image.save(anzeige.fenster, pfad)
    pygame.quit()
    return pfad


def run_pyinstaller(version):
    print("PyInstaller …")
    cmd = [
        sys.executable, "-m", "PyInstaller",
        "--onefile",
        "--name", "megasim",
        "--distpath", DIST_DIR,
        "--workpath", WORK_DIR,
        "--specpath", WORK_DIR,        # keine .spec-Datei im Quellordner
        "--noconfirm",
        "--version-file", versionsdatei_schreiben(version),
        "--splash", startbild_erzeugen(),
        # Pfade absolut: mit --specpath gelten relative Angaben ab WORK_DIR
        "--add-data", f"{os.path.join(ROOT, 'fake_avr')};fake_avr",
        "--add-data", f"{os.path.join(ROOT, 'fake_src')};fake_src",
        "--add-data", f"{GCC_ZIP};.",
        SIM_PY,
    ]
    result = subprocess.run(cmd, cwd=ROOT)
    if result.returncode != 0:
        die("PyInstaller fehlgeschlagen.")


def exe_frei_pruefen():
    """Bricht ab, wenn die exe noch laeuft.

    Eine laufende exe ist gesperrt. PyInstaller kann sie dann nicht ersetzen,
    und je nach Ablauf bleibt unbemerkt die alte Fassung liegen."""
    if not os.path.isfile(EXE):
        return
    try:
        os.rename(EXE, EXE)
    except OSError:
        die(f"{EXE} ist gesperrt. Bitte alle Simulatorfenster schliessen.")


def main():
    parser = argparse.ArgumentParser(description="Baut megasim.exe")
    parser.add_argument("--clean", action="store_true",
                        help="PyInstaller-Cache und Spec-Datei vorher loeschen")
    parser.add_argument("--bump", choices=("patch", "minor", "major"),
                        help="Version in sim.py vor dem Bauen erhoehen")
    parser.add_argument("--no-build", action="store_true",
                        help="nur die Version aendern, nicht bauen")
    parser.add_argument("--gcc-neu", action="store_true",
                        help="gcc_minimal/ vorher aus winlibs/ neu zusammenstellen")
    args = parser.parse_args()

    if args.bump:
        alt = version_lesen()
        neu = version_erhoehen(args.bump)
        print("Version %d.%d.%d -> %d.%d.%d" % (alt + neu))
    version = version_lesen()
    if args.no_build:
        return

    exe_frei_pruefen()
    print("megasim %d.%d.%d wird gebaut" % version)

    if args.clean:
        print("Cache loeschen …")
        shutil.rmtree(WORK_DIR, ignore_errors=True)
        spec = os.path.join(ROOT, "megasim.spec")
        if os.path.isfile(spec):
            os.remove(spec)

    if args.gcc_neu:
        build_gcc_minimal()
    elif not os.path.isfile(os.path.join(GCC_TMP, "bin", "gcc.exe")):
        die(f"{GCC_TMP} fehlt oder ist unvollstaendig.\n"
            "  -> install_gcc.ps1 ausfuehren, dann python build.py --gcc-neu")
    else:
        # Braucht eine geaenderte Datei in fake_src/ einen neuen Header,
        # faellt das hier auf und nicht erst beim Schueler
        print("Header pruefen …")
        header_ermitteln(GCC_TMP)
    gcc_packen()
    run_pyinstaller(version)

    size_mb = round(os.path.getsize(EXE) / 1_048_576, 1)
    print(f"\nFertig! {EXE} ({size_mb} MB), Version %d.%d.%d" % version)
    print(f'External Tools: /k "{EXE}" "$(ProjectDir)."')


if __name__ == "__main__":
    main()
