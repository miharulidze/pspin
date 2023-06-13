#pragma once

#include <stcp.h>
#include <pspinsim.h>
#include <spin.h>

#include "multilist_host.h"
#include "stcp_host.h"

namespace STCP
{
    class TCPReceiver
    {
    private:
        uint32_t shadow_buff_size;
        uint8_t *shadow_buffer;
        stcp_connection_t *connection_descr;
        spin_ec_t ec;
        uint32_t current_seqno;
        double G; // gap per byte in ns
        bool ready;

    public:
        TCPReceiver(uint32_t shadow_buffer_size, double G)
            : shadow_buff_size(shadow_buff_size), current_seqno(0), ready(false), G(G)
        {
            shadow_buffer = (uint8_t*) calloc(sizeof(uint8_t), shadow_buff_size);
            assert(shadow_buffer!=NULL);

            size_t l2_size = config_nic_mem();
            make_ec(NIC_L2_ADDR, l2_size);
        }

        ~TCPReceiver()
        {
            free(shadow_buffer);
        }

        void notifyWriteToHost(uint64_t addr, uint8_t* data, size_t len)
        {
            uint64_t shadow_buff_addr = (uint64_t) & (shadow_buffer[0]);

            if (addr == 0xdeadbeef) {
                uint64_t ready_seqno = *((uint64_t *)data);
                printf("NIC is updating seqno to %lu (0x%lx)\n", ready_seqno, ready_seqno);

                /* we are relaxing this assumption
                if (current_seqno >= ready_seqno)
                {
                    printf("Invalid seqno update: Current seqno (at the host): %u; new seqno: %u;\n", current_seqno, ready_seqno);
                    assert(current_seqno < ready_seqno);
                }
                */

                if (current_seqno < ready_seqno) {
                    printf("%lu SEQNO_UPDATE %lu %lu %lu\n", pspinsim_time(), current_seqno, ready_seqno,
                           ready_seqno - current_seqno);
                    assert(ready_seqno >= 0 && ready_seqno < shadow_buff_size);
                    current_seqno = ready_seqno;
                    for (uint32_t i = current_seqno; i < ready_seqno; i++) {
                        assert(shadow_buffer[i] == 1);
                    }
                }

                return;
            }

            printf("[HOST] NIC wants to write %lu bytes to addr %lx (-> %lx); Offset: %u; Buffer: [%lx; %lx]\n",
                   len, addr, addr + len, (uint32_t)(addr - shadow_buff_addr),
                   shadow_buff_addr, shadow_buff_addr + shadow_buff_size);
            if (addr < shadow_buff_addr)
                FATAL("Error: writing before shadow buffer! addr: %lx; shadow_buff_addr: %lx\n", addr, shadow_buff_addr);
            if (addr + len > shadow_buff_addr + shadow_buff_size)
                 FATAL("Error: writing after shadow buffer! addr + len: %lx; shadow_buff_addr + shadow_buff_size: %lx\n",
                       addr + len, shadow_buff_addr + shadow_buff_size);

            for (uint32_t i = 0; i < len; i++) {
                *((char *)shadow_buff_addr) = 1;
                shadow_buff_addr++;
            }
        }

        void notifyWriteToNICComplete(void *user_ptr)
        {
            ready = true;
        }

        bool isReady()
        {
            return ready;
        }

        void injectPacket(Packet* pkt)
        {
            pspinsim_packet_add(&ec, 0, &(pkt->data[0]), pkt->size, pkt->size,
                                pkt->eos, pkt->size * G, 0);
            if (pkt->eos) {
              pspinsim_packet_eos();
            }
        }

        size_t bytesReceived()
        {
            return (size_t) current_seqno;
        }

    private:
        void make_ec(uint32_t l2_addr, size_t l2_size)
        {
            spin_nic_addr_t hh_addr, ph_addr, th_addr;
            size_t hh_size, ph_size, th_size;

            CHECK_ERR(spin_find_handler_by_name(HANDLERS_FILE, "tcp_hh", &hh_addr, &hh_size));
            CHECK_ERR(spin_find_handler_by_name(HANDLERS_FILE, "tcp_ph", &ph_addr, &ph_size));
            CHECK_ERR(spin_find_handler_by_name(HANDLERS_FILE, "tcp_th", &th_addr, &th_size));

            printf("hh_addr: %p; hh_size: %lu;\n", hh_addr, hh_size);
            printf("ph_addr: %p; ph_size: %lu;\n", ph_addr, ph_size);
            printf("th_addr: %p; th_size: %lu;\n", th_addr, th_size);

            ec.handler_mem_addr = (uint32_t)l2_addr;
            ec.handler_mem_size = (uint32_t)l2_size;
            ec.host_mem_addr = (uint64_t) & (shadow_buffer[0]);
            ec.host_mem_size = shadow_buff_size;
            ec.hh_addr = hh_addr;
            ec.ph_addr = ph_addr;
            ec.th_addr = 0; // th_addr;
            ec.hh_size = hh_size;
            ec.ph_size = ph_size;
            ec.th_size = th_size;
            // ec.pin_to_cluster   = 1;

            for (int i = 0; i < NUM_CLUSTERS; i++) {
                ec.scratchpad_addr[i] = 0; // SCRATCHPAD_ADDR(i);
                ec.scratchpad_size[i] = L1_SCRATCHPAD_SIZE;
            }
        }

        void init_conn(stcp_connection_t *conn)
        {
            // set main buffer
            conn->buffer.host_ptr = (uint64_t) & (shadow_buffer[0]);
            conn->buffer.length = shadow_buff_size;
            conn->buffer.valid = 1;
            conn->buffer.bytes_left = shadow_buff_size;

            conn->refs = 0;
            conn->curr_state = STCP_LISTEN;

            conn->rx_isn = 0;
            conn->tx_isn = 0;

            conn->fin_seqno = 0;
            conn->fin_seen = false;

            conn->next_to_read = 0;
            conn->next_to_receive = 0;
            conn->next_to_ack = 0;

            conn->ooo_lock = 0;
            conn->ooo_threads = 0;
            conn->ooo_dirty = 0;

            uint32_t multilist_nic_address = NIC_L2_ADDR + offsetof(stcp_connection_t, ooo_man);
            uint32_t multilist_nic_mem_size = NIC_L2_SIZE - (sizeof(stcp_connection_t));
            multilist_host_init(&(conn->ooo_man), NUM_OOO_SEGMENTS, 8, multilist_nic_address, multilist_nic_mem_size);
        }

        size_t config_nic_mem()
        {
            size_t connection_state_size = sizeof(stcp_connection_t);
            connection_state_size += multilist_host_additional_size(NUM_OOO_SEGMENTS, 8);
            assert(connection_state_size <= NIC_L2_SIZE);

            printf("Connection state size: %lu B (multilist additional size: %lu B)\n",
                   connection_state_size, multilist_host_additional_size(NUM_OOO_SEGMENTS, 8));

            connection_descr = (stcp_connection_t *)malloc(connection_state_size * sizeof(uint8_t));
            init_conn(connection_descr);
            spin_nicmem_write(NIC_L2_ADDR, (void *)connection_descr, connection_state_size, NULL);

            return connection_state_size;
        }
    };
}
