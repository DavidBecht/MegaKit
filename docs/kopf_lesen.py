"""
kopf_lesen.py -- liest die Header der megalib und liefert die Doku als Daten.

Die Bibliothek ist ihre eigene Dokumentation: Ueber jeder Funktion steht ein
Kommentarblock mit @brief, @param und den Hinweisen. Dieses Modul zerlegt die
Header in einzelne Eintraege, build_docs.py macht daraus die Webseite. Damit
kann die Seite nicht veralten, und eine neue Funktion ohne Kommentar faellt
sofort auf.

Erkannt werden:

    /** ... */  vor einer Funktionsdeklaration   -> kind "funktion"
                vor einem #define                -> kind "makro" oder "konstante"
                vor typedef enum/struct          -> kind "typ"

Aus dem Kommentarblock werden @brief, der Fliesstext, eingerueckte
Codebeispiele, @param, @return, @note und @warning herausgeloest.
"""

import os
import re

# --- Der Kommentarblock ----------------------------------------------------

TAG = re.compile(r"^@(brief|param|return|returns|note|warning|see)\b\s*(.*)$")


def _block_saeubern(rohtext):
    """Macht aus /** ... */ eine Liste von Zeilen ohne Rahmen."""
    zeilen = []
    for zeile in rohtext.splitlines():
        zeile = zeile.rstrip()
        zeile = re.sub(r"^\s*/\*\*+", "", zeile)
        zeile = re.sub(r"\*+/\s*$", "", zeile)
        zeile = re.sub(r"^\s*\*( ?)", "", zeile)
        zeilen.append(zeile)
    while zeilen and not zeilen[0].strip():
        zeilen.pop(0)
    while zeilen and not zeilen[-1].strip():
        zeilen.pop()
    return zeilen


def _absaetze(zeilen):
    """Teilt Fliesstext in Absaetze und eingerueckte Codebeispiele."""
    bloecke, absatz, code = [], [], []

    def absatz_schliessen():
        if absatz:
            bloecke.append(("p", " ".join(absatz)))
            absatz.clear()

    def code_schliessen():
        if code:
            while code and not code[0].strip():
                code.pop(0)
            while code and not code[-1].strip():
                code.pop()
            tiefe = min((len(z) - len(z.lstrip()) for z in code if z.strip()),
                        default=0)
            bloecke.append(("code", "\n".join(z[tiefe:] for z in code)))
            code.clear()

    for zeile in zeilen:
        if zeile.startswith("    ") and zeile.strip():
            absatz_schliessen()
            code.append(zeile)
        elif not zeile.strip():
            if code:
                code.append(zeile)      # Leerzeile im Code erst einmal behalten
            else:
                absatz_schliessen()
        else:
            code_schliessen()
            absatz.append(zeile.strip())
    code_schliessen()
    absatz_schliessen()
    return bloecke


def _doku_zerlegen(rohtext):
    """Zerlegt einen Kommentarblock in seine Bestandteile."""
    doku = {"brief": "", "text": [], "params": [], "return": "",
            "notes": [], "warnings": [], "see": []}
    zeilen = _block_saeubern(rohtext)

    # Jeder Abschnitt sammelt seine Fortsetzungszeilen ein, bis der naechste
    # @-Tag oder eine Leerzeile kommt. Nach einer Leerzeile geht es wieder mit
    # Fliesstext weiter: So bleibt @brief eine Kurzfassung, auch wenn danach
    # noch Absaetze folgen.
    abschnitte = [("text", [])]
    for zeile in zeilen:
        treffer = TAG.match(zeile.strip())
        if treffer:
            abschnitte.append((treffer.group(1), [treffer.group(2)]))
        elif not zeile.strip() and abschnitte[-1][0] != "text":
            abschnitte.append(("text", []))
        else:
            abschnitte[-1][1].append(zeile)

    text_zeilen = []
    for art, inhalt in abschnitte:
        text = " ".join(z.strip() for z in inhalt if z.strip())
        if art == "text":
            if text_zeilen and inhalt:
                text_zeilen.append("")
            text_zeilen += inhalt
        elif art == "brief":
            doku["brief"] = text
        elif art == "param":
            teile = text.split(None, 1)
            if teile:
                doku["params"].append((teile[0], teile[1] if len(teile) > 1 else ""))
        elif art in ("return", "returns"):
            doku["return"] = text
        elif art == "note":
            doku["notes"].append(text)
        elif art == "warning":
            doku["warnings"].append(text)
        elif art == "see":
            doku["see"].append(text)
    doku["text"] = _absaetze(text_zeilen)

    # Steht kein @brief da, dient der erste Absatz als Kurzfassung.
    if not doku["brief"] and doku["text"] and doku["text"][0][0] == "p":
        doku["brief"] = doku["text"].pop(0)[1]
    return doku


# --- Die Deklaration dahinter ----------------------------------------------

def _name_der_deklaration(code):
    """Bezeichner, unter dem ein Eintrag gefuehrt wird."""
    if code.startswith("#define"):
        treffer = re.match(r"#define\s+(\w+)", code)
        return treffer.group(1) if treffer else ""
    if code.startswith("typedef") or code.startswith("struct"):
        # typedef enum {...} SPRITE_FPS_T;  /  struct SPRITE_T_tag {...};
        treffer = re.search(r"}\s*(\w+)\s*;\s*$", code)
        if treffer:
            return treffer.group(1)
        treffer = re.match(r"struct\s+(\w+)_tag", code)
        return treffer.group(1) if treffer else ""
    # Funktion: der Bezeichner vor der Klammer der Parameterliste
    ohne_attribut = re.sub(r"__attribute__\s*\(\(.*?\)\)", "", code, flags=re.S)
    treffer = re.search(r"(\w+)\s*\(", ohne_attribut)
    return treffer.group(1) if treffer else ""


def _art(code):
    if code.startswith("#define"):
        return "makro" if re.match(r"#define\s+\w+\(", code) else "konstante"
    if code.startswith("typedef") or code.startswith("struct"):
        return "typ"
    return "funktion"


def _signatur(code, art):
    """Die Zeile, die oben am Eintrag steht."""
    if art == "konstante":
        treffer = re.match(r"#define\s+(\w+)\s*(.*)", code, re.S)
        wert = " ".join(treffer.group(2).split())
        return f"#define {treffer.group(1)} {wert}".strip()
    if art == "makro":
        treffer = re.match(r"#define\s+(\w+\([^)]*\))", code, re.S)
        return " ".join(treffer.group(1).split())
    if art == "typ":
        kopf = code.split("{", 1)[0].strip()
        name = _name_der_deklaration(code)
        return f"{kopf} {{ ... }} {name};" if "{" in code else code
    # Funktion: Attribute weg, Zeilenumbrueche zusammenziehen
    code = re.sub(r"\s*__attribute__\s*\(\(.*?\)\)", "", code, flags=re.S)
    return " ".join(code.split()).rstrip(";") + ";"


def _felder(code):
    """Felder eines struct oder enum mit ihrer Beschreibung.

    Beschrieben wird ein Feld entweder rechts mit ///< oder darueber mit
    gewoehnlichen //-Zeilen. Zeilen der Form // --- Ueberschrift --- gliedern
    die Liste und werden als Gruppe uebernommen.
    """
    ist_enum = re.match(r"typedef\s+enum|^enum", code.strip()) is not None
    innen = code[code.find("{") + 1:code.rfind("}")]
    felder, sammler, gruppe, puffer, rechts = [], [], "", "", ""

    # Ein abschliessendes Semikolon erzwingt, dass auch das letzte Feld
    # herausfaellt, wenn es ohne Komma vor der Klammer steht.
    for zeile in innen.splitlines() + [";"]:
        text = zeile.strip()
        if not text:
            continue

        ueberschrift = re.match(r"//\s*-{2,}\s*(.*?)\s*-{2,}\s*$", text)
        if ueberschrift:                       # // --- Ort und Bewegung ---
            gruppe = ueberschrift.group(1)
            sammler = []
            continue

        treffer = re.search(r"///<\s*(.*)$", text)   # Kommentar rechts
        if treffer:
            rechts = (rechts + " " + treffer.group(1)).strip()
            text = text[:treffer.start()].strip()

        if text.startswith("//"):              # Kommentar ueber dem Feld
            sammler.append(text.lstrip("/ ").strip())
            continue
        if not text:                           # Zeile enthielt nur ///< ...
            if rechts and felder and not felder[-1]["text"]:
                felder[-1]["text"] = rechts
            continue

        puffer = (puffer + " " + text).strip()
        if not (puffer.endswith(";") or puffer.endswith(",")):
            continue                           # Deklaration geht weiter
        deklaration = puffer.rstrip(";, ").strip()
        puffer = ""
        beschreibung = rechts or " ".join(sammler)
        sammler, rechts = [], ""
        if not deklaration:
            continue

        if ist_enum:
            name = re.match(r"(\w+)", deklaration).group(1)
        elif "(" in deklaration:               # Funktionszeiger: on_anim_done
            zeiger = re.search(r"\(\s*\*\s*(\w+)\s*\)", deklaration)
            name = zeiger.group(1) if zeiger else deklaration
        else:                                  # letzter Bezeichner vor : oder Ende
            ohne_bitfeld = deklaration.split(":")[0]
            name = re.findall(r"\w+", ohne_bitfeld)[-1]

        felder.append({"gruppe": gruppe, "name": name,
                       "deklaration": deklaration, "text": beschreibung,
                       "intern": beschreibung.startswith("[intern]")})
    return felder


# --- Die Datei als Ganzes --------------------------------------------------

KOMMENTAR = re.compile(r"/\*\*(?!\*)(.*?)\*/", re.S)


def header_lesen(pfad):
    """Liefert alle dokumentierten Eintraege einer Header-Datei."""
    quelltext = open(pfad, encoding="utf-8", errors="replace").read()
    eintraege = []

    for block in KOMMENTAR.finditer(quelltext):
        rest = quelltext[block.end():]
        zeile_nr = quelltext[:block.start()].count("\n") + 1

        # Vom Kommentar bis zum Ende der Deklaration lesen.
        code = _deklaration_lesen(rest)
        if not code:
            continue
        art = _art(code)
        name = _name_der_deklaration(code)
        if not name:
            continue
        eintrag = {"name": name, "art": art, "signatur": _signatur(code, art),
                   "quelle": code, "datei": os.path.basename(pfad),
                   "zeile": zeile_nr, "felder": []}
        eintrag.update(_doku_zerlegen(block.group(0)))
        if art == "typ" and "{" in code:
            eintrag["felder"] = _felder(code)
        eintraege.append(eintrag)

    return eintraege


def _deklaration_lesen(rest):
    """Der Code direkt nach einem Kommentarblock, bis er vollstaendig ist."""
    zeilen = rest.splitlines()
    gesammelt = []
    tiefe = 0
    fortsetzung = False
    for zeile in zeilen:
        blank = zeile.strip()
        if not gesammelt:
            if not blank:
                continue
            if blank.startswith("//") or blank.startswith("/*"):
                return ""            # ein weiterer Kommentar, keine Deklaration
            if blank.startswith("#ifndef") or blank.startswith("#if"):
                continue             # Schutz um ein #define herum
        gesammelt.append(zeile)
        text = "\n".join(gesammelt)
        if text.lstrip().startswith("#define"):
            if not blank.endswith("\\"):
                break
            fortsetzung = True
            continue
        tiefe += zeile.count("{") - zeile.count("}")
        if tiefe == 0 and blank.endswith(";"):
            break
    code = "\n".join(gesammelt).strip()
    if fortsetzung:
        code = re.sub(r"\\\n", "\n", code)
    return code
