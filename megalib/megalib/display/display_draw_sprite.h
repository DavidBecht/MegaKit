/*-------------------------------------------------------------------------*\
| Datei:        display_draw_sprite.h
| Version:      1.1
| Projekt:      Zeichenbibliothek fuer die MEGACARD
| Beschreibung: Verwaltung bewegter Bilder (Sprites) ueber display_draw.
|               Bewegung, Animation, Kollision und Neuzeichnen je Bild.
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
| Erstellung:   30.04.2026
|
| Aenderung:    Doku vereinheitlicht und vervollstaendigt
\*-------------------------------------------------------------------------*/

#ifndef DISPLAY_DRAW_SPRITE_H_
#define DISPLAY_DRAW_SPRITE_H_

#include <stdint.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

#include "display_draw.h"   // BITMAP_T


/**
 * @brief Hoechstzahl gleichzeitig registrierter Sprites.
 *
 * @warning Der Wert darf 8 nicht ueberschreiten. display_draw_sprite_update_all()
 *          verwaltet je Sprite ein Bit in Masken vom Typ uint8_t. Ab dem
 *          neunten Sprite entfallen diese Bits, der Sprite wird weder
 *          geloescht noch gezeichnet, und es erfolgt keine Meldung. Fuer eine
 *          hoehere Zahl sind die vier Masken in der Implementierung auf
 *          uint16_t zu erweitern.
 */
#define MAX_SPRITES 8

/**
 * @brief Moegliche Bildraten fuer display_draw_sprite_init().
 *
 * Der Wert entspricht der Zahl der Bilder je Sekunde. Hoehere Raten erhoehen
 * die Rechenlast, da je Bild alle betroffenen Sprites geloescht und neu
 * gezeichnet werden.
 */
typedef enum
{
	FPS_15 = 15,
	FPS_25 = 25,
	FPS_30 = 30,
	FPS_60 = 60,
} SPRITE_FPS_T;

// Vorausdeklaration, damit der Callback SPRITE_T* verwenden kann
typedef struct SPRITE_T_tag SPRITE_T;

/**
 * @brief Der innere Zustand eines Sprites. Gehoert der Bibliothek.
 *
 * Diese Felder werden ausschliesslich von display_draw_sprite.c gelesen und
 * geschrieben. Sie sind hier aufgefuehrt, damit die Groesse eines Sprites
 * bekannt ist und Sprites als gewoehnliche Variablen angelegt werden koennen.
 *
 * Schreibzugriffe von aussen sind unzulaessig: die Verwaltung fuehrt hier
 * Buch darueber, was zuletzt auf das Display gezeichnet wurde, um es spaeter
 * wieder entfernen zu koennen.
 */
typedef struct {
	int16_t prev_x;          ///< zuletzt gezeichnete Position
	int8_t  prev_y;
	uint8_t current_frame;   ///< gerade sichtbares Bild
	uint8_t prev_frame;      ///< zuletzt gezeichnetes Bild
	uint8_t counter;         ///< Bilder seit dem letzten Animationsschritt
	uint8_t frame_interval;  ///< Bilder je Animationsschritt, max. 255
	uint8_t spawn_interval;  ///< spawn_delay_ms in Bilder, max. 255
	uint8_t spawn_timer;     ///< Countdown bis zum naechsten Spawn
	uint8_t anim_loop_count; ///< abgeschlossene Durchlaeufe
} SPRITE_INTERN_T;

/**
 * @brief Ein bewegtes Bild.
 *
 * Bis auf das abschliessende Feld intern, das der Bibliothek vorbehalten ist,
 * sind alle Felder vom Anwendungsprogramm zu setzen. Ueblich ist die
 * Initialisierung mit benannten Feldern; nicht genannte Felder werden mit null
 * vorbelegt, einschliesslich des inneren Zustands:
 *
 *     SPRITE_T auto_sprite = {
 *         .x = 20, .y = 2,
 *         .frames = Auto_Bilder, .frame_count = 2,
 *         .animation_ms = 200,      // beide Bilder zusammen in 200 ms
 *     };
 *
 * Breite, Hoehe und Ablageart entfallen; sie werden beim Registrieren aus dem
 * ersten Bild uebernommen.
 */
struct SPRITE_T_tag
{
	// --- Ort und Bewegung ------------------------------------------------
	// x ist 16 Bit, damit ein Sprite von rechts hereinfahren und links
	// hinauslaufen kann. y reicht mit 8 Bit, das Display ist 64 Pixel hoch.
	int16_t x;
	int8_t  y;

	// Geschwindigkeit in Pixel pro Bild (0 = steht still)
	int8_t vx;
	int8_t vy;

	// --- Aussehen --------------------------------------------------------
	// Liste der Animationsbilder und deren Anzahl.
	// Die Einzelbilder werden mit BITMAP_VERTIKAL oder BITMAP_HORIZONTAL
	// angelegt, die Liste verweist auf sie. Liste und Bilder liegen im
	// Programmspeicher und belegen kein SRAM:
	//
	//     BITMAP_VERTIKAL(Lama_1, Lama_1_daten, 48, 48);
	//     ...
	//     const BITMAP_T * const Lama_Bilder[] PROGMEM = { &Lama_1, &Lama_2 };
	//     SPRITE_T lama = { .frames = Lama_Bilder, .frame_count = 2, ... };
	const BITMAP_T * const *frames;
	uint8_t frame_count;

	// Dauer eines vollstaendigen Animationsdurchlaufs in Millisekunden,
	// nicht die Standzeit eines Einzelbildes: 800 bedeutet, dass alle
	// frame_count Bilder zusammen 800 ms in Anspruch nehmen. Die Angabe bleibt
	// damit bei geaenderter Bildanzahl gueltig.
	//
	// Die Umrechnung in Bilder je Animationsschritt erfolgt einmalig beim
	// Registrieren. Spaetere Aenderungen werden erst nach erneutem
	// display_draw_sprite_register() wirksam.
	//
	// Das Ergebnis der Umrechnung ist ganzzahlig und betraegt mindestens eins.
	// Sehr kurze Animationen dauern daher laenger als angegeben: drei Bilder in
	// 150 ms ergeben bei 15 Bildern je Sekunde 0,75 und damit aufgerundet ein
	// Bild je Schritt, also 200 ms Gesamtdauer. Bei 30 Bildern je Sekunde geht
	// dieselbe Angabe exakt auf.
	uint16_t animation_ms;

	// Nur lesen: Groesse des Sprites in Pixel.
	// Wird beim Registrieren aus dem ersten Bild uebernommen und dient den
	// Kollisions- und Bereichspruefungen als zwischengespeicherter Wert.
	uint8_t width;
	uint8_t height;

	// --- Schalter --------------------------------------------------------
	// Die vier Schalter belegen gemeinsam ein einzelnes Byte als Bitfeld und
	// muessen deshalb beisammen bleiben. Zugriff wie auf ein gewoehnliches
	// bool; die Adresse eines Bitfelds ist nicht bildbar.

	// true  = nur die gesetzten Pixel zeichnen, der Untergrund bleibt
	//         sichtbar. Fuer Sprites, die ueber anderen liegen sollen,
	//         zum Beispiel eine Explosion ueber dem Fahrzeug.
	// false = Standard. Das ganze Rechteck wird ueberschrieben, die Nullpixel
	//         des Bildes loeschen also, was darunter liegt.
	bool transparent : 1;

	// Nur lesen: gibt an, ob der Sprite gezeichnet wird.
	// Gesetzt wird das Feld von register, kill und activate. Wird es von aussen
	// zurueckgesetzt, entfaellt sowohl das Zeichnen als auch die Wiederherstellung
	// nach dem Loeschen durch andere Sprites. Zum Anhalten dient paused, zum
	// Entfernen display_draw_sprite_kill().
	bool active : 1;

	// Nur lesen: gibt an, ob der Sprite angehalten ist.
	// Umgeschaltet wird ueber display_draw_sprite_pause().
	bool paused : 1;

	// [intern] erzwingt das Neuzeichnen beim naechsten Update. Nicht setzen.
	// Das Feld steht hier statt in intern, da ein einzelnes Bitfeld dort ein
	// zusaetzliches Byte belegen wuerde.
	bool needs_draw : 1;

	// --- Neustart nach dem Verlassen des Bildbereichs --------------------
	// Position beim (Re)Spawn, nur relevant wenn spawn_delay_ms > 0
	int16_t respawn_x;
	int8_t  respawn_y;

	// Wartezeit in ms bis zum erneuten Erscheinen nach Verlassen des
	// Bildbereichs. 0 unterdrueckt das automatische Erscheinen, der Sprite
	// bleibt dann inaktiv. Im Unterschied zu animation_ms handelt es sich um
	// eine einzelne Dauer, die nicht durch die Bildanzahl geteilt wird. Die
	// Umrechnung erfolgt beim Registrieren.
	uint16_t spawn_delay_ms;

	// --- Wiederholungen und Rueckruf -------------------------------------
	// Wie oft die Animation wiederholt wird
	// 0 = unendlich (Standard)
	// 1 = einmal abspielen, dann automatisch stoppen
	// 2 = zweimal abspielen, dann automatisch stoppen, ...
	// Der Sprite wird nach Ablauf automatisch deaktiviert (kein Callback noetig).
	uint8_t anim_loops;

	// Optionaler Rueckruf nach dem automatischen Stop (NULL = keiner).
	// Wird NACH der automatischen Deaktivierung aufgerufen.
	// Verwendung z.B.: naechste Animation starten, Score erhoehen, Sound ausloesen.
	void (*on_anim_done)(SPRITE_T *s);

	// --- Der Bibliothek vorbehalten --------------------------------------
	SPRITE_INTERN_T intern;
};


/**
 * @brief Richtet Timer1 als Bildtakt ein.
 *
 * Konfiguriert Timer1 im CTC-Modus fuer die angegebene Bildrate und schaltet
 * den zugehoerigen Interrupt frei. Muss einmal vor der Spielschleife
 * aufgerufen werden, nach display_draw_init().
 *
 * @param fps Gewuenschte Bildrate.
 *
 * @note Schaltet die globalen Interrupts ein (sei()). Timer1 gehoert danach
 *       dem Sprite-System. Timer0 und Timer2 bleiben frei, die braucht die
 *       Tonausgabe in sound.h.
 * @note Der Vorteiler wird zur Bildrate passend gewaehlt, damit der
 *       Vergleichswert den 16-Bit-Bereich von OCR1A nicht ueberschreitet.
 */
void display_draw_sprite_init(SPRITE_FPS_T fps);

/**
 * @brief Registriert einen Sprite und bereitet seinen internen Zustand vor.
 *
 * Rechnet animation_ms und spawn_delay_ms in Bilder um, setzt den Sprite auf
 * aktiv und nimmt ihn in die Zeichenliste auf. In die Umrechnung von
 * animation_ms geht frame_count ein, denn die Angabe gilt fuer den ganzen
 * Durchlauf.
 *
 * Die Reihenfolge der Registrierung ist die Zeichenreihenfolge und damit die
 * Ebene: zuerst registriert liegt hinten, zuletzt registriert liegt vorn. Also
 * von hinten nach vorn registrieren, z.B. erst die Strasse, dann das Fahrzeug,
 * zuletzt Effekte wie eine Explosion.
 *
 * @param s Zeiger auf den Sprite. Bleibt in der Liste, darf also nicht auf dem
 *          Stack einer Funktion liegen, die spaeter verlassen wird.
 *
 * @warning Sind bereits MAX_SPRITES Sprites registriert, bleibt der Aufruf
 *          ohne Wirkung und ohne Meldung; der Sprite wird nicht gezeichnet.
 * @warning Gleiches gilt fuer unvollstaendig belegte Sprites ohne frames oder
 *          ohne frame_count. Die Pruefung erfolgt hier, da der Fehler sonst
 *          erst beim ersten Bildaufbau und fern seiner Ursache auftraete.
 * @warning animation_ms und spawn_delay_ms werden genau hier umgerechnet. Wer
 *          sie spaeter aendert, muss erneut registrieren, sonst bleibt die
 *          alte Geschwindigkeit stehen. width, height, frames, vx und vy
 *          duerfen dagegen jederzeit geaendert werden.
 */
void display_draw_sprite_register(SPRITE_T *s);

/**
 * @brief Leert die Zeichenliste.
 *
 * Danach kennt die Verwaltung keinen Sprite mehr und zeichnet keinen mehr.
 * Gedacht fuer den Wechsel in einen anderen Spielabschnitt mit anderen Sprites.
 *
 * @warning Der Displayinhalt bleibt unveraendert, bis display_draw_clear()
 *          aufgerufen wird.
 * @warning Der Zustand der Sprites selbst wird nicht zurueckgesetzt; Position
 *          und Feld active bleiben erhalten. Vor erneuter Registrierung
 *          derselben Sprites ist display_draw_sprite_activate() aufzurufen, um
 *          eine definierte Startposition herzustellen.
 */
void display_draw_sprite_reset(void);

/**
 * @brief Prueft, ob sich zwei Sprites beruehren.
 *
 * Verglichen werden die umgebenden Rechtecke aus x, y, width und height (AABB),
 * nicht die tatsaechlich gesetzten Pixel. Zwei Sprites koennen sich also in den
 * Rechtecken ueberschneiden, ohne dass sich ihre Bilder beruehren.
 *
 * @param a Erster Sprite.
 * @param b Zweiter Sprite.
 * @return true bei Ueberschneidung, false wenn einer der beiden inaktiv ist.
 */
bool display_draw_sprite_collides(const SPRITE_T *a, const SPRITE_T *b);

/**
 * @brief Wie display_draw_sprite_collides(), liefert zusaetzlich den Treffpunkt.
 *
 * Der Treffpunkt ist die Mitte der Flaeche, in der sich die beiden Rechtecke
 * ueberschneiden. Ohne Treffer bleiben x und y unveraendert.
 *
 * Gedacht fuer Effekte, die an der Beruehrungsstelle erscheinen sollen, etwa
 * eine Explosion. Da die Ueberschneidung im Moment des Zusammenstosses nur
 * wenige Pixel breit ist, liegt der Punkt praktisch auf der Beruehrungskante,
 * egal ob vorne, hinten oder seitlich getroffen wurde.
 *
 * @param a Erster Sprite.
 * @param b Zweiter Sprite.
 * @param x Ziel fuer die X-Koordinate des Treffpunkts, darf NULL sein.
 * @param y Ziel fuer die Y-Koordinate des Treffpunkts, darf NULL sein.
 * @return true bei Ueberschneidung, sonst false.
 *
 * @note Gemeint sind auch hier die Rechtecke, nicht die gesetzten Pixel.
 */
bool display_draw_sprite_collision_point(const SPRITE_T *a, const SPRITE_T *b,
                                         int16_t *x, int8_t *y);

/**
 * @brief Deaktiviert einen Sprite sofort und loescht ihn vom Display.
 *
 * Startet ausserdem den Spawn-Countdown, wenn spawn_delay_ms gesetzt ist.
 * Bei einem bereits inaktiven Sprite passiert nichts.
 *
 * @param s Zeiger auf den Sprite.
 */
void display_draw_sprite_kill(SPRITE_T *s);

/**
 * @brief Aktiviert einen Sprite an der angegebenen Position.
 *
 * Setzt die Animation auf das erste Bild zurueck und hebt ein gesetztes
 * Anhalten wieder auf. Gedacht fuer Sprites, die von Hand gespawnt werden,
 * etwa eine Explosion bei einer Kollision.
 *
 * @param s Zeiger auf den Sprite.
 * @param x Neue X-Position im Zeichenfenster.
 * @param y Neue Y-Position. Wird auf 8 Bit gekuerzt, Werte ausserhalb von
 *          -128 bis 127 sind sinnlos.
 */
void display_draw_sprite_activate(SPRITE_T *s, int16_t x, int16_t y);

/**
 * @brief Haelt einen Sprite an oder laesst ihn weiterlaufen.
 *
 * Angehalten ruhen Bewegung, Animation und ein laufender Respawn-Countdown.
 * Das Bild bleibt stehen, wo es steht. Der Sprite bleibt aktiv: er wird weiter
 * gezeichnet und bleibt heil, wenn ein anderer Sprite ueber ihn hinwegloescht.
 * Beim Weiterlaufen macht die Animation dort weiter, wo sie angehalten wurde.
 *
 * Nicht mit active verwechseln. Ein inaktiver Sprite verschwindet vom Display
 * und wird nicht mehr gezeichnet, ein angehaltener bleibt sichtbar.
 *
 * @param s     Zeiger auf den Sprite.
 * @param pause true = anhalten, false = weiterlaufen lassen.
 *
 * @note display_draw_sprite_activate() hebt ein Anhalten ebenfalls auf.
 */
void display_draw_sprite_pause(SPRITE_T *s, bool pause);

/**
 * @brief Rechnet ein Bild weiter und zeichnet alle Sprites neu.
 *
 * Bewegt und animiert jeden Sprite, prueft die Bildgrenzen, zeichnet alles
 * Noetige neu und uebertraegt es zum Schluss auf das Display. Ein eigener
 * Aufruf von display_draw_show() ist deshalb nicht noetig. Sprites, die den
 * Bildbereich verlassen, werden automatisch deaktiviert.
 *
 * Ein Bild entsteht in vier Phasen. Zuerst wird nur der Zustand fortgeschrieben
 * und vorgemerkt, wer neu gezeichnet werden muss. Dann wird geprueft, welche
 * weiteren Sprites von den freigeraeumten Rechtecken mitgetroffen werden; die
 * kommen dazu. Erst danach wird geloescht, und ganz zuletzt gezeichnet.
 *
 * Weil alle Loeschvorgaenge vor allen Zeichenvorgaengen laufen, kann kein
 * Sprite mehr Pixel eines anderen wegnehmen, und ueberlappende Sprites liegen
 * zuverlaessig in der Reihenfolge ihrer Registrierung. Im Spielcode ist dafuer
 * nichts zu tun, insbesondere kein Setzen von needs_draw.
 *
 * @param blocking true  = wartet, bis der Frame-Timer das naechste Bild
 *                         freigibt, und haelt so die Bildrate ein.
 *                 false = kehrt sofort zurueck, wenn das naechste Bild noch
 *                         nicht faellig ist. Fuer Schleifen, die nebenher noch
 *                         etwas anderes erledigen sollen.
 *
 * @note Nicht abgedeckt ist alles, was das Spiel selbst zeichnet, etwa ein
 *       Rahmen oder Text. Davon weiss die Sprite-Verwaltung nichts, das muss
 *       das Spiel bei Bedarf weiterhin selbst nachziehen.
 */
void display_draw_sprite_update_all(bool blocking);


#endif /* DISPLAY_DRAW_SPRITE_H_ */
