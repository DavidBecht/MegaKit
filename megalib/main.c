/*-------------------------------------------------------------------------*\
| Datei:        main.c
| Version:      1.0
| Projekt:      Einfuehrung in das Programmieren des Atmega16
| Beschreibung: Auswahl des Programms. Sonst steht hier nichts.
|               Zum Umschalten nur die Zeile mit PROGRAMM aendern.
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
| Erstellung:   11.09.2026
|
| Aenderung:
\*-------------------------------------------------------------------------*/

// Auswahlmoeglichkeiten
#define LAMA_DEMO       1   // demos/lama_demo.c     Sprite-Animation mit Lama
#define TRAFFIC_RACER   2   // demos/traffic_racer/  Videospiel
#define SELFTEST        3   // demos/selftest.c      Selbsttest der Bibliothek
#define GRAFIK_DEMO     4   // demos/grafik_demo.c   Alle Zeichenfunktionen
#define TONTEST         5   // demos/tontest.c       Tonleiter fuer den Piezo


// ===========================================================================
//
//   HIER UMSCHALTEN:  LAMA_DEMO | TRAFFIC_RACER | SELFTEST | GRAFIK_DEMO | TONTEST
//
#define PROGRAMM   TRAFFIC_RACER
//
// ===========================================================================


// Die drei Programme. Jedes laeuft endlos und kehrt nicht zurueck.
// Nicht gewaehlte Programme wirft der Linker beim Bauen wieder hinaus,
// sie kosten also weder Flash noch RAM.
void lama_demo_run(void);
void traffic_racer_run(void);
void selftest_run(void);
void grafik_demo_run(void);
void tontest_run(void);

// Startet das oben gewaehlte Programm. Das return am Ende wird nie
// erreicht, es steht nur da, weil main einen Rueckgabewert hat.
int main(void)
{
#if   PROGRAMM == LAMA_DEMO
	lama_demo_run();
#elif PROGRAMM == TRAFFIC_RACER
	traffic_racer_run();
#elif PROGRAMM == SELFTEST
	selftest_run();
#elif PROGRAMM == GRAFIK_DEMO
	grafik_demo_run();
#elif PROGRAMM == TONTEST
	tontest_run();
#else
#error "PROGRAMM muss LAMA_DEMO, TRAFFIC_RACER, SELFTEST, GRAFIK_DEMO oder TONTEST sein."
#endif

	return 0;
}
