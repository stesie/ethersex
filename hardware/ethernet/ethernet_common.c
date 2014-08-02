/*
 * Copyright (c) by Alexander Neumann <alexander@bumpern.de>
 * Copyright (c) 2007,2008,2009,2014 by Stefan Siegl <stesie@brokenpipe.de>
 * Copyright (c) 2008 by Christian Dietrich <stettberger@dokucode.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (version 3)
 * as published by the Free Software Foundation.
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

#include "network.h"
#include "protocols/uip/uip_arp.h"
#include "config.h"
#include "core/bit-macros.h"
#include "protocols/uip/uip_router.h"

#include "core/debug.h"

void
ethernet_process_packet(void)
{
  /* Set the enc stack active */
  uip_stack_set_active(STACK_ENC);

  /* process packet */
  struct uip_eth_hdr *packet = (struct uip_eth_hdr *)&uip_buf;

#ifdef IEEE8021Q_SUPPORT
  /* Check VLAN tag. */
  if (packet->tpid != HTONS(0x8100)
      || (packet->vid_hi & 0x1F) != (CONF_8021Q_VID >> 8)
      || packet->vid_lo != (CONF_8021Q_VID & 0xFF)) {
    debug_printf("net: wrong vlan tag detected.\n");
  }
  else
#endif
  {
    switch (HTONS(packet->type)) {

#     if !UIP_CONF_IPV6
      /* process arp packet */
      case UIP_ETHTYPE_ARP:
#       ifdef DEBUG_NET
        debug_printf("net: arp packet received\n");
#       endif
        uip_arp_arpin();

        /* if there is a packet to send, send it now */
        if (uip_len > 0)
#         ifdef ENC28J60_SUPPORT
          transmit_packet();
#         else
          w5100_txstart();
#         endif

        break;
#     endif /* !UIP_CONF_IPV6 */

#     if UIP_CONF_IPV6
      /* process ip packet */
      case UIP_ETHTYPE_IP6:
#       ifdef DEBUG_NET
        debug_printf ("net: ip6 packet received\n");
#       endif
#     else /* !UIP_CONF_IPV6 */

      /* process ip packet */
      case UIP_ETHTYPE_IP:
#       ifdef DEBUG_NET
        debug_printf ("net: ip packet received\n");
#       endif
        uip_arp_ipin();
#     endif /* !UIP_CONF_IPV6 */

        router_input(STACK_ENC);

        /* if there is a packet to send, send it now */
        if (uip_len > 0)
          router_output();

        break;

#     ifdef DEBUG_UNKNOWN_PACKETS
      default:
        /* debug output */
        debug_printf("net: unknown packet, %02x%02x%02x%02x%02x%02x "
                     "-> %02x%02x%02x%02x%02x%02x, type 0x%04x\n",
                     packet->src.addr[0], packet->src.addr[1],
                     packet->src.addr[2], packet->src.addr[3],
                     packet->src.addr[4], packet->src.addr[5],
                     packet->dest.addr[0], packet->dest.addr[1],
                     packet->dest.addr[2], packet->dest.addr[3],
                     packet->dest.addr[4], packet->dest.addr[5],
                     ntohs(packet->type));
            break;
#     endif
    }
  }
}
