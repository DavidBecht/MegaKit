#ifndef FAKE_AVR_PGMSPACE_H_
#define FAKE_AVR_PGMSPACE_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Strip PROGMEM — data lives in normal RAM on x86 */
#define PROGMEM
#define PROGMEM_NEAR
#define PROGMEM_FAR
#define PGM_P       const char *
#define PGM_VOID_P  const void *
#define PSTR(s)     (s)

/* pgm_read_* — just dereference the pointer on x86
 *
 * ACHTUNG, Stolperstelle: einen ZEIGER, der selbst im PROGMEM liegt, immer
 * mit pgm_read_ptr lesen, niemals mit pgm_read_word. Auf dem AVR ist ein
 * Zeiger zwei Byte gross, dort sind beide gleich und der Fehler faellt nicht
 * auf. Hier ist ein Zeiger acht Byte gross, pgm_read_word schneidet also die
 * oberen sechs Byte ab. Das Ergebnis ist eine Adresse wie 0x000000000000B2E0
 * und der Simulator stirbt mit
 *     OSError: exception: access violation reading 0x...
 */
#define pgm_read_byte(addr)  (*(const uint8_t  *)(addr))
#define pgm_read_word(addr)  (*(const uint16_t *)(addr))
#define pgm_read_dword(addr) (*(const uint32_t *)(addr))
#define pgm_read_float(addr) (*(const float    *)(addr))
#define pgm_read_ptr(addr)   (*(const void * const *)(addr))

/* String/memory helpers */
#define memcpy_P  memcpy
#define memmove_P memmove
#define memcmp_P  memcmp
#define memchr_P  memchr
#define strlen_P  strlen
#define strnlen_P strnlen
#define strcpy_P  strcpy
#define strncpy_P strncpy
#define strcat_P  strcat
#define strncat_P strncat
#define strcmp_P  strcmp
#define strncmp_P strncmp
#define strchr_P  strchr
#define strrchr_P strrchr
#define strstr_P  strstr
#define sprintf_P   sprintf
#define snprintf_P  snprintf
#define vsprintf_P  vsprintf
#define vsnprintf_P vsnprintf
#define printf_P    printf
#define fprintf_P   fprintf

#endif /* FAKE_AVR_PGMSPACE_H_ */
