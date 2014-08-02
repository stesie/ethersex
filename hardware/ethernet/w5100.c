/*
 * Copyright (c) 2014 by Stefan Siegl <stesie@brokenpipe.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <avr/io.h>

#include "config.h"
#include "core/debug.h"
#include "core/spi.h"
#include "hardware/ethernet/w5100.h"

#define cs_low()  PIN_CLEAR(SPI_CS_W5100)
#define cs_high() PIN_SET(SPI_CS_W5100)

#define W5100_MR_RST    7
#define W5100_MR_PB     4
#define W5100_MR_PPPOE  3
#define W5100_MR_AI     1
#define W5100_MR_IND    0

/* mode register */
#define w5100_write_mr(value)   w5100_write_uint8(0x0000, value)
#define w5100_read_mr()         w5100_read_uint8(0x0000)

#define w5100_write_imr(value)  w5100_write_uint8(0x0016, value)
#define w5100_read_imr()        w5100_read_uint8(0x0016)

#define w5100_write_rtr(value)  w5100_write_uint16(0x0017, value)
#define w5100_read_rtr()        w5100_read_uint16(0x0017)

#define w5100_write_rcr(value)  w5100_write_uint8(0x0019, value)
#define w5100_read_rcr()        w5100_read_uint8(0x0019)

#define w5100_write_rmsr(value) w5100_write_uint8(0x001A, value)
#define w5100_read_rmsr()       w5100_read_uint8(0x001A)

#define w5100_write_tmsr(value) w5100_write_uint8(0x001B, value)
#define w5100_read_tmsr()       w5100_read_uint8(0x001B)

static inline void
w5100_write_uint8(uint16_t address, uint8_t value)
{
  cs_low();

  spi_send(0xF0);
  spi_send(address >> 8);
  spi_send(address & 0xFF);
  spi_send(value);

  cs_high();
}

static inline uint8_t
w5100_read_uint8(uint16_t address)
{
  cs_low();

  spi_send(0x0F);
  spi_send(address >> 8);
  spi_send(address & 0xFF);
  uint8_t value = spi_send(0);

  cs_high();
  return value;
}

void
w5100_init(void)
{
  /* Soft-Reset W5100 */
  w5100_write_mr(_BV(W5100_MR_RST));

  /* Mask all interrupts, for the moment we're polling. */
  w5100_write_imr(0x00);

#ifdef DEBUG_W5100
  uint8_t rmsr_config = w5100_read_rmsr();
  debug_printf("w5100 rmsr config: 0x%02x\n", rmsr_config);
#endif

}

void
w5100_txstart(void)
{
}

/*
  -- Ethersex META --
  header(hardware/ethernet/w5100.h)
  net_init(w5100_init)
*/
