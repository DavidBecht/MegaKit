"""Baut und startet die Prueffaelle fuer megalib auf dem PC.

Aufruf:
    python tests/run_tests.py [pfad/zum/projekt]

Standardmaessig wird ../megalib geprueft. Die Bibliothek wird dabei mit
denselben nachgebauten AVR-Headern uebersetzt, die auch der Simulator benutzt,
es braucht also keinen AVR-Compiler und keine Hardware.

Der Rueckgabewert ist die Zahl der fehlgeschlagenen Pruefungen, damit sich das
Skript in einen automatischen Ablauf einhaengen laesst.
"""
import os
import subprocess
import sys

HIER = os.path.dirname(os.path.abspath(__file__))
SIM = os.path.dirname(HIER)

# Dieselbe Suche wie in sim.py: diese beiden werden durch Nachbauten ersetzt.
ERSETZT = ("twi_soft.c", "random.c")


def gcc_finden():
    for pfad in (os.environ.get("MEGACARD_GCC"),
                 r"C:\msys64\mingw64\bin\gcc.exe",
                 os.path.join(SIM, "gcc_minimal", "bin", "gcc.exe")):
        if pfad and os.path.isfile(pfad):
            return pfad
    sys.exit("[test] Kein gcc gefunden. MEGACARD_GCC setzen oder MSYS2 installieren.")


def quellen_sammeln(wurzel):
    dateien = []
    for ordner, _, namen in os.walk(wurzel):
        for n in sorted(namen):
            if n.endswith(".c") and n not in ERSETZT:
                dateien.append(os.path.join(ordner, n))
    return dateien


def include_pfade(wurzel):
    pfade = set()
    for ordner, _, namen in os.walk(wurzel):
        if any(n.endswith(".h") for n in namen):
            pfade.add(ordner)
    return sorted(pfade)


def main():
    projekt = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                              else os.path.join(SIM, "..", "megalib"))
    lib = os.path.join(projekt, "megalib")
    if not os.path.isdir(lib):
        sys.exit("[test] Kein megalib-Ordner in " + projekt)

    gcc = gcc_finden()
    exe = os.path.join(HIER, "megalib_test.exe")

    befehl = [gcc, "-o", exe,
              "-I" + os.path.join(SIM, "fake_avr"),
              "-I" + os.path.join(SIM, "fake_src"),
              "-I" + projekt]
    befehl += ["-I" + p for p in include_pfade(lib)]
    befehl += ["-DF_CPU=12000000UL", "-DNDEBUG", "-std=gnu11",
               "-Wall", "-Wextra", "-Wno-unused-parameter",
               "-include", "stdint.h", "-include", "stdbool.h"]
    befehl += [os.path.join(HIER, "megalib_test.c"), os.path.join(HIER, "main.c")]
    befehl += quellen_sammeln(lib)
    befehl += [os.path.join(SIM, "fake_src", n)
               for n in ("sim_api.c", "twi_fake.c", "fake_random.c")]

    umgebung = os.environ.copy()
    umgebung["PATH"] = os.path.dirname(gcc) + os.pathsep + umgebung.get("PATH", "")

    print("[test] uebersetze ...")
    bau = subprocess.run(befehl, capture_output=True, text=True,
                         encoding="utf-8", errors="replace", env=umgebung)
    if bau.stderr.strip():
        print(bau.stderr)
    if bau.returncode != 0:
        sys.exit("[test] Uebersetzen fehlgeschlagen.")

    print("[test] starte ...")
    lauf = subprocess.run([exe], env=umgebung)
    return lauf.returncode


if __name__ == "__main__":
    sys.exit(main())
