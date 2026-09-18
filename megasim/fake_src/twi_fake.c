/*
 * twi_fake.c — Fake software I2C that parses the SSD1306 command stream
 * and writes pixel data into sim_framebuffer instead of real I2C pins.
 *
 * SSD1306 PAGE addressing mode protocol:
 *   ctrl 0x80 + cmd  -> command byte (page addr, col addr, etc.)
 *   ctrl 0x40        -> switch to data mode; all following bytes are pixels
 */
#include <stdint.h>
#include "sim_api.h"

typedef enum { ST_CTRL, ST_CMD, ST_DATA } twi_state_t;

static twi_state_t _state = ST_CTRL;
static uint8_t     _page  = 0;
static uint8_t     _col   = 0;

void twi_s_init(void) {}

uint8_t twi_s_start(uint8_t address)
{
    (void)address;
    _state = ST_CTRL;   /* reset state on every new transaction */
    return 1;           /* ACK */
}

void twi_s_stop(void)
{
    _state = ST_CTRL;
}

uint8_t twi_s_write(uint8_t data)
{
    switch (_state) {

    case ST_CTRL:
        if      (data == 0x80) _state = ST_CMD;    /* next byte = command */
        else if (data == 0x40) _state = ST_DATA;   /* following bytes = pixel data */
        break;

    case ST_CMD:
        /* SSD1306 page-address commands */
        if      ((data & 0xF8) == 0xB0)            /* 0xB0..0xB7 — set page */
            _page = data & 0x07;
        else if ((data & 0xF0) == 0x00)            /* 0x00..0x0F — lower col nibble */
            _col  = (_col & 0xF0) | (data & 0x0F);
        else if ((data & 0xF0) == 0x10)            /* 0x10..0x1F — upper col nibble */
            _col  = (_col & 0x0F) | ((data & 0x0F) << 4);
        /* all other commands (init sequence, contrast, etc.) are ignored */
        _state = ST_CTRL;   /* back to control-byte mode after every command */
        break;

    case ST_DATA:
        if (_page < 8 && _col < 128) {
            sim_framebuffer[_page][_col] = data;
            _col++;
            if (_col >= 128) _col = 0;  /* wrap column */
        }
        /* stay in DATA mode until twi_s_stop() */
        break;
    }
    return 1; /* ACK */
}

uint8_t twi_s_read(uint8_t ack)
{
    (void)ack;
    return 0;
}
