"""
build_template.py -- Baut das Projekt-Template fuer Microchip Studio.

Das Template enthaelt die Bibliothek megalib/megalib/ und die Startdatei
template/main.c. Die Projektdatei entsteht aus megalib/MegaLib.cproj: die
Compiler- und Linkereinstellungen werden uebernommen, Name und GUID durch
Template-Parameter ersetzt, die Dateiliste neu erzeugt. Die Demos kommen
nicht mit.

Installation in Microchip Studio: die ZIP-Datei unveraendert nach
    Dokumente\\Atmel Studio\\7.0\\Templates\\ProjectTemplates\\
kopieren. Das Template erscheint dann unter Datei -> Neu -> Projekt.

Verwendung:
    python build_template.py                    # Version "dev"
    python build_template.py --version 1.2.0
    python build_template.py --pruefen          # zusaetzlich mit avr-gcc uebersetzen
Ergebnis: template/dist/Template_HTL_Rankweil_MegaLib_V<version>.zip
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile

HIER      = os.path.dirname(os.path.abspath(__file__))
WURZEL    = os.path.dirname(HIER)
PROJEKT   = os.path.join(WURZEL, "megalib")
BIBLIO    = os.path.join(PROJEKT, "megalib")
CPROJ     = os.path.join(PROJEKT, "MegaLib.cproj")
MAIN_C    = os.path.join(HIER, "main.c")
ICON      = os.path.join(HIER, "__TemplateIcon.ico")
DIST_DIR  = os.path.join(HIER, "dist")

NS = "http://schemas.microsoft.com/developer/msbuild/2003"
VS_NS = "http://schemas.microsoft.com/developer/vstemplate/2005"
QUELLENDUNGEN = (".c", ".h")

# Einstellung, die in jeder Konfiguration vorhanden sein muss
F_CPU = "F_CPU=12000000UL"


def abbruch(text):
    print("[template] FEHLER: " + text, file=sys.stderr)
    sys.exit(1)


def bibliotheksdateien():
    """Alle .c- und .h-Dateien der Bibliothek, relativ zum Projektordner."""
    dateien = []
    for ordner, unterordner, namen in os.walk(BIBLIO):
        unterordner[:] = sorted(d for d in unterordner if d != "__pycache__")
        for name in sorted(namen):
            if name.endswith(QUELLENDUNGEN):
                dateien.append(os.path.relpath(os.path.join(ordner, name), PROJEKT))
    if not dateien:
        abbruch("Keine Quelldateien in " + BIBLIO)
    return dateien


def _q(tag):
    return "{%s}%s" % (NS, tag)


def projektdatei(dateien):
    """Erzeugt die .cproj des Templates aus MegaLib.cproj."""
    ET.register_namespace("", NS)
    baum = ET.parse(CPROJ)
    wurzel = baum.getroot()

    # Name und GUID setzt Microchip Studio beim Anlegen des Projekts ein
    for gruppe in wurzel.findall(_q("PropertyGroup")):
        for tag, wert in (("AssemblyName", "$safeprojectname$"),
                          ("Name", "$safeprojectname$"),
                          ("RootNamespace", "$safeprojectname$"),
                          ("ProjectGuid", "{$guid1$}")):
            element = gruppe.find(_q(tag))
            if element is not None:
                element.text = wert

    for avrgcc in wurzel.iter(_q("AvrGcc")):
        # Symbole: doppelte Eintraege entfernen, F_CPU in jeder Konfiguration
        symbole = avrgcc.find(_q("avrgcc.compiler.symbols.DefSymbols"))
        if symbole is not None:
            liste = symbole.find(_q("ListValues"))
            werte = []
            for v in list(liste):
                if v.text not in werte:
                    werte.append(v.text)
                liste.remove(v)
            if F_CPU not in werte:
                werte.insert(0, F_CPU)
            for w in werte:
                ET.SubElement(liste, _q("Value")).text = w
        # Absolute Suchpfade eines fremden Rechners haben im Template nichts verloren
        for element in list(avrgcc):
            if element.tag == _q("avrgcc.linker.libraries.LibrarySearchPaths"):
                avrgcc.remove(element)

    # Dateiliste neu aufbauen
    for gruppe in wurzel.findall(_q("ItemGroup")):
        wurzel.remove(gruppe)
    import_element = wurzel.find(_q("Import"))
    position = list(wurzel).index(import_element)

    kompilieren = ET.Element(_q("ItemGroup"))
    for pfad in ["main.c"] + dateien:
        eintrag = ET.SubElement(kompilieren, _q("Compile"), Include=pfad.replace("/", "\\"))
        ET.SubElement(eintrag, _q("SubType")).text = "compile"
    ordner = ET.Element(_q("ItemGroup"))
    for name in sorted({os.path.dirname(p) for p in dateien} | _elternordner(dateien)):
        ET.SubElement(ordner, _q("Folder"), Include=name.replace("/", "\\"))
    wurzel.insert(position, kompilieren)
    wurzel.insert(position + 1, ordner)

    ET.indent(baum, space="  ")
    return ET.tostring(wurzel, encoding="unicode", xml_declaration=False)


def _elternordner(dateien):
    ergebnis = set()
    for p in dateien:
        teil = os.path.dirname(p)
        while teil:
            ergebnis.add(teil)
            teil = os.path.dirname(teil)
    return ergebnis


def vorlagenbeschreibung(dateien, version):
    """Erzeugt MyTemplate.vstemplate mit der Ordnerstruktur der Bibliothek."""
    baum = {}
    for p in dateien:
        knoten = baum
        teile = p.replace("\\", "/").split("/")
        for t in teile[:-1]:
            knoten = knoten.setdefault(t, {})
        knoten.setdefault(None, []).append(teile[-1])

    def ausgeben(knoten, einrueckung):
        zeilen = []
        for name in sorted(k for k in knoten if k is not None):
            zeilen.append(f'{einrueckung}<Folder Name="{name}" TargetFolderName="{name}">')
            zeilen += ausgeben(knoten[name], einrueckung + "  ")
            zeilen.append(f"{einrueckung}</Folder>")
        for datei in knoten.get(None, []):
            # Bibliotheksdateien unveraendert uebernehmen
            zeilen.append(f'{einrueckung}<ProjectItem ReplaceParameters="false" '
                          f'TargetFileName="{datei}">{datei}</ProjectItem>')
        return zeilen

    inhalt = "\n".join(ausgeben(baum, "      "))
    return f"""<VSTemplate Version="3.0.0" xmlns="{VS_NS}" Type="Project">
  <TemplateData>
    <Name>MegaLib {version} (HTL Rankweil)</Name>
    <Description>MEGACARD V6.11 mit megalib {version}: Anzeige, Zeichnen, Sprites, Ton.</Description>
    <ProjectType>CandCPP</ProjectType>
    <ProjectSubType>
    </ProjectSubType>
    <SortOrder>1000</SortOrder>
    <CreateNewFolder>true</CreateNewFolder>
    <DefaultName>MegaLibProjekt</DefaultName>
    <ProvideDefaultName>true</ProvideDefaultName>
    <LocationField>Enabled</LocationField>
    <EnableLocationBrowseButton>true</EnableLocationBrowseButton>
    <Icon>__TemplateIcon.ico</Icon>
  </TemplateData>
  <TemplateContent>
    <Project TargetFileName="$safeprojectname$.cproj" File="MegaLib.cproj" ReplaceParameters="true">
{inhalt}
      <ProjectItem ReplaceParameters="true" TargetFileName="main.c">main.c</ProjectItem>
    </Project>
  </TemplateContent>
</VSTemplate>
"""


def pruefen(zip_pfad):
    """Packt das Template aus und uebersetzt es mit avr-gcc fuer den ATmega16.

    Ersetzt die Template-Parameter wie Microchip Studio und baut mit denselben
    wesentlichen Schaltern wie das Studio-Projekt. Scheitert der Build, ist
    das Template unbrauchbar."""
    avr_gcc = shutil.which("avr-gcc")
    if avr_gcc is None:
        abbruch("avr-gcc nicht im PATH, --pruefen nicht moeglich.")
    with tempfile.TemporaryDirectory() as ziel:
        with zipfile.ZipFile(zip_pfad) as z:
            z.extractall(ziel)
        haupt = os.path.join(ziel, "main.c")
        with open(haupt, encoding="utf-8") as f:
            text = f.read()
        for parameter, wert in (("$projectname$", "Pruefung"),
                                ("$username$", "CI"), ("$time$", "-")):
            text = text.replace(parameter, wert)
        with open(haupt, "w", encoding="utf-8") as f:
            f.write(text)

        quellen = [os.path.join(o, n) for o, _, ns in os.walk(ziel)
                   for n in ns if n.endswith(".c")]
        elf = os.path.join(ziel, "pruefung.elf")
        befehl = [avr_gcc, "-mmcu=atmega16", "-DF_CPU=12000000UL", "-Os",
                  "-std=gnu99", "-funsigned-char", "-funsigned-bitfields",
                  "-fpack-struct", "-fshort-enums", "-ffunction-sections",
                  "-fdata-sections", "-Wall", "-Wl,--gc-sections",
                  "-o", elf] + quellen + ["-lm"]
        ergebnis = subprocess.run(befehl, capture_output=True, text=True)
        if ergebnis.returncode != 0:
            abbruch("Template baut nicht:\n" + ergebnis.stderr)
        if ergebnis.stderr.strip():
            print("[template] Warnungen:\n" + ergebnis.stderr)
        groesse = shutil.which("avr-size")
        if groesse:
            print(subprocess.run([groesse, elf], capture_output=True, text=True).stdout)
        print("[template] Pruefbuild mit avr-gcc erfolgreich.")


def main():
    parser = argparse.ArgumentParser(description="Baut das Microchip-Studio-Template.")
    parser.add_argument("--version", default="dev", help="Versionsnummer, z.B. 1.2.0")
    parser.add_argument("--pruefen", action="store_true",
                        help="Template danach mit avr-gcc uebersetzen")
    argumente = parser.parse_args()

    version = argumente.version.lstrip("vV")
    dateien = bibliotheksdateien()
    os.makedirs(DIST_DIR, exist_ok=True)
    ziel = os.path.join(DIST_DIR, f"Template_HTL_Rankweil_MegaLib_V{version}.zip")

    with zipfile.ZipFile(ziel, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("MyTemplate.vstemplate", vorlagenbeschreibung(dateien, version))
        # Studio liest die Projektdatei als UTF-8 mit BOM, wie es sie selbst schreibt
        z.writestr("MegaLib.cproj", "﻿<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                   + projektdatei(dateien))
        z.write(MAIN_C, "main.c")
        z.write(ICON, "__TemplateIcon.ico")
        for pfad in dateien:
            z.write(os.path.join(PROJEKT, pfad), pfad.replace("\\", "/"))

    print(f"[template] {ziel} ({len(dateien) + 1} Quelldateien)")
    if argumente.pruefen:
        pruefen(ziel)


if __name__ == "__main__":
    main()
