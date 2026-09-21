"""
build_docs.py -- baut die Dokumentationsseite der megalib.

Die Seite entsteht vollstaendig aus dem Repository:

    Header der megalib   ->  Beschreibung jeder Funktion   (kopf_lesen.py)
    docs/inhalt.py       ->  Reiter, Abschnitte, Einleitungen
    docs/beispiele/*.c   ->  Beispielprogramme
    docs/bilder/*.png    ->  deren Bild auf dem Display (aus dem Simulator)

Verwendung:

    python build_docs.py                  Seite nach docs/site/ bauen
    python build_docs.py --version 1.0.2  mit Versionsangabe im Kopf
    python build_docs.py --bilder         zuerst alle Beispielbilder neu erzeugen
    python build_docs.py --pruefen        jedes Beispiel mit avr-gcc uebersetzen

--bilder braucht Windows und den Simulator, --pruefen braucht avr-gcc. Die
Bilder liegen fertig im Repository, damit der Workflow auf GitHub die Seite
ohne beides bauen kann.
"""

import argparse
import glob
import html
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

import inhalt
import kopf_lesen
from vorlage import SEITE, CSS, JS

HIER = os.path.dirname(os.path.abspath(__file__))
MEGAKIT = os.path.normpath(os.path.join(HIER, ".."))
MEGALIB = os.path.join(MEGAKIT, "megalib", "megalib")
MEGASIM = os.path.join(MEGAKIT, "megasim")
BEISPIELE = os.path.join(HIER, "beispiele")
BILDER = os.path.join(HIER, "bilder")
SITE = os.path.join(HIER, "site")

REPO = "https://github.com/DavidBecht/MegaKit"
SKALIERUNG = 4                  # 128x64 Pixel werden zu 512x256

AVR_GCC_KANDIDATEN = (
    r"C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin",
    r"C:\Program Files\Microchip\xc8\v2.36\avr\bin",
)
AVR_FLAGS = ["-mmcu=atmega16", "-DF_CPU=12000000UL", "-Os", "-std=gnu99",
             "-funsigned-char", "-funsigned-bitfields", "-fpack-struct",
             "-fshort-enums", "-ffunction-sections", "-fdata-sections",
             "-Wall", "-Werror", "-Wl,--gc-sections"]


def abbruch(text):
    sys.exit("[docs] " + text)


def werkzeug(name, verzeichnisse):
    pfad = shutil.which(name)
    if pfad:
        return pfad
    for ordner in verzeichnisse:
        kandidat = os.path.join(ordner, name + ".exe")
        if os.path.isfile(kandidat):
            return kandidat
    return None


# ===========================================================================
# Einlesen
# ===========================================================================

def eintraege_lesen():
    """Alle dokumentierten Eintraege der Header, nach Namen."""
    nach_name = {}
    for reiter_id, header in inhalt.HEADER.items():
        pfad = os.path.join(MEGAKIT, "megalib", *header.split("/"))
        if not os.path.isfile(pfad):
            abbruch("Header nicht gefunden: " + pfad)
        for eintrag in kopf_lesen.header_lesen(pfad):
            eintrag["reiter"] = reiter_id
            eintrag["header"] = header
            if eintrag["name"] in nach_name:
                abbruch(f"{eintrag['name']} steht in zwei Headern.")
            nach_name[eintrag["name"]] = eintrag
    return nach_name


KOPF = re.compile(r"^//\s*(beispiel|titel|bild)\s*:\s*(.*?)\s*$")
E_DRUCK = re.compile(r"^S([0-3])\s+bei\s+([\d.]+)\s*s$")
E_HALTEN = re.compile(r"^S([0-3])\s+halten\s+([\d.]+)\s*-\s*([\d.]+)\s*s$")
E_POTI = re.compile(r"^poti=(\d+)$")


def beispiele_lesen():
    """Liest docs/beispiele/*.c mit ihrem Kopf.

    Der Kopf steht in den ersten Zeilen der Datei:

        // beispiel: display_draw_rect     Eintrag oder Abschnitt, wo es hingehoert
        // titel: Rahmen und Fuellung
        // bild: 1.2s                      Zeitpunkt der Aufnahme, ohne = kein Bild
        // bild: 2.0s, S0 bei 0.8s         zusaetzlich Taster druecken oder Poti stellen
    """
    beispiele = {}
    for pfad in sorted(glob.glob(os.path.join(BEISPIELE, "*.c"))):
        zeilen = open(pfad, encoding="utf-8").read().splitlines()
        kopf, rest = {}, []
        for i, zeile in enumerate(zeilen):
            treffer = KOPF.match(zeile)
            if treffer and not rest:
                kopf[treffer.group(1)] = treffer.group(2)
            else:
                rest.append(zeile)
        name = os.path.splitext(os.path.basename(pfad))[0]
        if "beispiel" not in kopf:
            abbruch(f"{name}.c: Zeile '// beispiel: <eintrag>' fehlt.")

        beispiel = {"id": name, "gehoert_zu": kopf["beispiel"],
                    "titel": kopf.get("titel", ""), "zeit": None,
                    "ereignisse": [], "datei": os.path.basename(pfad),
                    "code": "\n".join(rest).strip("\n")}
        if "bild" in kopf:
            teile = [t.strip() for t in kopf["bild"].split(",") if t.strip()]
            zeit = re.match(r"^([\d.]+)\s*s$", teile[0])
            if not zeit:
                abbruch(f"{name}.c: '// bild:' beginnt mit der Zeit, z.B. 1.5s")
            beispiel["zeit"] = float(zeit.group(1))
            for teil in teile[1:]:
                if E_DRUCK.match(teil):
                    n, t = E_DRUCK.match(teil).groups()
                    beispiel["ereignisse"] += [(float(t), "druck", int(n)),
                                               (float(t) + 0.12, "los", int(n))]
                elif E_HALTEN.match(teil):
                    n, t1, t2 = E_HALTEN.match(teil).groups()
                    beispiel["ereignisse"] += [(float(t1), "druck", int(n)),
                                               (float(t2), "los", int(n))]
                elif E_POTI.match(teil):
                    beispiel["ereignisse"].append((0.0, "poti",
                                                   int(E_POTI.match(teil).group(1))))
                else:
                    abbruch(f"{name}.c: unbekannter Zusatz '{teil}'")
            beispiel["ereignisse"].sort()
        beispiele.setdefault(kopf["beispiel"], []).append(beispiel)
    return beispiele


def gliederung_pruefen(eintraege, beispiele):
    """Jeder Eintrag der Header muss genau einmal in inhalt.py stehen."""
    verwendet, doppelt = set(), []
    for reiter in inhalt.REITER:
        for abschnitt in reiter["abschnitte"]:
            for name in abschnitt["eintraege"]:
                if name not in eintraege:
                    abbruch(f"inhalt.py nennt '{name}', das steht in keinem Header.")
                if name in verwendet:
                    doppelt.append(name)
                verwendet.add(name)
                if eintraege[name]["reiter"] != reiter["id"]:
                    abbruch(f"'{name}' steht im Reiter '{reiter['id']}', gehoert aber "
                            f"zu {eintraege[name]['header']}.")
    fehlend = sorted(set(eintraege) - verwendet)
    if fehlend:
        abbruch("Diese Eintraege der Header fehlen in inhalt.py:\n  "
                + "\n  ".join(f"{n}  ({eintraege[n]['header']})" for n in fehlend))
    if doppelt:
        abbruch("Doppelt in inhalt.py: " + ", ".join(doppelt))

    orte = {a["id"] for r in inhalt.REITER for a in r["abschnitte"]} | set(eintraege)
    for ziel, liste in beispiele.items():
        if ziel not in orte:
            abbruch(f"{liste[0]['datei']}: '{ziel}' ist weder Eintrag noch Abschnitt.")


# ===========================================================================
# C-Code einfaerben
# ===========================================================================

SCHLUESSEL = {
    "if", "else", "for", "while", "do", "switch", "case", "default", "break",
    "continue", "return", "goto", "sizeof", "typedef", "struct", "enum", "union",
    "static", "const", "volatile", "extern", "inline", "void", "int", "char",
    "long", "short", "unsigned", "signed", "float", "double", "bool", "true",
    "false", "NULL",
}
TYPEN = {
    "uint8_t", "uint16_t", "uint32_t", "int8_t", "int16_t", "int32_t", "size_t",
    "PGM_P", "FontSize_t", "BITMAP_T", "SPRITE_T", "SPRITE_FPS_T", "TON_T",
    "MELODIE_T", "SPRITE_INTERN_T", "PROGMEM",
}

TOKEN = re.compile(r"""
    (?P<kommentar>//[^\n]*|/\*.*?\*/)
  | (?P<text>"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*')
  | (?P<praeprozessor>\#[A-Za-z_]+)
  | (?P<zahl>\b0[xXbB][0-9a-fA-F]+\b|\b\d+[uUlLfF]*\b)
  | (?P<wort>[A-Za-z_]\w*)
""", re.S | re.X)


def einfaerben(code, namen=(), verlinken=False):
    """Faerbt C-Code ein. Bekannte megalib-Namen werden verlinkt."""
    teile, letzte = [], 0
    for treffer in TOKEN.finditer(code):
        teile.append(html.escape(code[letzte:treffer.start()]))
        letzte = treffer.end()
        art = treffer.lastgroup
        wort = treffer.group()
        sicher = html.escape(wort)
        if art == "wort":
            if wort in SCHLUESSEL:
                teile.append(f'<span class="k">{sicher}</span>')
            elif wort in TYPEN:
                teile.append(f'<span class="t">{sicher}</span>')
            elif wort in namen:
                if verlinken:
                    teile.append(f'<a class="l" href="#{wort}">{sicher}</a>')
                else:
                    teile.append(f'<span class="l">{sicher}</span>')
            else:
                teile.append(sicher)
        else:
            kurz = {"kommentar": "c", "text": "s",
                    "praeprozessor": "p", "zahl": "n"}[art]
            teile.append(f'<span class="{kurz}">{sicher}</span>')
    teile.append(html.escape(code[letzte:]))
    return "".join(teile)


# ===========================================================================
# HTML
# ===========================================================================

WORT = re.compile(r"[A-Za-z_]\w*(?:\(\))?")
EXTRA_CODE = {"PSTR", "PROGMEM", "NULL"}


def _codeartig(wort, namen):
    """Sieht das Wort im Fliesstext nach Code aus?"""
    if wort.endswith("()") or wort in EXTRA_CODE:
        return True
    if wort in namen:
        return True
    return bool(re.fullmatch(r"[A-Z][A-Z0-9_]{3,}", wort)) and "_" in wort


def text_mit_code(text, namen):
    """Setzt foo() und GROSSE_KONSTANTEN im Fliesstext in <code>.

    Bekannte Namen der Bibliothek werden zusaetzlich auf ihren Eintrag
    verlinkt. Ersetzt wird vor dem Maskieren, damit nichts in den
    HTML-Entitaeten zerschnitten wird.
    """
    teile, letzte = [], 0
    for treffer in WORT.finditer(text):
        wort = treffer.group()
        if not _codeartig(wort, namen):
            continue
        teile.append(html.escape(text[letzte:treffer.start()]))
        letzte = treffer.end()
        ziel = wort[:-2] if wort.endswith("()") else wort
        marke = f"<code>{html.escape(wort)}</code>"
        teile.append(f'<a class="quer" href="#{ziel}">{marke}</a>'
                     if ziel in namen else marke)
    teile.append(html.escape(text[letzte:]))
    return "".join(teile)


def bloecke_html(bloecke, namen):
    stuecke = []
    for art, text in bloecke:
        if art == "p":
            stuecke.append(f"<p>{text_mit_code(text, namen)}</p>")
        else:
            stuecke.append(f'<pre class="code">{einfaerben(text, namen, True)}</pre>')
    return "\n".join(stuecke)


def beispiel_html(beispiel, namen):
    bild = ""
    if beispiel["zeit"] is not None:
        quelle = f"bilder/{beispiel['id']}.png"
        if not os.path.isfile(os.path.join(BILDER, beispiel["id"] + ".png")):
            print(f"[docs] Hinweis: Bild fehlt fuer {beispiel['id']} "
                  f"(python build_docs.py --bilder)")
        else:
            bild = (f'<figure class="schirm"><img src="{quelle}" alt="Display-Ausgabe '
                    f'des Beispiels" width="512" height="256" loading="lazy">'
                    f'<figcaption>So sieht es auf dem Display aus</figcaption></figure>')
    titel = html.escape(beispiel["titel"] or "Beispiel")
    return (f'<div class="beispiel">'
            f'<div class="beispiel-kopf">{titel}</div>'
            f'<div class="beispiel-inhalt">'
            f'<pre class="code">{einfaerben(beispiel["code"], namen, True)}</pre>'
            f'{bild}</div></div>')


ART_NAME = {"funktion": "Funktion", "makro": "Makro",
            "konstante": "Konstante", "typ": "Typ"}


def eintrag_html(eintrag, namen, beispiele):
    name = eintrag["name"]
    teile = [f'<article class="eintrag" id="{name}" data-name="{name}" '
             f'data-kurz="{html.escape(eintrag["brief"], quote=True)}">']
    teile.append(
        f'<h3><span class="art art-{eintrag["art"]}">{ART_NAME[eintrag["art"]]}</span>'
        f'<span class="eintrag-name">{name}</span>'
        f'<a class="anker" href="#{name}" title="Link zu diesem Eintrag">#</a></h3>')
    teile.append(f'<pre class="code sig">{einfaerben(eintrag["signatur"], namen)}</pre>')
    if eintrag["brief"]:
        teile.append(f'<p class="brief">{text_mit_code(eintrag["brief"], namen)}</p>')
    if eintrag["text"]:
        teile.append(f'<div class="text">{bloecke_html(eintrag["text"], namen)}</div>')

    if eintrag["params"]:
        zeilen = "".join(
            f'<tr><td><code>{html.escape(p)}</code></td>'
            f'<td>{text_mit_code(t, namen)}</td></tr>'
            for p, t in eintrag["params"])
        ueberschrift = "Argumente" if eintrag["art"] == "makro" else "Parameter"
        teile.append(f'<table class="params"><caption>{ueberschrift}</caption>'
                     f'{zeilen}</table>')

    if eintrag["felder"]:
        zeilen, gruppe = [], None
        sichtbar = [f for f in eintrag["felder"] if not f.get("intern")]
        for feld in sichtbar:
            if feld["gruppe"] != gruppe:
                gruppe = feld["gruppe"]
                if gruppe:
                    zeilen.append(f'<tr class="gruppe"><td colspan="2">'
                                  f'{html.escape(gruppe)}</td></tr>')
            zeilen.append(
                f'<tr><td><code>{html.escape(feld["deklaration"])}</code></td>'
                f'<td>{text_mit_code(feld["text"], namen) if feld["text"] else ""}</td></tr>')
        teile.append(f'<table class="params felder"><caption>Felder</caption>'
                     f'{"".join(zeilen)}</table>')

    if eintrag["return"]:
        teile.append(f'<p class="rueckgabe"><span class="etikett">Rueckgabe</span>'
                     f'{text_mit_code(eintrag["return"], namen)}</p>')
    for text in eintrag["notes"]:
        teile.append(f'<div class="hinweis note">{text_mit_code(text, namen)}</div>')
    for text in eintrag["warnings"]:
        teile.append(f'<div class="hinweis warnung">{text_mit_code(text, namen)}</div>')

    for beispiel in beispiele.get(name, []):
        teile.append(beispiel_html(beispiel, namen))

    quelle = f'{REPO}/blob/main/megalib/{eintrag["header"]}#L{eintrag["zeile"]}'
    teile.append(f'<div class="quelle"><a href="{quelle}" target="_blank" '
                 f'rel="noopener">{html.escape(eintrag["header"])}'
                 f'<span class="zeile">Zeile {eintrag["zeile"]}</span></a></div>')
    teile.append("</article>")
    return "\n".join(teile)


def seite_bauen(eintraege, beispiele, version):
    namen = set(eintraege)
    reiter_knoepfe, inhalte, leisten = [], [], []

    for reiter in inhalt.REITER:
        rid = reiter["id"]
        reiter_knoepfe.append(
            f'<button class="reiter-knopf" data-reiter="{rid}" '
            f'role="tab">{reiter["titel"]}</button>')

        abschnitte_html, leiste = [], [f'<div class="leiste" data-reiter="{rid}">']
        for abschnitt in reiter["abschnitte"]:
            aid = abschnitt["id"]
            leiste.append(f'<a class="leiste-abschnitt" href="#{aid}">'
                          f'{html.escape(abschnitt["titel"])}</a>')
            eintraege_html = []
            for name in abschnitt["eintraege"]:
                leiste.append(f'<a class="leiste-eintrag" href="#{name}">{name}</a>')
                eintraege_html.append(eintrag_html(eintraege[name], namen, beispiele))
            for beispiel in beispiele.get(aid, []):
                eintraege_html.insert(0, beispiel_html(beispiel, namen))
            abschnitte_html.append(
                f'<section class="abschnitt" id="{aid}">'
                f'<h2>{html.escape(abschnitt["titel"])}</h2>'
                f'<p class="abschnitt-text">{html.escape(abschnitt["text"])}</p>'
                f'{"".join(eintraege_html)}</section>')
        leiste.append("</div>")
        leisten.append("\n".join(leiste))

        inhalte.append(
            f'<div class="reiter-inhalt" data-reiter="{rid}" role="tabpanel">'
            f'<header class="reiter-kopf"><h1>{html.escape(reiter["titel"])}</h1>'
            f'<p class="untertitel">{reiter["untertitel"]}</p></header>'
            f'<div class="einleitung">{reiter["einleitung"]}</div>'
            f'{"".join(abschnitte_html)}</div>')

    verzeichnis = json.dumps(
        [{"n": e["name"], "k": e["brief"], "r": e["reiter"], "a": e["art"]}
         for e in eintraege.values()], ensure_ascii=False)

    return SEITE.format(css=CSS, js=JS, version=html.escape(version), repo=REPO,
                        knoepfe="\n".join(reiter_knoepfe),
                        leisten="\n".join(leisten),
                        inhalte="\n".join(inhalte),
                        verzeichnis=verzeichnis)


# ===========================================================================
# Bilder aus dem Simulator
# ===========================================================================

def ein_bild(beispiel):
    """Uebersetzt ein Beispiel, laesst es laufen und speichert sein Display.

    Laeuft in einem eigenen Prozess: Jedes Programm bringt seine eigenen
    globalen Variablen in der DLL mit und laesst sich danach nicht sauber
    zuruecksetzen.
    """
    os.environ["SDL_VIDEODRIVER"] = "dummy"
    sys.path.insert(0, MEGASIM)
    import threading
    import time
    import pygame
    import sim

    basis = tempfile.mkdtemp(prefix="megadocs_")
    projekt = os.path.join(basis, "p")
    os.makedirs(projekt)
    shutil.copytree(MEGALIB, os.path.join(projekt, "megalib"),
                    ignore=shutil.ignore_patterns("*.mid", "Debug", "Release"))
    with open(os.path.join(projekt, "main.c"), "w", encoding="utf-8") as f:
        f.write(beispiel["code"] + "\n")
    sim.GCC = os.path.join(MEGASIM, "gcc_minimal", "bin", "gcc.exe")

    class Stumm:                       # Ladeanzeige ohne Fenster
        def zeigen(self, *a, **k): pass
        def bereich(self, *a): pass
        def fehler(self, zeilen): sys.exit("  Uebersetzen fehlgeschlagen")

    dll = os.path.join(basis, "bild.dll")
    sim.build(projekt, dll, Stumm())
    (lib, fb, pina, ddra, porta, portc, ddrc, ocr1a, tccr1b,
     rect_fn, *_) = sim.load_dll(dll)

    stop = threading.Event()
    frame = threading.Event()
    threading.Thread(target=lib.main, daemon=True).start()
    threading.Thread(target=sim.isr_thread,
                     args=(lib, ocr1a, tccr1b, 30, frame, stop), daemon=True).start()
    sim.timer8_threads_starten(lib, stop)
    threading.Thread(target=sim.adc_thread, args=(lib, stop), daemon=True).start()

    pygame.init()
    start = time.perf_counter()
    for zeit, art, wert in beispiel["ereignisse"] + [(beispiel["zeit"], "bild", 0)]:
        while time.perf_counter() - start < zeit:
            time.sleep(0.002)
        if art == "druck":
            pina.value &= ~(1 << wert)
        elif art == "los":
            pina.value |= (1 << wert)
        elif art == "poti":
            sim.poti_setzen(lib, wert)
        else:
            flaeche = pygame.Surface((sim.OLED_W * SKALIERUNG,
                                      sim.OLED_H * SKALIERUNG))
            sim.render_oled(flaeche, fb, SKALIERUNG, rect_fn())
            os.makedirs(BILDER, exist_ok=True)
            pfad = os.path.join(BILDER, beispiel["id"] + ".png")
            pygame.image.save(flaeche, pfad)
            print(f"  {os.path.basename(pfad)}")

    stop.set()
    shutil.rmtree(basis, ignore_errors=True)
    os._exit(0)                        # Faeden im Hintergrund nicht abwarten


def bilder_erzeugen(beispiele, auswahl):
    fehler = 0
    anzahl = 0
    for liste in beispiele.values():
        for beispiel in liste:
            if beispiel["zeit"] is None:
                continue
            if auswahl and not any(beispiel["id"].startswith(a) for a in auswahl):
                continue
            anzahl += 1
            print(f"{beispiel['datei']}: Bild bei {beispiel['zeit']} s")
            ergebnis = subprocess.run(
                [sys.executable, os.path.abspath(__file__),
                 "--einzelbild", json.dumps(beispiel)], cwd=HIER)
            fehler += ergebnis.returncode != 0
    print(f"\n{anzahl} Bilder, {fehler} mit Fehlern.")
    return fehler


# ===========================================================================
# Beispiele uebersetzen
# ===========================================================================

def beispiele_pruefen(beispiele):
    """Uebersetzt jedes Beispiel mit avr-gcc fuer den ATmega16."""
    avr_gcc = werkzeug("avr-gcc", AVR_GCC_KANDIDATEN)
    avr_size = werkzeug("avr-size", AVR_GCC_KANDIDATEN)
    if not avr_gcc:
        abbruch("avr-gcc nicht gefunden, --pruefen nicht moeglich.")

    basis = tempfile.mkdtemp(prefix="megadocs_pruefung_")
    fehler = 0
    alle = sorted((b for liste in beispiele.values() for b in liste),
                  key=lambda b: b["id"])
    for beispiel in alle:
        projekt = os.path.join(basis, beispiel["id"])
        os.makedirs(projekt)
        shutil.copytree(MEGALIB, os.path.join(projekt, "megalib"),
                        ignore=shutil.ignore_patterns("*.mid"))
        with open(os.path.join(projekt, "main.c"), "w", encoding="utf-8") as f:
            f.write(beispiel["code"] + "\n")
        quellen = [os.path.join(o, n) for o, _, ns in os.walk(projekt)
                   for n in ns if n.endswith(".c")]
        elf = os.path.join(projekt, "beispiel.elf")
        ergebnis = subprocess.run([avr_gcc] + AVR_FLAGS + ["-o", elf] + quellen + ["-lm"],
                                  capture_output=True, text=True)
        if ergebnis.returncode != 0:
            print(f"{beispiel['datei']:34s} FEHLER\n{ergebnis.stderr}")
            fehler += 1
            continue
        groesse = ""
        if avr_size:
            ausgabe = subprocess.run([avr_size, elf], capture_output=True,
                                     text=True).stdout.splitlines()
            if len(ausgabe) > 1:
                text, data, bss = (int(x) for x in ausgabe[1].split()[:3])
                groesse = f"Flash {text + data:6d} B, RAM {data + bss:4d} B"
        print(f"{beispiel['datei']:34s} ok   {groesse}")
    shutil.rmtree(basis, ignore_errors=True)
    print(f"\n{len(alle)} Beispiele, {fehler} mit Fehlern.")
    return fehler


# ===========================================================================
# Hauptprogramm
# ===========================================================================

def main():
    parser = argparse.ArgumentParser(description="Baut die Doku-Seite der megalib.")
    parser.add_argument("--version", default="Entwicklungsstand",
                        help="Versionsangabe im Kopf der Seite")
    parser.add_argument("--bilder", nargs="*", metavar="BEISPIEL",
                        help="Bilder neu erzeugen (ohne Angabe: alle)")
    parser.add_argument("--pruefen", action="store_true",
                        help="jedes Beispiel mit avr-gcc uebersetzen")
    parser.add_argument("--einzelbild", help=argparse.SUPPRESS)
    argumente = parser.parse_args()

    if argumente.einzelbild:
        ein_bild(json.loads(argumente.einzelbild))
        return

    eintraege = eintraege_lesen()
    beispiele = beispiele_lesen()
    gliederung_pruefen(eintraege, beispiele)

    if argumente.bilder is not None:
        if bilder_erzeugen(beispiele, argumente.bilder):
            sys.exit(1)
    if argumente.pruefen:
        if beispiele_pruefen(beispiele):
            sys.exit(1)

    os.makedirs(SITE, exist_ok=True)
    seite = seite_bauen(eintraege, beispiele, argumente.version)
    with open(os.path.join(SITE, "index.html"), "w", encoding="utf-8") as f:
        f.write(seite)
    if os.path.isdir(BILDER):
        # dirs_exist_ok statt vorher loeschen: Auf einem OneDrive-Ordner
        # schlaegt das Entfernen gelegentlich fehl.
        shutil.copytree(BILDER, os.path.join(SITE, "bilder"), dirs_exist_ok=True)
    with open(os.path.join(SITE, ".nojekyll"), "w") as f:
        f.write("")

    anzahl_beispiele = sum(len(l) for l in beispiele.values())
    print(f"[docs] {len(eintraege)} Eintraege, {anzahl_beispiele} Beispiele "
          f"-> {os.path.join(SITE, 'index.html')}")


if __name__ == "__main__":
    main()
