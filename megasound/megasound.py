"""
megasound.py -- MIDI-Datei in eine Tontabelle fuer die MEGACARD umwandeln.

Waehlt eine Spur, einen Taktbereich und erzeugt daraus C-Code fuer den
Piepser an OC0 (PB3). Timer0 laeuft dabei im CTC-Modus mit umschaltendem
Ausgang, die Tonhoehe kommt aus OCR0:

    f = F_CPU / (2 * Vorteiler * (1 + OCR0))

Aufruf ohne Argumente oeffnet die Oberflaeche. Ausserdem:
    python megasound.py --info  datei.mid
    python megasound.py --gen   datei.mid --spur 5 --takt 9 --takte 8
    python megasound.py --version
"""

import os
import struct
import sys

# Versionsnummer nach dem Schema MAJOR.MINOR.PATCH. Einzige Quelle: build.py
# liest sie von hier und setzt sie mit --bump herauf.
VERSION = '1.0.1'

NL = chr(10)

F_CPU_STANDARD = 12_000_000
TICK_HZ_STANDARD = 100          # Takt der Ablaufsteuerung im Spiel
VORTEILER = [(1, 1), (8, 2), (64, 3), (256, 4), (1024, 5)]   # (Teiler, CS-Bits)

NOTENNAMEN = ['C', 'Cis', 'D', 'Dis', 'E', 'F', 'Fis', 'G', 'Gis', 'A', 'Ais', 'H']

INSTRUMENTE = {
    0: 'Klavier', 24: 'Gitarre', 25: 'Gitarre', 26: 'E-Gitarre', 27: 'E-Gitarre',
    28: 'E-Gitarre', 30: 'E-Gitarre', 32: 'Bass', 33: 'Bass', 34: 'Bass',
    38: 'Synthbass', 48: 'Streicher', 49: 'Streicher', 50: 'Synthstreicher',
    52: 'Chor', 53: 'Chor', 54: 'Chor', 56: 'Trompete', 61: 'Blaeser',
    80: 'Leadsynth', 81: 'Leadsynth', 87: 'Basslead', 122: 'Geraeusch',
}


def _heute():
    import datetime
    return datetime.date.today().strftime('%d.%m.%Y')


def _programmordner():
    """Ordner, in dem das Programm liegt.

    Als exe entpackt PyInstaller das Skript in einen temporaeren Ordner,
    __file__ zeigt dann dorthin. Massgeblich ist dort der Ort der exe."""
    if getattr(sys, 'frozen', False):
        return os.path.dirname(os.path.abspath(sys.executable))
    return os.path.dirname(os.path.abspath(__file__))


def _konsole_anbinden():
    """Verbindet die Ausgabe mit der Konsole des Aufrufers.

    Die exe ist als Fensterprogramm gebaut und hat keine eigene Konsole,
    sys.stdout ist dort None. Fuer die Kommandozeile wird deshalb die Konsole
    uebernommen, aus der das Programm gestartet wurde."""
    if sys.stdout is not None:
        return
    try:
        import ctypes
        if ctypes.windll.kernel32.AttachConsole(-1):     # ATTACH_PARENT_PROCESS
            sys.stdout = open('CONOUT$', 'w')
            sys.stderr = sys.stdout
    except Exception:
        pass


def notenname(nummer):
    return NOTENNAMEN[nummer % 12] + str(nummer // 12 - 1)


def frequenz(nummer):
    return 440.0 * 2.0 ** ((nummer - 69) / 12.0)


def timerwert(f, f_cpu=F_CPU_STANDARD):
    """Kleinster Vorteiler, bei dem OCR0 noch in acht Bit passt."""
    for teiler, csbits in VORTEILER:
        ocr = round(f_cpu / (2.0 * teiler * f)) - 1
        if 0 <= ocr <= 255:
            ist = f_cpu / (2.0 * teiler * (1 + ocr))
            return ocr, csbits, teiler, ist
    return None

# ---------------------------------------------------------------------------
# MIDI einlesen
# ---------------------------------------------------------------------------
class Spur:
    def __init__(self, nummer):
        self.nummer = nummer
        self.name = ''
        self.programm = None
        self.kanaele = set()
        self.noten = []          # (start_tick, notennummer, ende_tick)

    @property
    def umfang(self):
        if not self.noten:
            return None
        hoehen = [n for _, n, _ in self.noten]
        return min(hoehen), max(hoehen)

    @property
    def mehrstimmig(self):
        """Groesste Anzahl gleichzeitig klingender Noten."""
        punkte = []
        for a, _, e in self.noten:
            punkte.append((a, 1))
            punkte.append((e, -1))
        punkte.sort()
        jetzt = hoechstens = 0
        for _, richtung in punkte:
            jetzt += richtung
            hoechstens = max(hoechstens, jetzt)
        return hoechstens

    def erster_takt(self, ticks_je_takt):
        """Takt, in dem die erste Note dieser Spur steht. 1 wenn leer."""
        if not self.noten:
            return 1
        return self.noten[0][0] // ticks_je_takt + 1

    @property
    def instrument(self):
        if 9 in self.kanaele:
            return 'Schlagzeug'
        if self.programm is None:
            return ''
        return INSTRUMENTE.get(self.programm, 'Programm ' + str(self.programm))


class MidiDatei:
    def __init__(self, pfad):
        self.pfad = pfad
        roh = open(pfad, 'rb').read()
        if roh[:4] != b'MThd':
            raise ValueError('Keine MIDI-Datei: ' + os.path.basename(pfad))
        kopflaenge, self.format, anzahl, self.aufloesung = struct.unpack('>IHHH', roh[4:14])
        self.tempo = 500000                     # Mikrosekunden je Viertel
        self.taktschlaege = 4                   # Zaehler der Taktart
        self.spuren = []
        pos = 8 + kopflaenge
        nummer = 0
        while pos < len(roh) and roh[pos:pos + 4] == b'MTrk':
            laenge = struct.unpack('>I', roh[pos + 4:pos + 8])[0]
            nummer += 1
            self.spuren.append(self._spur_lesen(roh[pos + 8:pos + 8 + laenge], nummer))
            pos += 8 + laenge

    @staticmethod
    def _zahl(daten, i):
        """Variable-length quantity, wie MIDI sie fuer Zeitabstaende benutzt."""
        wert = 0
        while True:
            b = daten[i]
            i += 1
            wert = (wert << 7) | (b & 0x7F)
            if not b & 0x80:
                return wert, i

    def _spur_lesen(self, daten, nummer):
        spur = Spur(nummer)
        i = 0
        zeit = 0
        status = 0
        offen = {}
        while i < len(daten):
            abstand, i = self._zahl(daten, i)
            zeit += abstand
            if daten[i] & 0x80:
                status = daten[i]
                i += 1
            art = status & 0xF0
            if status == 0xFF:                       # Meta-Ereignis
                typ = daten[i]
                i += 1
                laenge, i = self._zahl(daten, i)
                inhalt = daten[i:i + laenge]
                i += laenge
                if typ == 0x03 and not spur.name:
                    spur.name = inhalt.decode('latin-1').strip()
                elif typ == 0x51:
                    self.tempo = int.from_bytes(inhalt, 'big')
                elif typ == 0x58 and len(inhalt) >= 2:
                    self.taktschlaege = inhalt[0]
            elif status in (0xF0, 0xF7):
                laenge, i = self._zahl(daten, i)
                i += laenge
            elif art in (0x80, 0x90):
                hoehe = daten[i]
                anschlag = daten[i + 1]
                i += 2
                spur.kanaele.add(status & 0x0F)
                if art == 0x90 and anschlag > 0:
                    offen[hoehe] = zeit
                elif hoehe in offen:
                    spur.noten.append((offen.pop(hoehe), hoehe, zeit))
            elif art in (0xA0, 0xB0, 0xE0):
                i += 2
            elif art == 0xC0:
                spur.programm = daten[i]
                spur.kanaele.add(status & 0x0F)
                i += 1
            elif art == 0xD0:
                i += 1
            else:
                break                                # unbekannt, Spur abbrechen
        spur.noten.sort()
        return spur

    @property
    def ticks_je_takt(self):
        return self.aufloesung * self.taktschlaege

    def sekunden(self, ticks):
        return ticks / self.aufloesung * self.tempo / 1_000_000.0

# ---------------------------------------------------------------------------
# Auswahl in eine einstimmige Tonfolge verwandeln
# ---------------------------------------------------------------------------
class Ton:
    def __init__(self, hoehe, dauer_ms):
        self.hoehe = hoehe            # None = Pause
        self.dauer_ms = dauer_ms

    def __repr__(self):
        name = 'Pause' if self.hoehe is None else notenname(self.hoehe)
        return name + '/' + str(int(self.dauer_ms)) + 'ms'


def tonfolge(midi, spur, ab_takt=1, takte=None, luecke_ms=20):
    """Schneidet den Bereich aus und macht daraus eine einstimmige Folge.

    Klingen mehrere Noten gleichzeitig, bleibt die hoechste stehen. Luecken
    zwischen den Noten werden zu Pausen, sehr kurze Luecken verschluckt.
    """
    start = (ab_takt - 1) * midi.ticks_je_takt
    ende = start + takte * midi.ticks_je_takt if takte else None

    ausschnitt = []
    for a, hoehe, e in spur.noten:
        if e <= start:
            continue
        if ende is not None and a >= ende:
            continue
        a = max(a, start)
        e = min(e, ende) if ende is not None else e
        if e > a:
            ausschnitt.append((a, hoehe, e))
    if not ausschnitt:
        return []

    # Einstimmig machen: bei Ueberlappung gewinnt die hoehere Note
    ausschnitt.sort()
    einstimmig = []
    for a, hoehe, e in ausschnitt:
        if einstimmig and a < einstimmig[-1][2]:
            va, vh, ve = einstimmig[-1]
            if hoehe > vh:
                if a > va:
                    einstimmig[-1] = (va, vh, a)     # Vorgaenger kuerzen
                else:
                    einstimmig.pop()                 # Vorgaenger ersetzen
                einstimmig.append((a, hoehe, e))
            continue                                  # tiefere Note faellt weg
        einstimmig.append((a, hoehe, e))

    # Pausen einfuegen und in Millisekunden umrechnen
    folge = []
    zeiger = einstimmig[0][0]
    for a, hoehe, e in einstimmig:
        if a > zeiger:
            pause = midi.sekunden(a - zeiger) * 1000.0
            if pause >= luecke_ms:
                folge.append(Ton(None, pause))
            elif folge:
                folge[-1].dauer_ms += pause          # winzige Luecke anhaengen
        folge.append(Ton(hoehe, midi.sekunden(e - a) * 1000.0))
        zeiger = e
    return folge


def in_c(folge, name='Melodie', f_cpu=F_CPU_STANDARD, tick_hz=TICK_HZ_STANDARD):
    """Erzeugt den C-Block. Ein Ton sind drei Byte: OCR0, Vorteiler, Dauer."""
    ms_je_tick = 1000.0 / tick_hz
    zeilen = []
    fehler = []
    for ton in folge:
        ticks = max(1, min(255, round(ton.dauer_ms / ms_je_tick)))
        if ton.hoehe is None:
            zeilen.append(('\t{ 0, 0, ' + str(ticks) + ' },').ljust(30) + '// Pause')
            continue
        soll = frequenz(ton.hoehe)
        treffer = timerwert(soll, f_cpu)
        if treffer is None:
            fehler.append(notenname(ton.hoehe) + ' liegt ausserhalb des Bereichs')
            continue
        ocr, csbits, teiler, ist = treffer
        zeilen.append(('\t{ ' + str(ocr) + ', ' + str(csbits) + ', ' + str(ticks) + ' },').ljust(30)
                      + '// ' + notenname(ton.hoehe)
                      + '  ' + format(soll, '.1f') + ' Hz, Vorteiler ' + str(teiler))
    kopf = ['/*-------------------------------------------------------------------------*\\',
            '| Datei:        ' + name + '.c',
            '| Version:      1.0',
            '| Projekt:      Toene und Melodien auf der MEGACARD',
            '| Beschreibung: Tontabelle, erzeugt von megasound.py. Nicht von Hand aendern.',
            '| Schaltung:    MEGACARD V6.11, Piezo an PB3',
            '| Autor:        megasound ' + VERSION,
            '| Erstellung:   ' + _heute(),
            '|',
            '| Aenderung:',
            '\\*-------------------------------------------------------------------------*/',
            '',
            '// Ein Eintrag: OCR0, Vorteilerbits fuer TCCR0, Dauer in Ticks.',
            '// OCR0 gleich null bedeutet Pause. Tickrate: ' + str(tick_hz) + ' Hz.',
            '//',
            '// Dazu gehoert im Programm:',
            '//   typedef struct { uint8_t ocr; uint8_t clock; uint8_t ticks; } TON_T;',
            '//   Timer0: TCCR0 = (1<<WGM01) | (1<<COM00), Tonhoehe ueber OCR0,',
            '//   Vorteiler ueber die unteren drei Bit von TCCR0.',
            '//',
            '// Die Tabelle ist mit static bewusst auf diese Datei beschraenkt.',
            '// Nach aussen sichtbar ist nur die Melodie darunter, und die bringt',
            '// ihre Laenge selbst mit.',
            '',
            '#include <avr/pgmspace.h>',
            '// Pfad an den Ort dieser Datei anpassen: aus einem Demo-Ordner',
            '// heraus ist es ../../megalib/sound/sound.h',
            '#include "sound.h"',
            '',
            'static const TON_T ' + name + '_toene[] PROGMEM = {']
    fuss = ['};', '',
            '// MELODIE() laesst den Compiler die Eintraege zaehlen. Die Laenge kann',
            '// dadurch nicht zur Tabelle daneben passen und trotzdem falsch sein.',
            'const MELODIE_T ' + name + ' PROGMEM = MELODIE(' + name + '_toene);', '',
            '// Damit das Programm die Melodie findet, diese Zeile in die',
            '// Header-Datei neben dieser Datei eintragen:',
            '//   extern const MELODIE_T ' + name + ' PROGMEM;']
    return NL.join(kopf + zeilen + fuss), fehler

# ---------------------------------------------------------------------------
# Vorhoeren (nur Windows, benutzt den eingebauten Tongenerator)
# ---------------------------------------------------------------------------
# Haelt den zuletzt abgespielten Puffer am Leben, solange Windows ihn liest.
_puffer = {'pfad': None}


def wav_bauen(folge, rate=22050, ausklang_ms=3.0):
    """Baut aus der Tonfolge eine Rechteckwelle als WAV im Speicher.

    Ein Rechteck klingt so, wie es spaeter aus dem Piepser kommt. Am Ende
    jeder Note wird kurz ausgeblendet, sonst knackt es bei jedem Wechsel.
    """
    import struct

    stille = 128
    oben, unten = 200, 56
    daten = bytearray()

    for ton in folge:
        anzahl = int(rate * ton.dauer_ms / 1000.0)
        if anzahl <= 0:
            continue
        if ton.hoehe is None:
            daten.extend(bytes([stille]) * anzahl)
            continue

        periode = rate / frequenz(ton.hoehe)
        ausklang = min(anzahl, int(rate * ausklang_ms / 1000.0))
        beginn_ausklang = anzahl - ausklang

        for i in range(anzahl):
            wert = oben if (i % periode) < periode / 2.0 else unten
            if i >= beginn_ausklang and ausklang > 0:
                rest = (anzahl - i) / float(ausklang)
                wert = int(stille + (wert - stille) * rest)
            daten.append(wert)

    kopf = b'RIFF' + struct.pack('<I', 36 + len(daten)) + b'WAVEfmt '
    kopf += struct.pack('<IHHIIHH', 16, 1, 1, rate, rate, 1, 8)
    kopf += b'data' + struct.pack('<I', len(daten))
    return bytes(kopf) + bytes(daten)


def vorhoeren_starten(wav):
    """Startet die Wiedergabe und kehrt sofort zurueck.

    Python erlaubt SND_ASYNC nicht zusammen mit SND_MEMORY, weil der Puffer
    sonst freigegeben werden koennte, waehrend Windows noch daraus liest.
    Deshalb geht die Welle ueber eine Datei im Temp-Ordner.

    Muss aus demselben Thread aufgerufen werden wie vorhoeren_stoppen(),
    sonst greift SND_PURGE nicht. In der Oberflaeche ist das der
    Oberflaechen-Thread, gebaut wird die Welle daneben.
    """
    try:
        import winsound
    except ImportError:
        return 'Vorhoeren geht nur unter Windows.'
    import tempfile

    winsound.PlaySound(None, winsound.SND_PURGE)     # Datei wieder freigeben
    pfad = os.path.join(tempfile.gettempdir(), 'megasound_vorschau.wav')
    try:
        open(pfad, 'wb').write(wav)
    except OSError as e:
        return 'Vorschau nicht schreibbar: ' + str(e)
    _puffer['pfad'] = pfad
    winsound.PlaySound(pfad, winsound.SND_FILENAME | winsound.SND_ASYNC)
    return 'spielt'


def vorhoeren(folge):
    """Blockierende Wiedergabe, fuer die Kommandozeile."""
    try:
        import winsound
    except ImportError:
        return 'Vorhoeren geht nur unter Windows.'
    winsound.PlaySound(wav_bauen(folge), winsound.SND_MEMORY)
    return 'fertig'


def vorhoeren_stoppen():
    try:
        import winsound
        winsound.PlaySound(None, winsound.SND_PURGE)
    except Exception:
        pass


# ---------------------------------------------------------------------------
# Kommandozeile
# ---------------------------------------------------------------------------
def spurtabelle(midi):
    zeilen = []
    kopf = ('Nr  Name                 Instrument    Noten  Umfang        '
            'Stimmen  ab Takt  Laenge')
    zeilen.append(kopf)
    zeilen.append('-' * len(kopf))
    for s in midi.spuren:
        if not s.noten:
            zeilen.append(format(s.nummer, '2d') + '  ' + s.name[:20].ljust(20)
                          + '  ' + s.instrument.ljust(12) + '      0')
            continue
        tief, hoch = s.umfang
        dauer = midi.sekunden(max(e for _, _, e in s.noten))
        zeilen.append(
            format(s.nummer, '2d') + '  ' + s.name[:20].ljust(20) + '  '
            + s.instrument.ljust(12) + format(len(s.noten), '6d') + '  '
            + (notenname(tief) + '..' + notenname(hoch)).ljust(13)
            + format(s.mehrstimmig, '5d') + format(s.erster_takt(midi.ticks_je_takt), '9d')
            + '   ' + format(dauer, '6.1f') + 's')
    return NL.join(zeilen)


def hauptprogramm_cli(argumente):
    def wert(name, standard=None, zahl=True):
        if name in argumente:
            v = argumente[argumente.index(name) + 1]
            return int(v) if zahl else v
        return standard

    if '--version' in argumente:
        print('megasound ' + VERSION)
        return 0

    pfad = None
    for a in argumente[1:]:
        if a.lower().endswith('.mid') or a.lower().endswith('.midi'):
            pfad = a
            break
    if pfad is None:
        print('Bitte eine MIDI-Datei angeben.')
        return 2

    midi = MidiDatei(pfad)
    print(os.path.basename(pfad) + ':  Format ' + str(midi.format)
          + ', ' + str(len(midi.spuren)) + ' Spuren, '
          + str(midi.aufloesung) + ' Ticks je Viertel, '
          + format(60_000_000 / midi.tempo, '.0f') + ' bpm, '
          + str(midi.taktschlaege) + ' Schlaege je Takt')
    print()
    print(spurtabelle(midi))

    if '--info' in argumente:
        return 0

    nr = wert('--spur')
    if nr is None:
        print(NL + 'Fuer die Umwandlung fehlt --spur.')
        return 2
    spur = next((s for s in midi.spuren if s.nummer == nr), None)
    if spur is None:
        print('Spur ' + str(nr) + ' gibt es nicht.')
        return 2

    folge = tonfolge(midi, spur, wert('--takt', 1), wert('--takte', None))
    if not folge:
        print('In diesem Bereich stehen keine Noten.')
        return 1
    code, fehler = in_c(folge, wert('--name', 'Melodie', zahl=False),
                        wert('--fcpu', F_CPU_STANDARD), wert('--tick', TICK_HZ_STANDARD))
    for f in fehler:
        print('Hinweis: ' + f)
    ziel = wert('--aus', os.path.splitext(pfad)[0] + '_melodie.c', zahl=False)
    open(ziel, 'w', encoding='utf-8', newline=NL).write(code + NL)
    print(NL + str(len(folge)) + ' Toene geschrieben nach ' + os.path.basename(ziel))
    return 0

# ---------------------------------------------------------------------------
# Oberflaeche
# ---------------------------------------------------------------------------
def hauptprogramm_ui():
    import threading
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk

    zustand = {'midi': None, 'pfad': None, 'abbruch': None}

    fenster = tk.Tk()
    fenster.title('megasound ' + VERSION + ' -- MIDI in Tontabelle fuer die MEGACARD')
    fenster.geometry('980x640')

    # --- Datei ---
    oben = ttk.Frame(fenster, padding=8)
    oben.pack(fill='x')
    ttk.Label(oben, text='MIDI-Datei:').pack(side='left')
    dateifeld = ttk.Entry(oben)
    dateifeld.pack(side='left', fill='x', expand=True, padx=6)

    # --- Spurtabelle ---
    mitte = ttk.Frame(fenster, padding=(8, 0))
    mitte.pack(fill='both', expand=True)
    spalten = ('nr', 'name', 'instrument', 'noten', 'umfang', 'stimmen',
               'abtakt', 'laenge')
    titel = ('Nr', 'Name', 'Instrument', 'Noten', 'Umfang', 'Stimmen',
             'ab Takt', 'Laenge')
    breiten = (40, 190, 110, 60, 110, 65, 65, 75)
    tabelle = ttk.Treeview(mitte, columns=spalten, show='headings', height=9)
    for spalte, text, breite in zip(spalten, titel, breiten):
        tabelle.heading(spalte, text=text)
        tabelle.column(spalte, width=breite, anchor='w')
    tabelle.pack(side='left', fill='both', expand=True)
    leiste = ttk.Scrollbar(mitte, orient='vertical', command=tabelle.yview)
    leiste.pack(side='left', fill='y')
    tabelle.configure(yscrollcommand=leiste.set)

    # --- Einstellungen ---
    unten = ttk.Frame(fenster, padding=8)
    unten.pack(fill='x')

    def feld(text, standard, breite=7):
        ttk.Label(unten, text=text).pack(side='left', padx=(10, 2))
        e = ttk.Entry(unten, width=breite)
        e.insert(0, str(standard))
        e.pack(side='left')
        return e

    takt_feld = feld('ab Takt', 1, 5)
    takte_feld = feld('Anzahl Takte', 8, 5)
    tick_feld = feld('Tickrate Hz', TICK_HZ_STANDARD, 6)
    fcpu_feld = feld('F_CPU', F_CPU_STANDARD, 10)
    name_feld = feld('Arrayname', 'Melodie', 12)

    # --- Ausgabe ---
    ausgabe = tk.Text(fenster, height=14, font=('Consolas', 9), wrap='none')
    ausgabe.pack(fill='both', expand=True, padx=8, pady=(0, 4))
    fusszeile = ttk.Frame(fenster, padding=(8, 0, 8, 6))
    fusszeile.pack(fill='x')
    ttk.Label(fusszeile, text='Version ' + VERSION,
              foreground='gray').pack(side='right')
    statuszeile = ttk.Label(fusszeile, text='Bitte eine MIDI-Datei waehlen.')
    statuszeile.pack(side='left', fill='x', expand=True)

    def melden(text):
        statuszeile.config(text=text)
        fenster.update_idletasks()

    def zahl(eingabe, standard):
        try:
            return int(eingabe.get())
        except ValueError:
            return standard

    def gewaehlte_spur():
        auswahl = tabelle.selection()
        if not auswahl:
            return None
        nr = int(tabelle.item(auswahl[0], 'values')[0])
        return next((s for s in zustand['midi'].spuren if s.nummer == nr), None)

    def aktuelle_folge():
        if zustand['midi'] is None:
            melden('Erst eine Datei laden.')
            return None
        spur = gewaehlte_spur()
        if spur is None:
            melden('Erst eine Spur in der Liste anklicken.')
            return None
        takte = zahl(takte_feld, 0)
        folge = tonfolge(zustand['midi'], spur, zahl(takt_feld, 1),
                         takte if takte > 0 else None)
        if not folge:
            melden('Takt ' + str(zahl(takt_feld, 1)) + ' bis '
                   + str(zahl(takt_feld, 1) + takte - 1) + ' ist leer. Spur '
                   + str(spur.nummer) + ' beginnt bei Takt '
                   + str(spur.erster_takt(zustand['midi'].ticks_je_takt)) + '.')
            return None
        return folge

    def datei_laden(pfad):
        try:
            zustand['midi'] = MidiDatei(pfad)
        except Exception as e:
            messagebox.showerror('Fehler', str(e))
            return
        zustand['pfad'] = pfad
        dateifeld.delete(0, 'end')
        dateifeld.insert(0, pfad)
        tabelle.delete(*tabelle.get_children())
        midi = zustand['midi']
        for s in midi.spuren:
            if not s.noten:
                continue
            tief, hoch = s.umfang
            dauer = midi.sekunden(max(e for _, _, e in s.noten))
            tabelle.insert('', 'end', values=(
                s.nummer, s.name or '(ohne Namen)', s.instrument, len(s.noten),
                notenname(tief) + '..' + notenname(hoch), s.mehrstimmig,
                s.erster_takt(midi.ticks_je_takt), format(dauer, '.1f') + 's'))
        melden(os.path.basename(pfad) + ':  ' + str(len(midi.spuren)) + ' Spuren, '
               + format(60000000.0 / midi.tempo, '.0f') + ' bpm, '
               + str(midi.taktschlaege) + ' Schlaege je Takt, '
               + str(midi.ticks_je_takt) + ' Ticks je Takt')

    def waehlen():
        start = os.path.dirname(zustand['pfad'] or '')
        if not start:
            start = _programmordner()
        pfad = filedialog.askopenfilename(
            initialdir=start,
            filetypes=[('MIDI', '*.mid *.midi'), ('Alle Dateien', '*.*')])
        if pfad:
            datei_laden(pfad)

    def vorschau():
        folge = aktuelle_folge()
        if folge is None:
            return
        code, fehler = in_c(folge, name_feld.get() or 'Melodie',
                            zahl(fcpu_feld, F_CPU_STANDARD),
                            zahl(tick_feld, TICK_HZ_STANDARD))
        ausgabe.delete('1.0', 'end')
        ausgabe.insert('1.0', code)
        noten = ' '.join(repr(t) for t in folge[:24])
        nachsatz = ' ...' if len(folge) > 24 else ''
        melden(str(len(folge)) + ' Toene.  ' + noten + nachsatz)
        for f in fehler:
            melden(f)

    def abspielen():
        folge = aktuelle_folge()
        if folge is None:
            return
        vorhoeren_stoppen()
        melden('Baue Vorschau ...')

        def arbeit():
            wav = wav_bauen(folge)

            def starten():
                vorhoeren_starten(wav)
                dauer = sum(t.dauer_ms for t in folge) / 1000.0
                melden(str(len(folge)) + ' Toene, ' + format(dauer, '.1f') + ' s, spielt ...')

            fenster.after(0, starten)

        threading.Thread(target=arbeit, daemon=True).start()

    def stoppen():
        vorhoeren_stoppen()
        melden('Abspielen gestoppt.')

    def speichern():
        folge = aktuelle_folge()
        if folge is None:
            return
        code, _ = in_c(folge, name_feld.get() or 'Melodie',
                       zahl(fcpu_feld, F_CPU_STANDARD),
                       zahl(tick_feld, TICK_HZ_STANDARD))
        vorgabe = os.path.splitext(os.path.basename(zustand['pfad']))[0] + '_melodie.c'
        ziel = filedialog.asksaveasfilename(
            initialdir=os.path.dirname(zustand['pfad']), initialfile=vorgabe,
            defaultextension='.c', filetypes=[('C-Quelle', '*.c')])
        if not ziel:
            return
        open(ziel, 'w', encoding='utf-8', newline=NL).write(code + NL)
        melden('Gespeichert: ' + ziel)

    ttk.Button(oben, text='Datei waehlen ...', command=waehlen).pack(side='left')
    ttk.Button(unten, text='Vorschau', command=vorschau).pack(side='left', padx=(16, 4))
    ttk.Button(unten, text='Abspielen', command=abspielen).pack(side='left', padx=4)
    ttk.Button(unten, text='Stopp', command=stoppen).pack(side='left', padx=4)
    ttk.Button(unten, text='C-Datei speichern', command=speichern).pack(side='left', padx=4)
    def spur_gewaehlt(_ereignis=None):
        """Springt an den Anfang der Spur. Viele Spuren setzen erst spaeter
        ein, ohne das sieht man sonst nur eine leere Auswahl."""
        spur = gewaehlte_spur()
        if spur is not None and spur.noten:
            takt_feld.delete(0, 'end')
            takt_feld.insert(0, str(spur.erster_takt(zustand['midi'].ticks_je_takt)))
        vorschau()

    tabelle.bind('<<TreeviewSelect>>', spur_gewaehlt)

    # Liegt eine MIDI-Datei neben dem Programm, gleich laden
    hier = _programmordner()
    for datei in sorted(os.listdir(hier)):
        if datei.lower().endswith('.mid') or datei.lower().endswith('.midi'):
            datei_laden(os.path.join(hier, datei))
            break

    fenster.mainloop()


if __name__ == '__main__':
    if len(sys.argv) > 1:
        _konsole_anbinden()
        sys.exit(hauptprogramm_cli(sys.argv))
    hauptprogramm_ui()
