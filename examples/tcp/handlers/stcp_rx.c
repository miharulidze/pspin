#include "stcp_rx.h"
#include "stcp_tx.h"
#include "stcp_events.h"
#include "stcp_utils.h"
#include "stcp_bitmap.h"
#include <ooo.h>

#include <handler.h>

void notify_host(stcp_connection_t *connection, uint32_t new_seqno)
{
    //update the host 
    spin_cmd_t cmd;
    spin_host_write(0xdeadbeef, new_seqno, &cmd);

    //We need to wait the completion of this write before updating the next_to_receive in L2.
    //If we don't, another handler can finish the update before us (it would be enabled by the new next_to_receive in L2)
    //causing to write an old seqno to the host.  It becomes clearer if you inver the write_to_host with the amo_store. 
    spin_cmd_wait(cmd);

    //store the new next_to_receive in L2
    //we can use an amo_store here because nobody only the handler with the right seqno is going to update next_to_receive
    //amo_store(&(connection->next_to_receive), new_seqno);
}
    
static void stcp_rx_handle_data(handler_args_t *handler_args, stcp_connection_t *connection, tcp_header_t *tcp_hdr, size_t tcp_seg_size)
{
    // we shift rseqno so to reserve some space for special values (e.g., sentinel in multilist starting at 0)
    uint32_t rseqno = NTOHL(tcp_hdr->seq) - connection->rx_isn;
    uint32_t tcp_header_size = tcp_hdr->doff * sizeof(uint32_t);
    uint32_t payload_size = tcp_seg_size - tcp_header_size;
    uint8_t *payload_ptr = ((uint8_t *)tcp_hdr) + tcp_header_size;

    uint32_t buff_offset = FAST_MOD(rseqno, connection->buffer.length);

    uint32_t orig_buf_offset = buff_offset;
    uint32_t orig_payload_size = payload_size;

    uint32_t hpu_id = handler_args->hpu_id;

    cluster_state_t **all_clusters_state = (cluster_state_t **)(handler_args->task->scratchpad);
    cluster_state_t *cluster_state = all_clusters_state[handler_args->cluster_id];
    //cluster_state_t *cluster_state = all_clusters_state[0];

    //we need to check if we can store this packet
    uint32_t max_seqno = connection->next_to_read + connection->buffer.length;

    ooo_range_t stream_range;
    stream_range.left = rseqno;
    stream_range.right = rseqno + payload_size - 1; // right-open interval

    // No payload
    if (payload_size == 0)
    {
        STCP_DPRINTF("No payload!\n");
        return;
    }

    // Before the recv window
    if (rseqno < connection->next_to_receive)
    {
        STCP_DPRINTF("Packet falling before recv win. Dropping! (rseqno: %lu; next_to_receive: %lu)\n", rseqno, connection->next_to_receive);
        return;
    }
    
    // Outside the rcv windows
    if (rseqno > max_seqno)
    {
        STCP_DPRINTF("Packet is falling on the right of the recv win! Dropping! (rseqno: %lu; payload_size: %lu; max_seqno: %lu)\n", rseqno, payload_size, max_seqno);
        return;
    }

    // Doesn't fit in rcv window
    if (rseqno + payload_size > max_seqno)
    {
        STCP_DPRINTF("No buffer available! Dropping! (rseqno: %lu; payload_size: %lu; max_seqno: %lu)\n", rseqno, payload_size, max_seqno);
        return;
    }
    
    // NOTE: we write anyway for now.. if later we find out that we don't have space in the heap then we will write again. 
    //printf("Copying %lu bytes to the USER buffer (0x%llx) at offset %lu (rseqno: %lu; max_seqno: %lu)\n", payload_size, connection->buffer.host_ptr, buff_offset, rseqno, max_seqno);
    spin_cmd_t cmd;
    //printf("connection->buffer.host_ptr: %llx\n", connection->buffer.host_ptr);
    
#ifdef SPIN_SNITCH
    spin_host_dma(connection->buffer.host_ptr + buff_offset, (uint32_t)payload_ptr, NIC_TO_HOST, payload_size, &cmd);
#else
    spin_dma_to_host(connection->buffer.host_ptr + buff_offset, (uint32_t)payload_ptr, payload_size, 0, &cmd);
#endif

    // OOO handling
    amo_add(&connection->ooo_threads, 1);
    uint32_t res = ooo_range_push(&connection->ooo_man, handler_args->hpu_id, handler_args->cluster_id, stream_range);
    amo_or(&connection->ooo_dirty, 1);

    if (res != OOO_SUCCESS)
    {
        // here we should signal the error condition to the host
        S_ASSERT(false);
    }

    uint32_t num_threads = amo_add(&connection->ooo_threads, -1);
    
    // try to acquire ooo_lock
    uint32_t tot_flushed_bytes = 0;
    if (spin_lock_try_lock(&(connection->ooo_lock)))
    {

        uint32_t flushed_bytes = ooo_range_front_flush(&connection->ooo_man, handler_args->hpu_id, handler_args->cluster_id, connection->next_to_receive);        
        /*
        uint32_t new_flushed_bytes = 0;
        while (connection->ooo_threads==0 && (new_flushed_bytes>0 || connection->ooo_dirty))
        {
            connection->ooo_dirty = 0;
            new_flushed_bytes = ooo_range_front_flush(&connection->ooo_man, handler_args->hpu_id, handler_args->cluster_id, connection->next_to_receive);
            flushed_bytes += new_flushed_bytes;
        }

        // another handler could entirely execute here (after having failed to progress because the lock was acquired)
        // and its update could be lost

        // by having the unlock inside the loop, we have these possible scenarios:
        //  - the other handler executes fully (e.g., before the unlock). The current handler will see ooo_dirty and re-check
        //  - the other handler locks ooo_lock. If that happens
        //      - it's a signal that there exist another handler and it will get to the "clean-up" block. 

        */

        /*
        if (flushed_bytes > 0)
        {
            spin_cmd_t cmd;
            connection->next_to_receive += flushed_bytes;
            spin_write_to_host(0xdeadbeef, connection->next_to_receive, &cmd);
        }
        */
        connection->next_to_receive += flushed_bytes;
        tot_flushed_bytes += flushed_bytes;
        spin_lock_unlock(&(connection->ooo_lock));
    }    

    // if we were the only one running and OOO is not empty, then we should force a flushing round
    // this avoids the case in which at the previous flush we missed some concurrent OOO segment insertion
    // if there are other threads, then we don't care... somebody else will pick it up
    // we need a while here because in the meantime we are doing this another HPU could add an OOO segment

    // TODO: add signal to check if a HPU ran while we were doing this (amo_or)
    // if we ran alone and did not flush anything in this run, then we are sure that there is nothing to deliver (double check)


    // why I cannot lock beforehand ??? (see above)   
    uint32_t flushed_bytes = 0;
    while (connection->ooo_threads==0 && (flushed_bytes>0 || connection->ooo_dirty) && spin_lock_try_lock(&(connection->ooo_lock)))
    {
        connection->ooo_dirty = 0;
        flushed_bytes = ooo_range_front_flush(&connection->ooo_man, handler_args->hpu_id, handler_args->cluster_id, connection->next_to_receive);
        connection->next_to_receive += flushed_bytes;
        tot_flushed_bytes += flushed_bytes;
        spin_lock_unlock(&(connection->ooo_lock));
    }

    // we put this outside locks and accept that the host might receive OOO seqno updates.
    // those will be valid anyway but the host needs to keep track of the max
    if (tot_flushed_bytes > 0)
    {
        spin_cmd_t cmd;
        spin_write_to_host(0xdeadbeef, connection->next_to_receive, &cmd);
    }

    



    // end OOO handling

    //TODO: ACK!
}

void stcp_rx(handler_args_t *handler_args, stcp_connection_t *connection, tcp_header_t *tcp_hdr, size_t tcp_seg_size)
{
    switch (connection->curr_state)
    {

    case STCP_LISTEN:
        if (!tcp_hdr->syn || tcp_hdr->ack)
        {
            STCP_DPRINTF("ERROR! Received a non SYN or a SYN-ACK!\n");
            stcp_evt_error(connection, tcp_hdr, STCP_EVT_ERR_PROTO);
        }

        connection->curr_state = STCP_SYN_RCVD;
        connection->rx_isn = NTOHL(tcp_hdr->seq);

        STCP_DPRINTF("SYN recvd: ISN: %lu; cksum: %x\n", connection->rx_isn, tcp_hdr->check);

        //send ACK
        stcp_tx_ack(connection, tcp_hdr, tcp_hdr->seq);

        break;

    case STCP_SYN_RCVD:

        //Note: there is  no guaranteee that we will receive this ACK before other packets!
        //As soon as we see an ack, we move to STCP_ESTABLISHED
        /*if (tcp_hdr->syn || !tcp_hdr->ack)
        {
            STCP_DPRINTF("ERROR! Received a SYN or not an ACK!\n");
            stcp_evt_error(state, connection, tcp_hdr, STCP_EVT_ERR_PROTO);
            return;
        }
        */

        if (tcp_hdr->ack)
        {
            //multiple handlers
            STCP_DPRINTF("STCP_SYN_RCVD -> STCP_ESTABLISHED\n");
            connection->curr_state = STCP_ESTABLISHED;
        }

        //stcp_rx_handle_data(state, connection, tcp_hdr, tcp_seg_size);

        //fall through
    case STCP_ESTABLISHED:

        //TODO: handle RST!

        if (tcp_hdr->syn)
        {
            STCP_DPRINTF("ERROR! Received a SYN!\n");
            stcp_evt_error(connection, tcp_hdr, STCP_EVT_ERR_PROTO);
            return;
        }

        if (tcp_hdr->fin)
        {
            connection->fin_seqno = NTOHL(tcp_hdr->seq);
            asm_mem_fence();
            connection->fin_seen = true;
            stcp_tx_ack(connection, tcp_hdr, tcp_hdr->seq);
        }

        stcp_rx_handle_data(handler_args, connection, tcp_hdr, tcp_seg_size);

        /* we saw the fin and all the incoming have been processed */
        /* Can we receive a FIN before a ack? What happens in that case? */
        if (connection->fin_seen && connection->next_to_receive >= connection->fin_seqno)
        {
            connection->curr_state = STCP_CLOSE_WAIT;

            /* WARNING: here we shortcut to STCP_LAST_ACK because the we don't have an host running! */
            connection->curr_state = STCP_LAST_ACK;

            //here we send the FIN event to the host. Then we expect the host to
            //send a FIN and trigger a command handler that makes the state
            //to transition to STCP_LAST_ACK.
            stcp_evt(connection, tcp_hdr, STCP_EVT_FIN);

            STCP_DPRINTF("STCP_ESTABLISHED -> STCP_LAST_ACK (next_to_read: %lu; fin_seqno: %lu)", connection->next_to_read, connection->fin_seqno);
        }

        break;

    case STCP_LAST_ACK:
        /* the peer cannot send this before we the host sends its FIN and we make sure that the
        FIN event is propogated to the host only when all the packets are processed. Hence, this packet
        should not overlap with any other one. */
        STCP_DPRINTF("ERROR! Received LAST ACK!\n");
        stcp_evt(connection, tcp_hdr, STCP_EVT_CLOSE);
        break;
    default:
        ASSERT(0);
    }
}
