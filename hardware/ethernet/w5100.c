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
#include "protocols/uip/uip_router.h"

#define cs_low()  PIN_CLEAR(SPI_CS_W5100)
#define cs_high() PIN_SET(SPI_CS_W5100)

/* mode register */
#define w5100_write_mr(value)   w5100_write_uint8(0x0000, value)
#define w5100_read_mr()         w5100_read_uint8(0x0000)

#define W5100_MR_RST    7
#define W5100_MR_PB     4
#define W5100_MR_PPPOE  3
#define W5100_MR_AI     1
#define W5100_MR_IND    0

/* interrupt register */
#define w5100_write_ir(value)   w5100_write_uint8(0x0015, value)
#define w5100_read_ir()         w5100_read_uint8(0x0015)

#define W5100_IR_CONFLICT   7
#define W5100_IR_UNREACH    6
#define W5100_IR_PPPOE      5
#define W5100_IR_S3_INT     3
#define W5100_IR_S2_INT     2
#define W5100_IR_S1_INT     1
#define W5100_IR_S0_INT     0

/* interrupt mask register */
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


/*
 * macros for socket handling
 */
#define w5100_sock_write_uint8(sock, addr, value)   w5100_write_uint8(0x0400 + (0x0100 * sock) + addr, value)
#define w5100_sock_read_uint8(sock, addr)           w5100_read_uint8(0x0400 + (0x0100 * sock) + addr)

#define w5100_sock_write_uint16(sock, addr, value)  w5100_write_uint16(0x0400 + (0x0100 * sock) + addr, value)
#define w5100_sock_read_uint16(sock, addr)          w5100_read_uint16(0x0400 + (0x0100 * sock) + addr)

/* mode register */
#define w5100_sock_write_mr(sock, value)    w5100_sock_write_uint8(sock, 0x00, value)
#define w5100_sock_read_mr(sock)            w5100_sock_read_uint8(sock, 0x00)

/* command register */
#define w5100_sock_write_cr(sock, value)    w5100_sock_write_uint8(sock, 0x01, value)
#define w5100_sock_read_cr(sock)            w5100_sock_read_uint8(sock, 0x01)

#define W5100_SOCK_CR_OPEN          0x01
#define W5100_SOCK_CR_LISTEN        0x02
#define W5100_SOCK_CR_CONNECT       0x04
#define W5100_SOCK_CR_DISCON        0x08
#define W5100_SOCK_CR_CLOSE         0x10
#define W5100_SOCK_CR_SEND          0x20
#define W5100_SOCK_CR_SEND_MAC      0x21
#define W5100_SOCK_CR_SEND_KEEP     0x22
#define W5100_SOCK_CR_RECV          0x40

/* interrupt register */
#define w5100_sock_write_ir(sock, value)    w5100_sock_write_uint8(sock, 0x02, value)
#define w5100_sock_read_ir(sock)            w5100_sock_read_uint8(sock, 0x02)

#define W5100_SOCK_IR_SEND_OK       4
#define W5100_SOCK_IR_TIMEOUT       3
#define W5100_SOCK_IR_RECV          2
#define W5100_SOCK_IR_DISCON        1
#define W5100_SOCK_IR_CON           0

/* status register */
#define w5100_sock_write_sr(sock, value)    w5100_sock_write_uint8(sock, 0x03, value)
#define w5100_sock_read_sr(sock)            w5100_sock_read_uint8(sock, 0x03)

#define W5100_SOCK_SR_CLOSED        0x00
#define W5100_SOCK_SR_INIT          0x13
#define W5100_SOCK_SR_LISTEN        0x14
#define W5100_SOCK_SR_ESTABLISHED   0x17
#define W5100_SOCK_SR_CLOSE_WAIT    0x1C
#define W5100_SOCK_SR_UDP           0x22
#define W5100_SOCK_SR_IPRAW         0x32
#define W5100_SOCK_SR_MACRAW        0x42
#define W5100_SOCK_SR_PPPOE         0x5F
#define W5100_SOCK_SR_SYNSENT       0x15
#define W5100_SOCK_SR_SYNRECV       0x16
#define W5100_SOCK_SR_FIN_WAIT      0x18
#define W5100_SOCK_SR_CLOSING       0x1A
#define W5100_SOCK_SR_TIME_WAIT     0x1B
#define W5100_SOCK_SR_LAST_ACK      0x1D
#define W5100_SOCK_SR_ARP           0x11 /* or 0x21 or 0x22 */


#define w5100_sock_write_txfsr(sock, value) w5100_sock_write_uint16(sock, 0x20, value)
#define w5100_sock_read_txfsr(sock)         w5100_sock_read_uint16(sock, 0x20)

#define w5100_sock_write_txrd(sock, value)  w5100_sock_write_uint16(sock, 0x22, value)
#define w5100_sock_read_txrd(sock)          w5100_sock_read_uint16(sock, 0x22)

#define w5100_sock_write_txwr(sock, value)  w5100_sock_write_uint16(sock, 0x24, value)
#define w5100_sock_read_txwr(sock)          w5100_sock_read_uint16(sock, 0x24)

#define w5100_sock_write_rxrsr(sock, value) w5100_sock_write_uint16(sock, 0x26, value)
#define w5100_sock_read_rxrsr(sock)         w5100_sock_read_uint16(sock, 0x26)

#define w5100_sock_write_rxrd(sock, value)  w5100_sock_write_uint16(sock, 0x28, value)
#define w5100_sock_read_rxrd(sock)          w5100_sock_read_uint16(sock, 0x28)



#define gS0_RX_BASE                 0x6000
#define gS0_RX_MASK                 0x07FF

#define gS0_TX_BASE                 0x4000
#define gS0_TX_MASK                 0x07FF

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

static void
w5100_write_uint16(uint16_t address, uint16_t value)
{
  w5100_write_uint8(address, value >> 8);
  w5100_write_uint8(address + 1, value & 0xFF);
}

static uint16_t
w5100_read_uint16(uint16_t address)
{
  uint16_t res = w5100_read_uint8(address);
  res = (res << 8) + w5100_read_uint8(address + 1);
  return res;
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
  debug_printf("w5100: rmsr=0x%02x\n", rmsr_config);
#endif

start_macraw_mode:
  /* Enable MAC raw mode on socket 0. */
  w5100_sock_write_mr(0, 0x04);
  w5100_sock_write_cr(0, W5100_SOCK_CR_OPEN);

  if(w5100_sock_read_sr(0) != W5100_SOCK_SR_MACRAW)
  {
    w5100_sock_write_cr(0, W5100_SOCK_CR_CLOSE);
    goto start_macraw_mode;
  }

#ifdef DEBUG_W5100
  debug_printf("w5100: configured for mac raw mode\n");
#endif
}

void
w5100_process(void)
{
  uint8_t ir = w5100_read_ir();

  if(!ir)
    return;

#ifdef DEBUG_W5100
  debug_printf("w5100: ir=0x%02x\n", ir);
#endif

  if(ir & _BV(W5100_IR_S0_INT))
  {
    uint8_t sock_ir = w5100_sock_read_ir(0);
#ifdef DEBUG_W5100
    debug_printf("w5100: got interrupt on socket 0; s0ir=0x%02x\n", sock_ir);
#endif

    if(sock_ir & _BV(W5100_SOCK_IR_SEND_OK))
    {
#ifdef DEBUG_W5100
      debug_printf("w5100: packet send ok.\n");
#endif

      w5100_sock_write_ir(0, _BV(W5100_SOCK_IR_SEND_OK));
    }

    if(sock_ir & _BV(W5100_SOCK_IR_RECV))
    {
#ifdef DEBUG_W5100
      debug_printf("w5100: got packet.\n");
#endif

      uint16_t size = w5100_sock_read_rxrsr(0) - 2;

#ifdef DEBUG_W5100
      debug_printf("w5100: received %d bytes.\n", size);
#endif

      uint16_t rxrd = w5100_sock_read_rxrd(0) + 2;

      if(size < UIP_BUFSIZE)
      {
        /* calculate start address(physical address) */
        uint16_t address = gS0_RX_BASE + (rxrd & gS0_RX_MASK);

        uint8_t *ptr = uip_buf;
        for(uint16_t i = 0; i < size; i ++)
        {
          uint8_t byte = w5100_read_uint8(address);
          *ptr = byte;

          ptr ++;

          if(address == gS0_RX_BASE + gS0_RX_MASK)
            address = gS0_RX_BASE;
          else
            address ++;
        }
      }

      w5100_sock_write_rxrd(0, rxrd + size);
      w5100_sock_write_cr(0, W5100_SOCK_CR_RECV);
      w5100_sock_write_ir(0, _BV(W5100_SOCK_IR_RECV));

      if(size < UIP_BUFSIZE)
      {
        uip_len = size;
        ethernet_process_packet();
      }
    }
  }

  if(ir)
  {
    w5100_write_ir(ir);
  }
}

void
w5100_txstart(void)
{
#ifdef DEBUG_W5100
  debug_printf("w5100: txstart.\n");
#endif

  if(uip_len > gS0_TX_MASK)
  {
#ifdef DEBUG_W5100
    debug_printf("w5100: packet too large.\n");
#endif
    return;
  }

  uint16_t free_size;
  do
  {
    free_size = w5100_sock_read_txfsr(0);
  } while(free_size < uip_len);

  uint16_t txwr = w5100_sock_read_txwr(0);
  uint16_t address = gS0_TX_BASE + (txwr & gS0_TX_MASK);

  uint8_t *ptr = uip_buf;
  for(uint16_t i = 0; i < uip_len; i ++)
  {
    w5100_write_uint8(address, *ptr);
    ptr ++;

    if(address == gS0_TX_BASE + gS0_TX_MASK)
      address = gS0_TX_BASE;
    else
      address ++;
  }

  txwr += uip_len;
  w5100_sock_write_txwr(0, txwr);
  w5100_sock_write_cr(0, W5100_SOCK_CR_SEND);
}

/*
  -- Ethersex META --
  header(hardware/ethernet/w5100.h)
  net_init(w5100_init)
  mainloop(w5100_process)
*/