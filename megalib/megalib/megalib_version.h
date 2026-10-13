/*-------------------------------------------------------------------------*\
| Datei:        megalib_version.h
| Projekt:      megalib fuer die MEGACARD
| Beschreibung: Version der Bibliothek. Die einzige Stelle im Code, an der
|               eine Versionsnummer steht.
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
|
| Aenderung:    neu angelegt
\*-------------------------------------------------------------------------*/

#ifndef MEGALIB_VERSION_H_
#define MEGALIB_VERSION_H_

/**
 * @brief Version der megalib als Zeichenkette, zum Beispiel "1.0.3".
 *
 * Gesetzt wird der Wert nicht von Hand: Beim Release schreibt ihn release.py
 * hier hinein, in das Studio-Template schreibt ihn build_template.py. Ein
 * Template, das ohne Versionsangabe gebaut wurde, traegt "dev".
 *
 * Beim Uebersetzen meldet display.c die Version mit #pragma message in das
 * Ausgabefenster von Microchip Studio. Wer sie im Programm anzeigen will:
 *
 *     #include "megalib/megalib_version.h"
 *     display_string_pos_P(0, 7, PSTR(MEGALIB_VERSION));
 *
 */
#define MEGALIB_VERSION "1.0.3"

#endif /* MEGALIB_VERSION_H_ */
