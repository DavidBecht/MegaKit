/*-------------------------------------------------------------------------*\
| Datei:        display.c
| Version:      1.0
| Projekt:      Display-Bibliothek fuer die MEGACARD
| Beschreibung: Bibliotheksfunktionen (Implementierung)
| Schaltung:    MEGACARD V6.11, OLED an PB0 (SCL) und PB1 (SDA)
| Autor:        D.I. Leopold Moosbrugger
| Erstellung:   3.3.2022
|
| Aenderung:    Doku vereinheitlicht, const bei display_string_pos
\*-------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include "./twi/twi_soft.h"  // Sofware TWI-Schnittstelle
#include "./font/FontData.h"  // Font-Informationen
#include "display.h" // Schnittstellendefinitionen

// Lokale Makros
#define OLED_CTRL_BYTE_CMD  0x80 // 0b1000 0000
#define OLED_CTRL_BYTE_DATA 0x40 // 0b0100 0000
#define OLED_DEV_ADDR 0x78       // 0b0111 1000

#define DISP_LINES (OLED_PIXEL_Y/fontParam.height)
#define DISP_COLS (OLED_PIXEL_X/(fontParam.width+fontParam.spacing))

#define  OLED_SEND_CMD(value)  {twi_s_write(OLED_CTRL_BYTE_CMD); twi_s_write((value));} 

// Modullokale Variablen
static uint8_t d_offset = 0;

void display_init(void)
{
   // Initialisierung der SW-I2C Schnittstelle
   twi_s_init();
   
   // Initialisierungssequenzen senden
   // Siehe Application Note [SOLOMON SYSTECH]
   // http://www.solomon-systech.com

   twi_s_start(OLED_DEV_ADDR);
   OLED_SEND_CMD(0xA8); // Set Mux Ratio
   OLED_SEND_CMD(0x3F); // S32 (R)

   OLED_SEND_CMD(0xD3); // Set Display offset 
   OLED_SEND_CMD(0x00); // S33 (R)

   OLED_SEND_CMD(0x40); // Set Segment Start Line S32 (R)

   OLED_SEND_CMD(0xA0); // Set Segment re-map S32 (R)

   OLED_SEND_CMD(0xC0); // Set COM Output Scan Direction S32 (R)

   OLED_SEND_CMD(0xDA); // Set COM Pins hardware configuration
   OLED_SEND_CMD(0x12); // S33 (R)

   OLED_SEND_CMD(0x81); // Set Contrast Control 
   OLED_SEND_CMD(0x7F); // S27 (R)

   OLED_SEND_CMD(0xA4); // Disable Entre Display On S27 (R)

   OLED_SEND_CMD(0xA6); // Set Normal Display S27 (R)

   OLED_SEND_CMD(0xD5); // Set Osc Frequency
   OLED_SEND_CMD(0x80); // S33 (R)

   OLED_SEND_CMD(0x20); // Page addressing Mode
   OLED_SEND_CMD(0x02); // S31 (R)

   OLED_SEND_CMD(0x8D); // Enable charge pump regulator 
   OLED_SEND_CMD(0x14); // during display on 7.5V S34 (R)
   twi_s_stop();

   display_clear();

   twi_s_start(OLED_DEV_ADDR);
   OLED_SEND_CMD(0xAF); // Display ON S27
   twi_s_stop();

   // Modullokale Variablen initialisieren
   d_offset = 0;
}

uint8_t display_lines (void)
{
   return DISP_LINES;
}

uint8_t display_chars (void)
{
   return DISP_COLS;
}

uint8_t display_char_first (void)
{
   return fontParam.char_first;
}

uint8_t display_char_last (void)
{
   return fontParam.char_last;
}

void display_scroll_up (void)
{
   uint8_t ic;

   // oberste Zeile loeschen
   display_pos(0, 0);

   twi_s_start(OLED_DEV_ADDR);
   twi_s_write(OLED_CTRL_BYTE_DATA);
   // gesamte Page loeschen
   for(ic=0; ic<OLED_PIXEL_X; ic++)
   {
      twi_s_write(0x00);
   }
   twi_s_stop();

   d_offset += 1;
   if (d_offset >= DISP_LINES)
      d_offset -= DISP_LINES;

   twi_s_start(OLED_DEV_ADDR);
   OLED_SEND_CMD(0xD3); // Set Display offset
   OLED_SEND_CMD(d_offset<<3); // S33 
   twi_s_stop();
}

void display_scroll_down (void)
{
   uint8_t ic;

   // unterste Zeile loeschen
   display_pos(0, DISP_LINES-1);

   twi_s_start(OLED_DEV_ADDR);
   twi_s_write(OLED_CTRL_BYTE_DATA);
   // gesamte Page loeschen
   for(ic=0; ic<OLED_PIXEL_X; ic++)
   {
      twi_s_write(0x00);
   }
   twi_s_stop();

   if (d_offset > 0)
   {
      d_offset--;
   } else
   {
      d_offset = DISP_LINES - 1;
   }

   twi_s_start(OLED_DEV_ADDR);
   OLED_SEND_CMD(0xD3); // Set Display offset
   OLED_SEND_CMD(d_offset<<3); // S33
   twi_s_stop();
}
void display_pos(uint8_t col, uint8_t row)
{
   uint8_t pixel_col;

   // ungueltige Parameter abfangen
   if ((row < DISP_LINES) && (col < DISP_COLS)) 
   {
	   pixel_col = col * (fontParam.width+fontParam.spacing);

      // Zeilenadresse anpassen
      row += d_offset;
      if (row >= DISP_LINES) 
         row -= DISP_LINES;

	   twi_s_start(OLED_DEV_ADDR);
	   // Seitenadresse setzen
	   OLED_SEND_CMD((uint8_t)(0xB0 | row));   // Set Page Start Address

	   // unteres Nibble der Col-Addresse ((col*8) & 0x0F)
	   OLED_SEND_CMD(pixel_col & 0x0F);

	   // oberes Nibble der Col-Addresse ((col*8/16) & 0x0F)
	   OLED_SEND_CMD(0x10 | ((pixel_col>>4) & 0x0F));

	   twi_s_stop();
   }
}

void display_clear(void)
{
   uint8_t ip, ic;
   
   // Alle Speicher Pages loeschen
   for(ip=0; ip<OLED_PAGES; ip++)
   {
      display_pos(0, ip);

	   twi_s_start(OLED_DEV_ADDR);
      twi_s_write(OLED_CTRL_BYTE_DATA);
      // gesamte Page loeschen	
      for(ic=0; ic<OLED_PIXEL_X; ic++)
      {
         twi_s_write(0x00);
      }
      twi_s_stop();
   }

   // Cursor in Position 0, 0
   display_pos(0, 0);
}

void display_char (uint8_t c)
{
   uint8_t cnt;
   uint16_t f_index;
   
   // Ungueltige Zeichen abfangen (ersetzt durch das erste Zeichen des Satzes)
   if ((c < fontParam.char_first) || (c > fontParam.char_last)) 
      c = fontParam.char_first;
   
   // Index im Zeichensatz berechnen
   f_index = (c - fontParam.char_first) * fontParam.width;

   // Datenuebertragung zum Display beginnen
   twi_s_start(OLED_DEV_ADDR);
   twi_s_write(OLED_CTRL_BYTE_DATA);

   // Spalten des Zeichens ausgeben
   for (cnt=0; cnt<fontParam.width; cnt++)
   {
      twi_s_write(pgm_read_byte(&fontData[f_index++]));
   }

   // Spacing ausgeben
   for (cnt=0; cnt<fontParam.spacing; cnt++)
   {
      twi_s_write(0);
   }

   twi_s_stop();
}

void display_string(const char *line, bool from_progmem)
{
	while (1)
	{
		char c = from_progmem ? pgm_read_byte(line) : *line;
		if (c == '\0') break;

		display_char(c);
		line++;
	}
}

void display_string_pos (uint8_t posx, uint8_t posy, const char *line)
{
   display_pos(posx, posy); 
   display_string(line, false);
}

void display_string_pos_P(uint8_t posx, uint8_t posy, PGM_P line)
{
	display_pos(posx, posy);
	display_string(line, true);
}

/* Begrenzt eine Laenge auf den Wertebereich des Rueckgabetyps.

   vsnprintf liefert die Laenge, die der Text ohne Begrenzung haette, und damit
   moeglicherweise mehr als 127. Eine direkte Zuweisung an int8_t ergaebe einen
   negativen Wert und damit einen vorgetaeuschten Fehler. */
static int8_t _laenge_begrenzen(int laenge)
{
   if (laenge < 0)   return -1;
   if (laenge > 127) return 127;
   return (int8_t)laenge;
}

int8_t  display_printf(const char *fmt, ...)
{
   char buf[25];
   int c_count;                 // vsnprintf liefert int, nicht int8_t

   va_list args;
   va_start(args, fmt);
   c_count = vsnprintf(buf, sizeof buf, fmt, args);
   va_end(args);
   
   if (c_count >= 0)
      display_string(buf, false);
   return _laenge_begrenzen(c_count);
}

int8_t display_printf_pos_P(uint8_t posx, uint8_t posy, PGM_P fmt, ...)
{
   char buf[25];
   int c_count;                 // vsnprintf liefert int, nicht int8_t

   va_list args;
   va_start(args, fmt);
   c_count = vsnprintf_P(buf, sizeof buf, fmt, args);
   va_end(args);

   if (c_count >= 0)
      display_string_pos(posx, posy, buf);
   return _laenge_begrenzen(c_count);
}

int8_t display_printf_pos(uint8_t posx, uint8_t posy, const char *fmt, ...)
{
   char buf[25];
   int c_count;                 // vsnprintf liefert int, nicht int8_t
   
   va_list args;
   va_start(args, fmt);
   c_count = vsnprintf(buf, sizeof buf, fmt, args);
   va_end(args);
   
   if (c_count >= 0)
      display_string_pos(posx, posy, buf);
   return _laenge_begrenzen(c_count);
}

void display_pixel_byte (uint8_t pcol, uint8_t prow, uint8_t pbyte)
{
   if ((prow < OLED_PIXEL_Y) && (pcol < OLED_PIXEL_X))
   {
	  prow >>= 3; // prow / 8

	  twi_s_start(OLED_DEV_ADDR);
	  // Seitenadresse setzen
	  OLED_SEND_CMD((uint8_t)(0xB0 | prow));  // Set Page Start Address
      // unteres Nibble der Col-Addresse ((col*8) & 0x0F)
	  OLED_SEND_CMD(pcol & 0x0F);
	  // oberes Nibble der Col-Addresse ((col*8/16) & 0x0F)
	  OLED_SEND_CMD(0x10 | ((pcol>>4) & 0x0F));
	  twi_s_stop();

     twi_s_start(OLED_DEV_ADDR);
     twi_s_write(OLED_CTRL_BYTE_DATA);
	  twi_s_write(pbyte);
 	  twi_s_stop();
  }
}

void display_burst_start(uint8_t pcol, uint8_t prow)
{
   prow >>= 3;
   twi_s_start(OLED_DEV_ADDR);
   OLED_SEND_CMD(0xB0 | prow);
   OLED_SEND_CMD(pcol & 0x0F);
   OLED_SEND_CMD(0x10 | ((pcol >> 4) & 0x0F));
   twi_s_stop();
   twi_s_start(OLED_DEV_ADDR);
   twi_s_write(OLED_CTRL_BYTE_DATA);
}

void display_burst_write(uint8_t byte)
{
   twi_s_write(byte);
}

void display_burst_end(void)
{
   twi_s_stop();
}




