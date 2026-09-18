#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sim_api.h"

/* ---- Zeitfunktionen aus kernel32 ----
   Statt windows.h einzubinden, werden die drei benoetigten Funktionen hier
   selbst deklariert. windows.h zieht rund 250 weitere Header nach, die die
   mitgelieferte GCC sonst enthalten muesste. Gelinkt wird wie bisher gegen
   kernel32, die Signaturen entsprechen der Windows-API. */
typedef int64_t zaehler_t;          /* LARGE_INTEGER, nur QuadPart genutzt */

__declspec(dllimport) void __stdcall Sleep(unsigned long ms);
__declspec(dllimport) int  __stdcall QueryPerformanceCounter(zaehler_t *wert);
__declspec(dllimport) int  __stdcall QueryPerformanceFrequency(zaehler_t *wert);

/* ---- Framebuffer ---- */
volatile uint8_t sim_framebuffer[8][128] = {{0}};

/* ---- Hardware registers ---- */
volatile uint8_t  PINA_reg  = 0xFF; /* pull-ups active: all 1 = no buttons */
volatile uint8_t  DDRA_reg  = 0,    PORTA_reg = 0;
volatile uint8_t  PINB_reg  = 0,    DDRB_reg  = 0, PORTB_reg = 0;
volatile uint8_t  PINC_reg  = 0,    DDRC_reg  = 0, PORTC_reg = 0;
volatile uint8_t  PIND_reg  = 0,    DDRD_reg  = 0, PORTD_reg = 0;
volatile uint8_t  TCCR0_reg  = 0,   OCR0_reg   = 0,   TCNT0_reg  = 0;
volatile uint8_t  TCCR1A_reg = 0,   TCCR1B_reg = 0;
volatile uint16_t OCR1A_reg  = 0;
volatile uint8_t  TCCR2_reg  = 0,   OCR2_reg   = 0;
volatile uint8_t  TIMSK_reg  = 0,   TIFR_reg   = 0;
volatile uint8_t  ADMUX_reg  = 0,   ADCSRA_reg = 0;
volatile uint16_t ADCW_reg   = 0;
volatile uint8_t  ADCH_reg   = 0,   ADCL_reg   = 0;

/* ---- EEPROM ----
   Die Zeiger, die das Programm uebergibt, sind auf dem AVR schlichte
   Adressen von 0 bis 511. Genau so werden sie hier als Index benutzt. */
volatile uint8_t sim_eeprom[SIM_EEPROM_SIZE];

/* Ein frischer Baustein ist mit 0xFF gefuellt. Laeuft beim Laden der DLL,
   also auch nach einem Reset. sim.py schreibt danach den gesicherten
   Inhalt wieder hinein. */
__attribute__((constructor))
static void _ee_init(void)
{
    memset((void *)sim_eeprom, 0xFF, sizeof(sim_eeprom));
}

static uint16_t _ee_index(const void *addr)
{
    return (uint16_t)((uintptr_t)addr & (SIM_EEPROM_SIZE - 1));
}

uint8_t eeprom_read_byte(const uint8_t *addr)
{
    return sim_eeprom[_ee_index(addr)];
}

uint16_t eeprom_read_word(const uint16_t *addr)
{
    uint16_t i = _ee_index(addr);
    return (uint16_t)(sim_eeprom[i] | (sim_eeprom[(i + 1) & (SIM_EEPROM_SIZE - 1)] << 8));
}

void eeprom_write_byte(uint8_t *addr, uint8_t value)
{
    sim_eeprom[_ee_index(addr)] = value;
}

void eeprom_write_word(uint16_t *addr, uint16_t value)
{
    uint16_t i = _ee_index(addr);
    sim_eeprom[i] = (uint8_t)(value & 0xFF);
    sim_eeprom[(i + 1) & (SIM_EEPROM_SIZE - 1)] = (uint8_t)(value >> 8);
}

void eeprom_update_byte(uint8_t *addr, uint8_t value)
{
    if (eeprom_read_byte(addr) != value) eeprom_write_byte(addr, value);
}

void eeprom_update_word(uint16_t *addr, uint16_t value)
{
    if (eeprom_read_word(addr) != value) eeprom_write_word(addr, value);
}

/* ---- ISR registry ---- */
#define MAX_ISR 16
static struct {
    const char     *name;
    sim_isr_func_t  fn;
} _isr_table[MAX_ISR];
static int _isr_count = 0;

void sim_register_isr(const char *name, sim_isr_func_t f)
{
    if (_isr_count < MAX_ISR) {
        _isr_table[_isr_count].name = name;
        _isr_table[_isr_count].fn   = f;
        _isr_count++;
    }
}

void sim_call_isr(const char *name)
{
    for (int i = 0; i < _isr_count; i++) {
        if (strcmp(_isr_table[i].name, name) == 0) {
            _isr_table[i].fn();
            return;
        }
    }
}

/* ---- Delays: echte Wartezeit, damit Animationen wie auf der Hardware laufen ----
   Ohne Warten laeuft z.B. eine _delay_ms(80)-Animation mit CPU-Vollgas und der
   Renderer sieht nur zufaellige Zwischenzustaende. */
void sim_delay_ms(uint32_t ms)
{
    if (ms == 0) return;

    /* Sleep() rundet unter Windows auf den Takt des Systemzeitgebers auf, das
       sind ueblich 15,6 ms. Ein _delay_ms(1) in einer Schleife liefe damit
       rund fuenfzehnmal zu langsam, und Animationen kriechen.

       Deshalb zweigeteilt: der grobe Teil wird geschlafen, der Rest aktiv
       abgewartet. Kurze Wartezeiten laufen ganz im aktiven Warten. Das kostet
       Rechenzeit, entspricht aber genau dem, was _delay_ms auf dem AVR tut. */
    zaehler_t freq, start, now;
    if (!QueryPerformanceFrequency(&freq) || freq == 0)
    {
        Sleep(ms);
        return;
    }
    QueryPerformanceCounter(&start);
    const zaehler_t ziel = (freq * (zaehler_t)ms) / 1000;

    if (ms > 20)
    {
        Sleep(ms - 15);               /* grob vorschlafen, Rest unten */
    }

    do {
        QueryPerformanceCounter(&now);
    } while (now - start < ziel);
}

void sim_delay_us(uint32_t us)
{
    if (us == 0) return;
    if (us >= 2000) { Sleep(us / 1000); return; }

    /* Unter 2 ms ist Sleep() zu grob -> aktives Warten mit dem
       Hochaufloesungs-Zaehler. */
    zaehler_t freq, start, now;
    if (!QueryPerformanceFrequency(&freq) || freq == 0) return;
    QueryPerformanceCounter(&start);
    const zaehler_t ticks = (freq * (zaehler_t)us) / 1000000;
    do {
        QueryPerformanceCounter(&now);
    } while (now - start < ticks);
}

/* ---- Soft-Reset: Framebuffer + Register, ISR-Tabelle bleibt erhalten ---- */
void sim_soft_reset(void)
{
    memset((void *)sim_framebuffer, 0, sizeof(sim_framebuffer));
    PINA_reg   = 0xFF;  /* Pull-ups aktiv */
    DDRA_reg   = 0;    PORTA_reg  = 0;
    PINB_reg   = 0;    DDRB_reg   = 0;    PORTB_reg  = 0;
    PINC_reg   = 0;    DDRC_reg   = 0;    PORTC_reg  = 0;
    PIND_reg   = 0;    DDRD_reg   = 0;    PORTD_reg  = 0;
    TCCR0_reg  = 0;    OCR0_reg   = 0;    TCNT0_reg  = 0;
    TCCR1A_reg = 0;    TCCR1B_reg = 0;    OCR1A_reg  = 0;
    TCCR2_reg  = 0;    OCR2_reg   = 0;
    TIMSK_reg  = 0;    TIFR_reg   = 0;
    ADMUX_reg  = 0;    ADCSRA_reg = 0;    ADCW_reg   = 0;
    ADCH_reg   = 0;    ADCL_reg   = 0;
    /* _isr_count wird NICHT zurueckgesetzt */
}

