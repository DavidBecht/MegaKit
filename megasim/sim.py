"""
megasim -- MegaCard Simulator
Usage: python sim.py <project_directory> [--scale N] [--fps N]
       python sim.py --version

Compiles all .c files in the given directory with fake AVR headers,
loads the resulting DLL, and renders the SSD1306 display in pygame.

Buttons:   1/2/3/4  ->  S0/S1/S2/S3  (active-low on PINA)
           Pfeile: rechts S0, links S1, unten S2, oben S3
LEDs:      PORTC bits shown as coloured circles
"""

import sys
import os
import re
import signal
import argparse
import subprocess
import ctypes
import threading
import time
import shutil
import zipfile
from concurrent.futures import ThreadPoolExecutor, wait, FIRST_COMPLETED

# Die Begruessung von pygame wuerde vor der Versionszeile in der Konsole stehen
os.environ.setdefault("PYGAME_HIDE_SUPPORT_PROMPT", "1")
# Mittig wie der Startbildschirm der exe, damit das Fenster ihn nahtlos ersetzt
os.environ.setdefault("SDL_VIDEO_CENTERED", "1")
import pygame

# Versionsnummer nach dem Schema MAJOR.MINOR.PATCH. Einzige Quelle: build.py
# liest sie von hier und setzt sie mit --bump herauf.
VERSION = '1.0.0'

# ---------------------------------------------------------------------------
# Paths — works both in development (plain Python) and frozen (PyInstaller)
# ---------------------------------------------------------------------------
if getattr(sys, "frozen", False):
    # PyInstaller --onefile: everything is extracted to sys._MEIPASS at runtime
    # PyInstaller --onedir:  sys._MEIPASS == directory of the .exe
    _BASE = sys._MEIPASS
    # Die GCC liegt als gcc.zip in der exe. Einzeln mitgeliefert muesste
    # PyInstaller bei jedem Start rund 2300 Dateien entpacken, das dauert
    # mehrere Sekunden. Das Archiv wird stattdessen einmal in GCC_CACHE_DIR
    # ausgepackt, siehe gcc_bereitstellen().
    GCC_ZIP       = os.path.join(sys._MEIPASS, "gcc.zip")
    GCC_CACHE_DIR = os.path.join(os.environ.get("LOCALAPPDATA", os.path.dirname(sys.executable)),
                                 "megasim")
    # Ohne gcc.zip (aeltere Builds, --onedir): GCC neben der exe
    _GCC_DEFAULT  = os.path.join(os.path.dirname(sys.executable), "gcc", "bin", "gcc.exe")
    BUILD_DIR = os.path.join(os.environ.get("TEMP", os.path.dirname(sys.executable)), "megacard-sim-build")
else:
    GCC_ZIP       = None
    GCC_CACHE_DIR = None
    _BASE = os.path.dirname(os.path.abspath(__file__))
    _GCC_DEFAULT = r"C:\msys64\mingw64\bin\gcc.exe"
    BUILD_DIR = os.path.join(_BASE, "build")

SIM_DIR      = _BASE
FAKE_AVR_DIR = os.path.join(_BASE, "fake_avr")
FAKE_SRC_DIR = os.path.join(_BASE, "fake_src")
GCC          = os.environ.get("MEGACARD_GCC", _GCC_DEFAULT)

# Files replaced by simulator stubs (basename -> fake file in fake_src/)
REPLACED = {
    "twi_soft.c": "twi_fake.c",
    "random.c":   "fake_random.c",
}

# ---------------------------------------------------------------------------
# Ladeanzeige
# ---------------------------------------------------------------------------
class Ladeanzeige:
    """Ladebalken im Simulatorfenster, solange GCC eingerichtet und das
    Projekt uebersetzt wird.

    Gezeichnet wird auf einer Flaeche in OLED-Aufloesung, die anschliessend
    ohne Glaettung vergroessert wird. Schrift und Balken bestehen dadurch aus
    denselben Bloecken wie die Pixel der Anzeige und tragen die Farbe der
    Pixel ausserhalb der Zeichenflaeche."""

    BILDRATE = 30

    def __init__(self, scale, project_name):
        pygame.init()
        pygame.display.set_caption(f"megasim {VERSION} — {project_name}")
        self.scale    = scale
        self.projekt  = project_name
        self.fenster  = pygame.display.set_mode(fenstergroesse(scale))
        self.oled     = pygame.Surface((OLED_W, OLED_H))
        self.schrift_gross = pygame.font.SysFont("consolas", 16, bold=True)
        self.schrift_klein = pygame.font.SysFont("consolas", 11)
        self.schrift_panel = pygame.font.SysFont("consolas", 13, bold=True)
        self.von, self.bis = 0.0, 1.0
        self._zuletzt = 0.0
        self._zeichnen(0.0, "starte")
        self._startbild_schliessen()

    @staticmethod
    def _startbild_schliessen():
        """Schliesst den Startbildschirm der exe.

        PyInstaller zeigt ihn, waehrend sich die exe entpackt, also bevor
        Python laeuft. Er sieht aus wie das erste Bild dieser Anzeige; das
        Fenster uebernimmt an derselben Stelle."""
        try:
            import pyi_splash
            pyi_splash.close()
        except ImportError:
            pass

    def bereich(self, von, bis):
        """Legt fest, welchen Teil des Balkens der folgende Schritt fuellt."""
        self.von, self.bis = von, bis

    def zeigen(self, anteil, text, sofort=False):
        """anteil 0..1 bezieht sich auf den mit bereich() gesetzten Abschnitt.
        Ohne sofort wird hoechstens BILDRATE-mal je Sekunde gezeichnet."""
        self._ereignisse()
        jetzt = time.monotonic()
        if not sofort and jetzt - self._zuletzt < 1.0 / self.BILDRATE:
            return
        self._zuletzt = jetzt
        anteil = max(0.0, min(1.0, anteil))
        self._zeichnen(self.von + (self.bis - self.von) * anteil, text)

    def fehler(self, zeilen):
        """Zeigt eine Fehlermeldung und wartet, bis das Fenster geschlossen
        oder eine Taste gedrueckt wird. Die Einzelheiten stehen in der Konsole."""
        self._zeichnen(None, None, zeilen)
        while True:
            for ev in pygame.event.get():
                if ev.type in (pygame.QUIT, pygame.KEYDOWN):
                    pygame.quit()
                    return
            time.sleep(0.05)

    def _ereignisse(self):
        for ev in pygame.event.get():
            if ev.type == pygame.QUIT:
                pygame.quit()
                os._exit(0)

    def _text(self, schrift, text, y):
        bild = schrift.render(text, False, COL_PIXEL_OUTSIDE)
        self.oled.blit(bild, ((OLED_W - bild.get_width()) // 2, y))

    def _zeichnen(self, gesamt, text, fehlerzeilen=None):
        self.oled.fill(COL_BG)
        self._text(self.schrift_gross, "megasim", 3)
        self._text(self.schrift_klein, "v" + VERSION, 20)
        if fehlerzeilen:
            for i, zeile in enumerate(fehlerzeilen[:3]):
                self._text(self.schrift_klein, zeile[:21], 31 + i * 10)
        else:
            # Rahmen, ein Pixel Abstand, dann die Fuellung
            pygame.draw.rect(self.oled, COL_PIXEL_OUTSIDE, (8, 35, 112, 9), 1)
            breite = round(108 * gesamt)
            if breite > 0:
                self.oled.fill(COL_PIXEL_OUTSIDE, (10, 37, breite, 5))
            if text:
                self._text(self.schrift_klein, text[:21], 49)

        s = self.scale
        self.fenster.fill(COL_BORDER)
        self.fenster.blit(pygame.transform.scale(self.oled, (OLED_W * s, OLED_H * s)),
                          (OLED_BORDER, OLED_BORDER))
        panel_w, _ = fenstergroesse(s)
        panel = pygame.Surface((panel_w, PANEL_H))
        panel.fill(COL_PANEL)
        zustand = "Fehler" if fehlerzeilen else "wird geladen"
        info = self.schrift_panel.render(f"{self.projekt}   {zustand}", True, COL_TEXT)
        panel.blit(info, (panel_w // 2 - info.get_width() // 2, 4))
        self.fenster.blit(panel, (0, OLED_H * s + 2 * OLED_BORDER))
        pygame.display.flip()


def gcc_cache_ordner():
    """Ordner der ausgepackten GCC, oder None ohne mitgeliefertes Archiv."""
    if GCC_ZIP is None or not os.path.isfile(GCC_ZIP) or "MEGACARD_GCC" in os.environ:
        return None
    return os.path.join(GCC_CACHE_DIR, f"gcc-{os.path.getsize(GCC_ZIP)}")


def gcc_muss_ausgepackt_werden():
    ziel = gcc_cache_ordner()
    return ziel is not None and not os.path.isfile(os.path.join(ziel, "bin", "gcc.exe"))


def gcc_bereitstellen(anzeige):
    """Packt die mitgelieferte GCC beim ersten Start aus und setzt GCC.

    Der Zielordner traegt die Groesse des Archivs im Namen, eine exe mit
    anderer GCC bekommt dadurch einen eigenen Ordner. Ausgepackt wird in einen
    Zwischenordner, der erst am Ende umbenannt wird: ein abgebrochener Lauf
    hinterlaesst keinen halben Compiler, und zwei gleichzeitig gestartete
    Simulatoren kommen sich nicht in die Quere."""
    global GCC
    ziel = gcc_cache_ordner()
    if ziel is None:
        return
    GCC = os.path.join(ziel, "bin", "gcc.exe")
    if os.path.isfile(GCC):
        return

    print(f"[sim] GCC wird eingerichtet (einmalig) in {ziel}")
    zwischen = f"{ziel}.tmp{os.getpid()}"
    shutil.rmtree(zwischen, ignore_errors=True)
    with zipfile.ZipFile(GCC_ZIP) as archiv:
        eintraege = archiv.infolist()
        gesamt = sum(e.compress_size for e in eintraege) or 1
        erledigt = 0
        for e in eintraege:
            archiv.extract(e, zwischen)
            erledigt += e.compress_size
            anzeige.zeigen(erledigt / gesamt, "GCC einrichten")
    try:
        os.rename(zwischen, ziel)
    except OSError:
        # Ein zweiter Simulator war schneller, dessen Stand wird verwendet
        shutil.rmtree(zwischen, ignore_errors=True)

    # Staende frueherer Versionen entfernen. Noch benutzte Dateien sind
    # gesperrt und bleiben stehen, das ist unschaedlich.
    for eintrag in os.listdir(GCC_CACHE_DIR):
        pfad = os.path.join(GCC_CACHE_DIR, eintrag)
        if eintrag.startswith("gcc-") and pfad != ziel and ".tmp" not in eintrag:
            shutil.rmtree(pfad, ignore_errors=True)

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
def find_c_files(root):
    """Return all .c files under root, excluding replaced hardware drivers."""
    found = []
    for dirpath, _, files in os.walk(root):
        for f in files:
            if f.endswith(".c") and f not in REPLACED:
                found.append(os.path.join(dirpath, f))
    return found

def find_include_dirs(root):
    """Jeder Ordner unter root, der mindestens einen Header enthaelt.

    Der Projektstamm gehoert bewusst NICHT dazu. In Microchip Studio ist er
    ebenfalls kein Suchpfad, und beide Seiten sollen gleich streng sein. Sonst
    uebersetzt hier etwas, das auf der Hardware nicht baut. Angaben zwischen
    den Ordnern werden deshalb relativ geschrieben, etwa
        #include "../../megalib/sound/sound.h"
    """
    dirs = set()
    for dirpath, _, files in os.walk(root):
        if any(f.endswith(".h") for f in files):
            dirs.add(dirpath)
    return sorted(dirs)

def _is_up_to_date(dll_path, c_files, fake_files):
    """True wenn DLL neuer ist als alle Quelldateien."""
    if not os.path.isfile(dll_path):
        return False
    dll_mtime = os.path.getmtime(dll_path)
    if getattr(sys, "frozen", False):
        # Die exe packt fake_src/ bei jedem Start neu aus, die Dateien tragen
        # dann den Startzeitpunkt. Aendern koennen sie sich nur mit der exe.
        exe_mtime = os.path.getmtime(sys.executable)
        if exe_mtime > dll_mtime:
            return False
        return all(os.path.getmtime(f) <= dll_mtime for f in c_files)
    return all(os.path.getmtime(f) <= dll_mtime for f in c_files + fake_files)


def _dll_is_writable(path):
    """False, wenn die DLL von einer laufenden Simulator-Instanz gesperrt ist.
    Windows sperrt geladene DLLs gegen Schreibzugriff."""
    if not os.path.exists(path):
        return True
    try:
        with open(path, "r+b"):
            return True
    except OSError:
        return False


def pick_dll_path(build_dir, project_name):
    """Eigener DLL-Name je Projekt, bei Bedarf durchnummeriert.

    Alle Instanzen teilten sich frueher eine feste megacard.dll. Lief noch ein
    Simulator (oder hing ein Prozess), scheiterte der Link des naechsten mit
    'ld.exe: cannot open output file ... Permission denied'."""
    safe = re.sub(r"[^A-Za-z0-9_.-]", "_", project_name) or "projekt"
    for n in range(50):
        name = f"megacard-{safe}.dll" if n == 0 else f"megacard-{safe}-{n}.dll"
        path = os.path.join(build_dir, name)
        if _dll_is_writable(path):
            return path
    sys.exit(f"[sim] Alle DLL-Namen fuer {project_name} sind gesperrt. "
             "Bitte laufende Simulatorfenster schliessen.")


def build(project_dir, dll_path, anzeige):
    """Uebersetzt jede Quelldatei einzeln und parallel, dann wird gelinkt.

    Die Einzelschritte liefern den Fortschritt fuer den Ladebalken und nutzen
    alle Prozessorkerne. Scheitert ein Schritt, zeigt das Fenster einen Hinweis,
    die Meldungen von GCC stehen in der Konsole."""
    c_files = find_c_files(project_dir)
    if not c_files:
        sys.exit(f"[sim] No .c files found in {project_dir}")

    # Which replacement fakes are needed?
    all_project_files = {f for _, _, fs in os.walk(project_dir) for f in fs}
    fake_files = [os.path.join(FAKE_SRC_DIR, "sim_api.c")]
    for orig, fake in REPLACED.items():
        if orig in all_project_files:
            fake_files.append(os.path.join(FAKE_SRC_DIR, fake))

    if _is_up_to_date(dll_path, c_files, fake_files):
        print(f"[sim] {os.path.basename(dll_path)} ist aktuell, kein Rebuild noetig.")
        anzeige.zeigen(1.0, "aktuell", sofort=True)
        return

    inc_dirs  = find_include_dirs(project_dir)
    inc_flags = (
        [f"-I{FAKE_AVR_DIR}", f"-I{FAKE_SRC_DIR}"]
        + [f"-I{d}" for d in inc_dirs]
    )
    c_flags = inc_flags + [
        "-DF_CPU=12000000UL", "-DNDEBUG", "-std=gnu11", "-w",
        "-include", "stdint.h", "-include", "stdbool.h",
    ]

    # Ensure MinGW DLLs are findable by GCC itself
    env = os.environ.copy()
    mingw_bin = os.path.dirname(GCC)
    env["PATH"] = mingw_bin + os.pathsep + env.get("PATH", "")

    if not os.path.isfile(GCC):
        print(f"[sim] GCC nicht gefunden: {GCC}")
        if getattr(sys, "frozen", False):
            print(f"      Bitte WinLibs GCC entpacken nach:")
            print(f"      {os.path.join(os.path.dirname(sys.executable), 'gcc', 'bin', 'gcc.exe')}")
            print(f"      Download: https://github.com/brechtsanders/winlibs_mingw/releases/latest")
        sys.exit(1)

    quellen = c_files + fake_files
    print("[sim] Compiling ...")
    print("      " + " ".join(os.path.basename(f) for f in quellen))

    obj_dir = dll_path + ".obj"
    shutil.rmtree(obj_dir, ignore_errors=True)
    os.makedirs(obj_dir)

    def gcc(args):
        return subprocess.run([GCC] + args, capture_output=True, text=True,
                              encoding="utf-8", errors="replace", env=env)

    def uebersetzen(nr, quelle):
        # Nummer im Namen: gleichnamige Dateien in verschiedenen Ordnern
        # duerfen sich nicht ueberschreiben
        name = os.path.splitext(os.path.basename(quelle))[0]
        obj  = os.path.join(obj_dir, f"{nr:03d}_{name}.o")
        return quelle, obj, gcc(["-c", quelle, "-o", obj] + c_flags)

    # Der Linkschritt zaehlt als ein weiterer Schritt
    schritte = len(quellen) + 1
    objekte, fehler = [], []
    zuletzt = "uebersetze"
    anzeige.zeigen(0.0, zuletzt, sofort=True)
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        offen = {pool.submit(uebersetzen, nr, q) for nr, q in enumerate(quellen)}
        while offen:
            # Kurzes Warten haelt das Fenster bedienbar, auch wenn gerade
            # keine Datei fertig wird
            fertig, offen = wait(offen, timeout=1.0 / Ladeanzeige.BILDRATE,
                                 return_when=FIRST_COMPLETED)
            for f in fertig:
                quelle, obj, result = f.result()
                objekte.append(obj)
                zuletzt = os.path.basename(quelle)
                if result.returncode != 0:
                    fehler.append(result.stderr)
                elif result.stderr:
                    print("[sim] GCC warnings:\n" + result.stderr)
            anzeige.zeigen(len(objekte) / schritte, zuletzt)

    if not fehler:
        anzeige.zeigen(len(quellen) / schritte, "linke", sofort=True)
        result = gcc(["-shared", "-o", dll_path] + sorted(objekte)
                     + ["-Wl,--export-all-symbols"])
        if result.returncode != 0:
            fehler.append(result.stderr)
    shutil.rmtree(obj_dir, ignore_errors=True)

    if fehler:
        print("[sim] GCC errors:\n" + "\n".join(fehler))
        sys.stdout.flush()
        anzeige.fehler(["Fehler beim", "Uebersetzen,", "siehe Konsole"])
        sys.exit(1)
    anzeige.zeigen(1.0, "fertig", sofort=True)
    print(f"[sim] Built {os.path.basename(dll_path)}")

# ---------------------------------------------------------------------------
# EEPROM
# ---------------------------------------------------------------------------
# Das nachgebildete EEPROM liegt als Byte-Array in der DLL. Echte Hardware
# behaelt seinen Inhalt ueber Aus- und Einschalten und ueber einen Reset.
# Damit sich der Simulator genauso verhaelt, wird der Inhalt in eine Datei
# neben dem Projekt gesichert und beim Start wieder eingelesen.
EEPROM_SIZE = 512

# Wird von main() gesetzt und beim Reset nachgezogen, damit auch der
# Launcher-Watchdog den Inhalt noch sichern kann, wenn Microchip Studio
# den Simulator von aussen abschiesst.
_ee_ctx = {"lib": None, "pfad": None}

def eeprom_datei(project_dir):
    return os.path.join(project_dir, "megacard.eep")

def eeprom_array(lib):
    try:
        return (ctypes.c_uint8 * EEPROM_SIZE).in_dll(lib, "sim_eeprom")
    except (ValueError, OSError):
        return None          # Projekt benutzt kein EEPROM

def eeprom_laden(lib, pfad):
    arr = eeprom_array(lib)
    if arr is None or not os.path.isfile(pfad):
        return
    try:
        with open(pfad, "rb") as f:
            daten = f.read(EEPROM_SIZE)
    except OSError as e:
        print(f"[sim] EEPROM nicht lesbar: {e}")
        return
    for i, b in enumerate(daten):
        arr[i] = b
    print(f"[sim] EEPROM aus {os.path.basename(pfad)} geladen ({len(daten)} Byte)")

def eeprom_sichern_jetzt():
    """Sichert mit dem zuletzt gemerkten Zustand. Fuer Abbruch von aussen."""
    if _ee_ctx["lib"] is not None and _ee_ctx["pfad"]:
        eeprom_sichern(_ee_ctx["lib"], _ee_ctx["pfad"])

def eeprom_sichern(lib, pfad):
    arr = eeprom_array(lib)
    if arr is None:
        return
    try:
        with open(pfad, "wb") as f:
            f.write(bytes(arr))
    except OSError as e:
        print(f"[sim] EEPROM nicht schreibbar: {e}")

# ---------------------------------------------------------------------------
# ctypes interface
# ---------------------------------------------------------------------------
FB_ROW = ctypes.c_uint8 * 128
FB     = FB_ROW * 8

def load_dll(dll_path):
    os.add_dll_directory(os.path.dirname(GCC))
    lib = ctypes.CDLL(dll_path)

    fb      = FB.in_dll(lib, "sim_framebuffer")
    pina    = ctypes.c_uint8.in_dll(lib, "PINA_reg")
    ddra    = ctypes.c_uint8.in_dll(lib, "DDRA_reg")
    porta   = ctypes.c_uint8.in_dll(lib, "PORTA_reg")
    portc   = ctypes.c_uint8.in_dll(lib, "PORTC_reg")
    ddrc    = ctypes.c_uint8.in_dll(lib, "DDRC_reg")
    ocr1a   = ctypes.c_uint16.in_dll(lib, "OCR1A_reg")
    tccr1b  = ctypes.c_uint8.in_dll(lib, "TCCR1B_reg")

    lib.sim_call_isr.argtypes              = [ctypes.c_char_p]
    lib.sim_call_isr.restype               = None
    lib.main.argtypes                      = []
    lib.main.restype                       = ctypes.c_int
    lib.sim_soft_reset.argtypes            = []
    lib.sim_soft_reset.restype             = None
    lib.display_draw_sprite_reset.argtypes = []
    lib.display_draw_sprite_reset.restype  = None

    # Zeichenfenster ermitteln.
    # 1. Wahl: die Getter aus display_draw.c — die liefern das Fenster, das
    #    display_draw_init() tatsaechlich gesetzt hat, und zwar auch dann noch,
    #    wenn es spaeter im Programm neu gesetzt wird.
    # 2. Wahl: Konstanten display_width/-height/-x_pos/-y_pos in main.c.
    getters = {}
    for _name in ("display_draw_get_x_pos", "display_draw_get_y_pos",
                  "display_draw_get_width", "display_draw_get_height"):
        try:
            _fn = getattr(lib, _name)
        except AttributeError:
            getters = None
            break
        _fn.argtypes = []
        _fn.restype  = ctypes.c_uint8
        getters[_name] = _fn

    def _u8(name):
        try:
            return ctypes.c_uint8.in_dll(lib, name).value
        except (OSError, ValueError):
            return None

    dw = _u8("display_width");  dh = _u8("display_height")
    dx = _u8("display_x_pos"); dy = _u8("display_y_pos")
    const_rect = (dx, dy, dw, dh) if None not in (dw, dh, dx, dy) else None

    def _reg(name):
        try:
            return ctypes.c_uint8.in_dll(lib, name)
        except (ValueError, OSError):
            return None

    tccr2 = _reg("TCCR2_reg")
    ocr2r = _reg("OCR2_reg")
    timsk = _reg("TIMSK_reg")

    def draw_rect_fn():
        """Aktuelles Zeichenfenster als (x, y, w, h) oder None."""
        if getters:
            w = getters["display_draw_get_width"]()
            h = getters["display_draw_get_height"]()
            if w and h:
                return (getters["display_draw_get_x_pos"](),
                        getters["display_draw_get_y_pos"](), w, h)
            return None   # display_draw_init() noch nicht gelaufen
        return const_rect

    return (lib, fb, pina, ddra, porta, portc, ddrc, ocr1a, tccr1b,
            draw_rect_fn, tccr2, ocr2r, timsk)

# ---------------------------------------------------------------------------
# Thread utilities
# ---------------------------------------------------------------------------
def terminate_thread(thread):
    """Windows: Thread per TerminateThread sofort beenden.
    Nur sicher wenn der Thread keine Locks haelt (z.B. busy-wait)."""
    if not thread.is_alive():
        return
    THREAD_TERMINATE = 0x0001
    handle = ctypes.windll.kernel32.OpenThread(THREAD_TERMINATE, False, thread.native_id)
    if handle:
        ctypes.windll.kernel32.TerminateThread(handle, 0)
        ctypes.windll.kernel32.CloseHandle(handle)

# ---------------------------------------------------------------------------
# Timer ISR thread
# ---------------------------------------------------------------------------
PRESCALERS = {0: 0, 1: 1, 2: 8, 3: 64, 4: 256, 5: 1024}

def isr_thread(lib, ocr1a, tccr1b, default_fps, frame_event, stop_event):
    """Fire TIMER1_COMPA_vect at the rate set by OCR1A / TCCR1B."""
    isr_name = b"TIMER1_COMPA_vect"
    while not stop_event.is_set():
        cs        = tccr1b.value & 0x07
        prescaler = PRESCALERS.get(cs, 0)
        ocr       = ocr1a.value

        if prescaler > 0 and ocr > 0:
            period = (ocr + 1) * prescaler / 12_000_000
        else:
            period = 1.0 / default_fps

        if stop_event.wait(timeout=period):
            break
        lib.sim_call_isr(isr_name)
        frame_event.set()   # pygame signalisieren: neuer Frame bereit

# ---------------------------------------------------------------------------
# Timer2 und Ton
# ---------------------------------------------------------------------------
# Timer2 hat auf dem ATmega16 eine andere Vorteilerleiter als Timer0 und
# Timer1, deshalb eine eigene Tabelle.
PRESCALERS_T2 = {0: 0, 1: 1, 2: 8, 3: 32, 4: 64, 5: 128, 6: 256, 7: 1024}
PRESCALERS_T0 = {0: 0, 1: 1, 2: 8, 3: 64, 4: 256, 5: 1024}

F_CPU_SIM = 12_000_000
TON_RATE = 22050


def isr_thread_t2(lib, tccr2, ocr2, timsk, stop_event):
    """Loest TIMER2_COMP_vect aus, solange OCIE2 gesetzt ist.

    Damit laeuft die Ablaufsteuerung von sound.c auch im Simulator. Der
    Takt ergibt sich wie auf der Hardware aus Vorteiler und OCR2.
    """
    isr_name = b"TIMER2_COMP_vect"
    faellig_ab = time.perf_counter()
    while not stop_event.is_set():
        teiler = PRESCALERS_T2.get(tccr2.value & 0x07, 0)
        aktiv = bool(timsk.value & (1 << 7))          # OCIE2
        if not teiler or not aktiv:
            faellig_ab = time.perf_counter()
            if stop_event.wait(timeout=0.05):
                break
            continue

        # Nicht je Tick schlafen: Windows weckt Python nur etwa alle 15 ms,
        # eine Ablaufsteuerung mit 100 Hz liefe dadurch rund ein Drittel zu
        # langsam. Stattdessen grob warten und so viele Ausloesungen
        # nachholen, wie seither faellig geworden sind. Der Durchschnitt
        # stimmt damit genau, nur die Notengrenzen wackeln um wenige
        # Millisekunden, und das hoert niemand.
        periode = (ocr2.value + 1) * teiler / F_CPU_SIM
        jetzt = time.perf_counter()
        faellig = int((jetzt - faellig_ab) / periode) + 1
        for _ in range(min(faellig, 64)):
            lib.sim_call_isr(isr_name)
        faellig_ab += faellig * periode
        if faellig_ab < jetzt - 0.25:                 # zu weit hinterher
            faellig_ab = jetzt
        if stop_event.wait(timeout=min(0.004, periode)):
            break


def _rechteck(f, rate=TON_RATE, kanaele=1, lautstaerke=5000):
    """Eine Rechteckwelle aus ganzen Perioden, damit sie nahtlos loopt.

    Rate und Kanalzahl muessen zu dem passen, womit der Mixer tatsaechlich
    laeuft. pygame.init() richtet ihn oft schon in Stereo ein, und ein
    Monopuffer wuerde dann als Stereo gelesen: eine Oktave zu tief und
    verzerrt.
    """
    import struct
    periode = max(2, int(round(rate / f)))
    perioden = max(1, -(-1024 // periode))
    hoch = struct.pack('<h', lautstaerke) * kanaele
    tief = struct.pack('<h', -lautstaerke) * kanaele
    halb = periode // 2
    return (hoch * halb + tief * (periode - halb)) * perioden


def ton_thread(lib, stop_event):
    """Macht den Piepser hoerbar.

    sound.c laesst Timer0 im CTC-Modus mit umschaltendem Ausgang laufen. Die
    Tonhoehe steht in OCR0, der Vorteiler in den unteren drei Bit von TCCR0,
    alles null heisst Ruhe. Dieser Faden liest beide Register und spielt den
    passenden Rechteckton als Dauerschleife, bis sich etwas aendert.
    """
    import pygame
    try:
        if pygame.mixer.get_init() is None:
            pygame.mixer.init(frequency=TON_RATE, size=-16, channels=1, buffer=512)
        rate, groesse, kanaele = pygame.mixer.get_init()
        kanal = pygame.mixer.Channel(0)
    except Exception as e:
        print(f"[sim] Kein Ton moeglich: {e}")
        return
    if abs(groesse) != 16:
        print(f"[sim] Kein Ton: Mixer laeuft mit {groesse} Bit statt 16.")
        return

    try:
        tccr0 = ctypes.c_uint8.in_dll(lib, "TCCR0_reg")
        ocr0 = ctypes.c_uint8.in_dll(lib, "OCR0_reg")
    except (ValueError, OSError):
        return                                   # Projekt benutzt keinen Ton

    vorrat = {}
    aktuell = object()
    while not stop_event.is_set():
        cs = tccr0.value & 0x07
        schluessel = (cs, ocr0.value) if cs else None
        if schluessel != aktuell:
            aktuell = schluessel
            if schluessel is None:
                kanal.stop()
            else:
                if schluessel not in vorrat:
                    teiler = PRESCALERS_T0.get(cs, 0)
                    f = F_CPU_SIM / (2 * teiler * (1 + schluessel[1])) if teiler else 0
                    vorrat[schluessel] = (
                        pygame.mixer.Sound(buffer=_rechteck(f, rate, kanaele))
                        if 20.0 <= f <= 8000.0 else None)
                klang = vorrat[schluessel]
                if klang is None:
                    kanal.stop()
                else:
                    kanal.play(klang, loops=-1)
        if stop_event.wait(timeout=0.002):
            break
    kanal.stop()

# ---------------------------------------------------------------------------
# Pygame renderer
# ---------------------------------------------------------------------------
OLED_W, OLED_H = 128, 64
PANEL_H        = 70
OLED_BORDER    = 4              # Randbreite in Screen-Pixeln (ausserhalb der OLED-Flaeche)

COL_BORDER     = (30, 80, 220)  # blauer Rand um das OLED-Display
COL_BG         = (5,  12,  5)   # Bereich ausserhalb der Zeichenflaeche
COL_BG_DRAW    = (10, 22, 10)   # aktive Zeichenflaeche (leicht heller)
COL_PIXEL_ON      = (80, 255, 120)
COL_PIXEL_OUTSIDE = (255, 120, 0)   # Diagnose: Pixel ausserhalb des Zeichenfensters
COL_PIXEL_OFF     = (15, 30, 15)
COL_PANEL      = (20, 20, 20)
COL_LED_ON     = (255, 50,  50)   # rot, Ausgang korrekt konfiguriert
COL_LED_WEAK   = (100, 20,  20)   # dunkelrot, Pin als Eingang (schwacher Pull-up-Strom)
COL_LED_OFF    = (60,  60,  60)
COL_BTN_ON     = (255, 80, 80)
COL_BTN_OFF    = (50, 50, 50)
COL_RESET_BTN  = (180, 60, 60)
COL_RESET_HOV  = (220, 80, 80)
COL_TEXT       = (160, 160, 160)

# Keyboard -> button bit in PINA (active-low: press clears the bit)
KEY_BUTTON = {
    pygame.K_1:     0,  # S0
    pygame.K_2:     1,  # S1
    pygame.K_3:     2,  # S2
    pygame.K_4:     3,  # S3
    # Pfeiltasten wie auf der MEGACARD angeordnet
    pygame.K_RIGHT: 0,  # PA0
    pygame.K_LEFT:  1,  # PA1
    pygame.K_DOWN:  2,  # PA2
    pygame.K_UP:    3,  # PA3
}

_reported_outside = set()   # Jede Streupixel-Position nur einmal ausgeben

def fenstergroesse(scale):
    """(Breite, Hoehe) des Fensters: Anzeige mit Rand, darunter das Panel."""
    return (OLED_W * scale + 2 * OLED_BORDER,
            OLED_H * scale + 2 * OLED_BORDER + PANEL_H)

def render_oled(surf, fb, scale, draw_rect=None):
    """draw_rect = (x, y, w, h) in OLED-Pixel der aktiven Zeichenflaeche."""
    surf.fill(COL_BG)
    # Aktive Zeichenflaeche leicht abheben
    if draw_rect:
        dx, dy, dw, dh = draw_rect
        surf.fill(COL_BG_DRAW, (dx * scale, dy * scale, dw * scale, dh * scale))
    for page in range(8):
        for col in range(128):
            byte = fb[page][col]
            if byte == 0:
                continue
            for bit in range(8):
                if byte & (1 << bit):
                    px = col                   # physikalische OLED-Spalte
                    py = page * 8 + bit        # physikalische OLED-Zeile
                    # Prüfen ob Pixel innerhalb des Zeichenfensters liegt
                    if draw_rect:
                        dx, dy, dw, dh = draw_rect
                        in_window = (dx <= px < dx + dw) and (dy <= py < dy + dh)
                        col_px = COL_PIXEL_ON if in_window else COL_PIXEL_OUTSIDE
                        if not in_window:
                            key = (px, py)
                            if key not in _reported_outside:
                                _reported_outside.add(key)
                                wx = px - dx   # Fenster-Koordinate X
                                wy = py - dy   # Fenster-Koordinate Y
                                print(f"[DIAG] Streupixel: phys({px},{py})  "
                                      f"fenster({wx},{wy})  "
                                      f"page={page} col={col} bit={bit}")
                    else:
                        col_px = COL_PIXEL_ON
                    surf.fill(col_px, (px * scale, py * scale, scale, scale))

def render_panel(surf, pina_val, portc_val, ddrc_val, fps, project_name, scale, panel_w):
    surf.fill(COL_PANEL)
    font_sm = pygame.font.SysFont("consolas", 11)
    font_md = pygame.font.SysFont("consolas", 13, bold=True)

    # ---- LEDs (PORTC bits 0-7) ----
    led_r  = 10
    led_y  = PANEL_H // 2
    led_x0 = 10
    for i in range(8):
        cx   = led_x0 + (7 - i) * (led_r * 2 + 6)   # L7 links, L0 rechts
        mask = 1 << i
        on   = bool(portc_val & mask)
        if not on:
            col = COL_LED_OFF
        elif ddrc_val & mask:
            col = COL_LED_ON      # Ausgang -> volle Helligkeit
        else:
            col = COL_LED_WEAK    # Eingang -> Pull-up, schwacher Strom
        pygame.draw.circle(surf, col, (cx, led_y), led_r)
        lbl = font_sm.render(f"L{i}", True, COL_TEXT)
        surf.blit(lbl, (cx - lbl.get_width() // 2, led_y + led_r + 1))

    # ---- Buttons S3-S0 (S3 links, S0 rechts) ----
    btn_labels = ["S3\n4/↓", "S2\n3/↑", "S1\n2/→", "S0\n1/←"]
    btn_w, btn_h = 44, 28
    btn_x0 = panel_w - 4 * (btn_w + 4) - 4
    btn_y0 = (PANEL_H - btn_h) // 2
    for i, lbl in enumerate(btn_labels):
        pressed = not bool(pina_val & (1 << (3 - i)))
        col     = COL_BTN_ON if pressed else COL_BTN_OFF
        rx      = btn_x0 + i * (btn_w + 4)
        pygame.draw.rect(surf, col, (rx, btn_y0, btn_w, btn_h), border_radius=4)
        txt = font_sm.render(lbl.split("\n")[0], True, (255, 255, 255))
        sub = font_sm.render(lbl.split("\n")[1], True, (200, 200, 200))
        surf.blit(txt, (rx + (btn_w - txt.get_width()) // 2, btn_y0 + 3))
        surf.blit(sub, (rx + (btn_w - sub.get_width()) // 2, btn_y0 + 14))

    # ---- Reset-Button (Mitte) ----
    rst_w, rst_h = 60, 22
    rst_x = panel_w // 2 - rst_w // 2
    rst_y = (PANEL_H - rst_h) // 2
    reset_rect = pygame.Rect(rst_x, rst_y, rst_w, rst_h)
    pygame.draw.rect(surf, COL_RESET_BTN, reset_rect, border_radius=4)
    rst_lbl = font_md.render("RESET", True, (255, 255, 255))
    surf.blit(rst_lbl, (rst_x + (rst_w - rst_lbl.get_width()) // 2,
                        rst_y + (rst_h - rst_lbl.get_height()) // 2))

    # ---- FPS + project name ----
    info = font_md.render(f"{project_name}   {fps:.1f} fps", True, COL_TEXT)
    surf.blit(info, (panel_w // 2 - info.get_width() // 2, 4))

    return reset_rect

def run_pygame(lib, fb, pina, ddra, porta, portc, ddrc, ocr1a, tccr1b,
               project_name, scale, game_fps, frame_event, draw_rect_fn=None,
               dll_path=None, ee_pfad=None, tccr2=None, ocr2r=None, timsk=None):
    pygame.init()
    pygame.display.set_caption(f"megasim {VERSION} — {project_name}")

    panel_w, _  = fenstergroesse(scale)
    screen      = pygame.display.set_mode(fenstergroesse(scale))
    oled_surf   = pygame.Surface((OLED_W * scale, OLED_H * scale))
    panel_surf  = pygame.Surface((panel_w, PANEL_H))

    clock    = pygame.time.Clock()
    fps_disp = game_fps
    reset_rect_abs = None

    # C main() starten
    t_main = threading.Thread(target=lib.main, daemon=True)
    t_main.start()
    time.sleep(0.15)

    # ISR-Thread starten
    stop_isr = threading.Event()
    threading.Thread(
        target=isr_thread,
        args=(lib, ocr1a, tccr1b, game_fps, frame_event, stop_isr),
        daemon=True,
    ).start()
    if tccr2 is not None:
        threading.Thread(target=isr_thread_t2,
                         args=(lib, tccr2, ocr2r, timsk, stop_isr), daemon=True).start()
    threading.Thread(target=ton_thread, args=(lib, stop_isr), daemon=True).start()

    # Ohne Timer1-ISR (z.B. Selftest, reine Polling-Programme) kommt nie ein
    # frame_event. Deshalb hoechstens 1/fps warten, sonst laufen Anzeige und
    # Tastatureingabe nur im 2-Frames-pro-Sekunde-Takt.
    frame_period = 1.0 / max(game_fps, 1)

    last_rect = None
    while True:
        frame_event.wait(timeout=frame_period)
        frame_event.clear()

        # ---- Events ----
        do_reset = False
        for ev in pygame.event.get():
            if ev.type == pygame.QUIT:
                if ee_pfad: eeprom_sichern(lib, ee_pfad)
                pygame.quit()
                os._exit(0)
            if ev.type == pygame.KEYDOWN:
                if ev.key in (pygame.K_ESCAPE, pygame.K_q):
                    if ee_pfad: eeprom_sichern(lib, ee_pfad)
                    pygame.quit()
                    os._exit(0)
                if ev.key == pygame.K_r:
                    do_reset = True
                if ev.key in KEY_BUTTON:
                    bit = KEY_BUTTON[ev.key]
                    mask = 1 << bit
                    if not (ddra.value & mask) and (porta.value & mask):
                        pina.value &= ~mask
            if ev.type == pygame.KEYUP:
                if ev.key in KEY_BUTTON:
                    bit = KEY_BUTTON[ev.key]
                    mask = 1 << bit
                    if not (ddra.value & mask) and (porta.value & mask):
                        pina.value |= mask
            if ev.type == pygame.MOUSEBUTTONDOWN and ev.button == 1:
                if reset_rect_abs and reset_rect_abs.collidepoint(ev.pos):
                    do_reset = True

        if do_reset:
            # Reset:
            # 1. ISR-Thread stoppen
            stop_isr.set()
            time.sleep(0.05)
            # 2. Alten main()-Thread beenden (haengt in while(!frame_ready) ohne Lock)
            terminate_thread(t_main)
            # 3. DLL neu laden — setzt ALLE globalen C-Variablen (Structs, Zaehler)
            #    auf ihre Compile-Zeit-Startwerte zurueck, genau wie ein MCU-Reset
            #    den RAM-Abschnitt .data aus dem Flash neu kopiert.
            if dll_path:
                # Echte Hardware verliert beim Reset kein EEPROM. Das Neuladen
                # der DLL setzt das Array aber zurueck, also vorher wegkopieren.
                ee_inhalt = eeprom_array(lib)
                ee_inhalt = bytes(ee_inhalt) if ee_inhalt is not None else None
                del fb, pina, ddra, porta, portc, ddrc, ocr1a, tccr1b
                # FreeLibrary explizit mit richtigem Typ aufrufen.
                # Nur del lib reicht nicht: alte Thread-Args-Tuples halten
                # noch eine Python-Referenz auf lib → refcount bleibt > 0 →
                # Windows entlaedt die DLL nicht → LoadLibrary gibt dieselbe
                # (unveraenderte) Instanz zurueck.
                _k32 = ctypes.windll.kernel32
                _k32.FreeLibrary.argtypes = [ctypes.c_void_p]
                _k32.FreeLibrary.restype  = ctypes.c_bool
                _k32.FreeLibrary(lib._handle)
                del lib   # CDLL.__del__ ruft FreeLibrary nochmal auf - harmlos
                (lib, fb, pina, ddra, porta, portc, ddrc, ocr1a, tccr1b,
                 draw_rect_fn, tccr2, ocr2r, timsk) = load_dll(dll_path)
                if ee_inhalt is not None:
                    arr = eeprom_array(lib)
                    if arr is not None:
                        for i, b in enumerate(ee_inhalt):
                            arr[i] = b
                _ee_ctx["lib"] = lib
            else:
                # Fallback ohne dll_path: nur Register/Framebuffer zuruecksetzen
                lib.display_draw_sprite_reset()
                lib.sim_soft_reset()
            pina.value = 0xFF
            _reported_outside.clear()
            # 4. Neue Threads starten
            frame_event = threading.Event()
            stop_isr    = threading.Event()
            t_main = threading.Thread(target=lib.main, daemon=True)
            t_main.start()
            time.sleep(0.15)
            threading.Thread(
                target=isr_thread,
                args=(lib, ocr1a, tccr1b, game_fps, frame_event, stop_isr),
                daemon=True,
            ).start()
            if tccr2 is not None:
                threading.Thread(target=isr_thread_t2,
                                 args=(lib, tccr2, ocr2r, timsk, stop_isr), daemon=True).start()
            threading.Thread(target=ton_thread, args=(lib, stop_isr), daemon=True).start()
            fps_disp = game_fps
            continue

        # ---- Render ----
        draw_rect = draw_rect_fn() if draw_rect_fn else None
        if draw_rect != last_rect:
            # Fenster hat sich geaendert -> alte Streupixel-Meldungen verwerfen
            _reported_outside.clear()
            last_rect = draw_rect
        render_oled(oled_surf, fb, scale, draw_rect)
        panel_reset_rect = render_panel(
            panel_surf, pina.value, portc.value, ddrc.value, fps_disp, project_name, scale, panel_w
        )
        oled_top = OLED_BORDER
        panel_top = OLED_H * scale + 2 * OLED_BORDER
        reset_rect_abs = panel_reset_rect.move(0, panel_top)

        # Rand als Hintergrundfarbe (OLED liegt INNEN, ueberdeckt den Rand nicht)
        screen.fill(COL_BORDER)
        screen.blit(oled_surf,  (OLED_BORDER, oled_top))
        screen.blit(panel_surf, (0, panel_top))
        pygame.display.flip()

        clock.tick(game_fps)
        fps_disp = clock.get_fps()

# ---------------------------------------------------------------------------
# Launcher-Watchdog
# ---------------------------------------------------------------------------
# Microchip Studio startet das External Tool als
#     cmd.exe /k "megasim.exe" "<Projektordner>"
# Der Stopp-Knopf ruft TerminateProcess auf — aber nur fuer die cmd.exe.
# TerminateProcess ist nicht abfangbar und erreicht Kindprozesse gar nicht,
# es kommt also auch kein Signal an. Der Simulator lief bisher verwaist
# weiter, hielt die gebaute DLL gesperrt (Linkerfehler beim naechsten Start)
# und musste im Task-Manager beendet werden.
#
# Abhilfe: die startenden Prozesse ueberwachen und mitsterben. Beobachtet
# werden nur der PyInstaller-Bootloader (gleicher Programmname) und eine
# cmd.exe darueber. Bei anderen Eltern — Explorer, Shell, IDE — passiert
# nichts, damit ein normaler Start nicht sofort wieder abbricht.

# Der eigene Programmname kommt aus sys.executable, damit ein Umbenennen der
# exe den Watchdog nicht still abschaltet.
_WATCHED_PARENTS = {"cmd.exe", "megasim.exe"}
if getattr(sys, "frozen", False):
    _WATCHED_PARENTS.add(os.path.basename(sys.executable).lower())

def _process_table():
    """{pid: (ppid, exe_name)} ueber CreateToolhelp32Snapshot."""
    TH32CS_SNAPPROCESS = 0x00000002
    MAX_PATH = 260

    class PROCESSENTRY32(ctypes.Structure):
        _fields_ = [("dwSize", ctypes.c_ulong),
                    ("cntUsage", ctypes.c_ulong),
                    ("th32ProcessID", ctypes.c_ulong),
                    ("th32DefaultHeapID", ctypes.POINTER(ctypes.c_ulong)),
                    ("th32ModuleID", ctypes.c_ulong),
                    ("cntThreads", ctypes.c_ulong),
                    ("th32ParentProcessID", ctypes.c_ulong),
                    ("pcPriClassBase", ctypes.c_long),
                    ("dwFlags", ctypes.c_ulong),
                    ("szExeFile", ctypes.c_char * MAX_PATH)]

    k32 = ctypes.windll.kernel32
    k32.CreateToolhelp32Snapshot.restype = ctypes.c_void_p
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    if snap in (None, -1, 0xFFFFFFFFFFFFFFFF):
        return {}

    table = {}
    entry = PROCESSENTRY32()
    entry.dwSize = ctypes.sizeof(PROCESSENTRY32)
    try:
        ok = k32.Process32First(ctypes.c_void_p(snap), ctypes.byref(entry))
        while ok:
            table[entry.th32ProcessID] = (
                entry.th32ParentProcessID,
                entry.szExeFile.decode("mbcs", "replace").lower(),
            )
            ok = k32.Process32Next(ctypes.c_void_p(snap), ctypes.byref(entry))
    finally:
        k32.CloseHandle(ctypes.c_void_p(snap))
    return table


def _launcher_pids():
    """PIDs der startenden Prozesse, von unten nach oben."""
    table = _process_table()
    pids  = []
    pid   = os.getpid()
    seen  = {pid}
    while len(pids) < 8:
        entry = table.get(pid)
        if not entry:
            break
        ppid, _ = entry
        parent = table.get(ppid)
        if not parent or ppid in seen:
            break
        if parent[1] not in _WATCHED_PARENTS:
            break          # Explorer, Shell, IDE: nicht ueberwachen
        pids.append(ppid)
        seen.add(ppid)
        pid = ppid
    return pids


def watch_launcher():
    """Simulator beenden, sobald ein startender Prozess verschwindet."""
    pids = _launcher_pids()
    if not pids:
        return

    SYNCHRONIZE = 0x00100000
    k32 = ctypes.windll.kernel32
    k32.OpenProcess.restype = ctypes.c_void_p
    handles = []
    for pid in pids:
        h = k32.OpenProcess(SYNCHRONIZE, False, pid)
        if h:
            handles.append(ctypes.c_void_p(h))
    if not handles:
        return

    def _wait():
        arr = (ctypes.c_void_p * len(handles))(*handles)
        k32.WaitForMultipleObjects(len(handles), arr, False, 0xFFFFFFFF)
        eeprom_sichern_jetzt()
        os._exit(0)

    threading.Thread(target=_wait, daemon=True).start()


def install_signal_handlers():
    """Strg+C und Strg+Untbr sofort beenden, ohne Traceback."""
    def _bye(signum, frame):
        eeprom_sichern_jetzt()
        os._exit(0)
    for name in ("SIGINT", "SIGBREAK", "SIGTERM"):
        sig = getattr(signal, name, None)
        if sig is not None:
            try:
                signal.signal(sig, _bye)
            except (ValueError, OSError):
                pass


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    # Microchip Studio leitet die Ausgabe in sein Ausgabefenster um. Ohne
    # Zeilenpufferung erschiene sie dort erst in Bloecken oder beim Beenden.
    try:
        sys.stdout.reconfigure(line_buffering=True)
    except (AttributeError, ValueError):
        pass
    if "--version" not in sys.argv:
        print(f"[sim] megasim {VERSION}")

    parser = argparse.ArgumentParser(description=f"megasim {VERSION} — MegaCard Simulator")
    parser.add_argument("project_dir", help="Directory containing main.c")
    parser.add_argument("--scale", type=int, default=4, help="Display scale (default 4)")
    parser.add_argument("--fps",   type=int, default=15, help="Spielrate / ISR-Fallback FPS (default 15)")
    parser.add_argument("--version", action="version", version=f"megasim {VERSION}")
    args = parser.parse_args()

    install_signal_handlers()
    watch_launcher()

    project_dir  = os.path.abspath(args.project_dir.rstrip('"\\/'))
    project_name = os.path.basename(project_dir)

    if not os.path.isdir(project_dir):
        sys.exit(f"[sim] Not a directory: {project_dir}")

    os.makedirs(BUILD_DIR, exist_ok=True)
    dll_path = pick_dll_path(BUILD_DIR, project_name)

    # Aufteilung des Ladebalkens: das einmalige Einrichten der GCC dauert
    # deutlich laenger als das Uebersetzen und bekommt, wenn noetig, den
    # groesseren Teil.
    anzeige = Ladeanzeige(args.scale, project_name)
    gcc_fehlt = gcc_muss_ausgepackt_werden()
    anzeige.bereich(0.0, 0.7 if gcc_fehlt else 0.0)
    gcc_bereitstellen(anzeige)
    anzeige.bereich(0.7 if gcc_fehlt else 0.0, 0.95)
    build(project_dir, dll_path, anzeige)
    anzeige.bereich(0.95, 1.0)
    anzeige.zeigen(0.0, "starte", sofort=True)
    (lib, fb, pina, ddra, porta, portc, ddrc, ocr1a, tccr1b,
     draw_rect_fn, tccr2, ocr2r, timsk) = load_dll(dll_path)
    anzeige.zeigen(1.0, "starte", sofort=True)

    ee_pfad = eeprom_datei(project_dir)
    eeprom_laden(lib, ee_pfad)
    _ee_ctx["lib"], _ee_ctx["pfad"] = lib, ee_pfad

    frame_event = threading.Event()

    # Run pygame — startet main()- und ISR-Thread intern, auch nach Reset
    run_pygame(lib, fb, pina, ddra, porta, portc, ddrc, ocr1a, tccr1b,
               project_name, args.scale, args.fps, frame_event, draw_rect_fn,
               dll_path=dll_path, ee_pfad=ee_pfad,
               tccr2=tccr2, ocr2r=ocr2r, timsk=timsk)

if __name__ == "__main__":
    main()
