#include <handler.h>
#include <packets.h>

#include "stcp.h"
#include "stcp_utils.h"
#include "stcp_mem.h"
#include "stcp_rx.h"
#include "stcp_tx.h"
#include "ip.h"
#include "tcp.h"

/* TCP connection lookup */
/*
static stcp_connection_t *stcp_conn_lookup(tcp_header_t *tcp_header, uint32_t sip)
{
    stcp_conn_key_t key;
    key.source_ip = sip;
    key.ports = STCP_MERGE_PORTS(tcp_header->source, tcp_header->dest);

    uint32_t ht_key;
    HASH_JEN(&key, sizeof(stcp_conn_key_t), ht_key);
    ht_key = FAST_MOD(ht_key, STCP_HASH_TABLE_SIZE);

    //printf("KEY: [ip: %lu; sport: %u, dport: %u; ports: %lu]; magic_num: %p; HASH: %lu\n",
    //       key.source_ip, tcp_header->source, tcp_header->dest, key.ports, tcp_state->magic_num, ht_key);

    // this is a linked list. The host might be modifying it (e.g., to add/remove connection)
    stcp_conn_key_state_t *key_state = &(tcp_state->conn_ht[ht_key]);
    spin_rw_lock_r_lock(&(key_state->rw_lock));

    stcp_connection_t *conn = key_state->head;
    while (conn != NULL) {
        //printf("ENTRY: ptr: %p, source_ip_ptr: %p [ip: %lu; ports: %lu]\n",
                 conn, &(conn->key.source_ip), conn->key.source_ip, conn->key.ports);
        if (conn->key.source_ip == key.source_ip && conn->key.ports == key.ports) {
            amo_add(&(conn->refs), 1);
            spin_rw_lock_r_unlock(&(key_state->rw_lock));
            return conn;
        }
        conn = conn->next;
    }

    spin_rw_lock_r_unlock(&(key_state->rw_lock));

    return NULL;
}
*/

__handler__ void tcp_hh(handler_args_t *args)
{
    task_t* task = args->task;
#ifdef CONNECTION_IN_L1
    cluster_state_t *cluster_state = ((cluster_state_t **)task->scratchpad)[task->home_cluster_id];
    stcp_connection_t *connection = &(cluster_state->connection);
    spin_cmd_t cmd;

    spin_dma(task->handler_mem, cluster_state, task->handler_mem_size, SPIN_DMA_EXT2LOC, 0, &cmd);
    spin_dma_wait(cmd);

    //printf("Handler mem: %p; cluster mem: %p; cluster_id: %lu; scratchpad[%u]: %p %p\n",
    //       task->handler_mem,
    //       &(cluster_state->connection),
    //       args->cluster_id,
    //       task->home_cluster_id,
    //       &(task->scratchpad[task->home_cluster_id]),
    //       cluster_state);
#else /* CONNECTION_IN_L2 */
    stcp_connection_t *connection = (stcp_connection_t*) task->handler_mem;
    ooo_list = connection->ooo_list;
#endif

#ifdef OOO_LAZYLIST
    //TODO: implement me if needed
    //ooo_list = &(cluster_state->connection.ooo_list)
    //printf("OOO list: head_idx: %lu (%lx -> %lx); right sentinel: (%lx -> %lx)\n", ooo_list->head_idx, ooo_list->nodes[ooo_list->head_idx].range.left, ooo_list->nodes[ooo_list->head_idx].range.right, ooo_list->nodes[127].range.left, ooo_list->nodes[127].range.right);
    //S_ASSERT(ooo_list->nodes[126].range.left == INT32_MIN && ooo_list->nodes[126].range.right == INT32_MIN);
    //S_ASSERT(ooo_list->nodes[127].range.left == INT32_MAX && ooo_list->nodes[127].range.right == INT32_MAX);
    //S_ASSERT(GET_NODE_PTR(ooo_list, ooo_list->head_idx) == &(ooo_list->nodes[126]));
    //S_ASSERT(GET_NODE_PTR(ooo_list, GET_NODE_PTR(ooo_list, ooo_list->head_idx)->next_idx) == &(ooo_list->nodes[127]));
    //printf("OOO list: head_idx: %lu (%lx -> %lx); right sentinel: (%lx -> %lx)\n", ooo_list->head_idx, ooo_list->nodes[ooo_list->head_idx].range.left, ooo_list->nodes[ooo_list->head_idx].range.right, ooo_list->nodes[127].range.left, ooo_list->nodes[127].range.right);
#else
    connection->ooo_man.free_pool.mem_base_addr_start =
        (uint32_t) (((uint8_t*) &connection->ooo_man) +
                    ((uint32_t) connection->ooo_man.free_pool.mem_base_addr_start));
    connection->ooo_man.free_pool.mem_base_addr_end =
        (uint32_t) (((uint8_t*) &connection->ooo_man) +
                    ((uint32_t) connection->ooo_man.free_pool.mem_base_addr_end));
#endif
}

__handler__ void tcp_ph(handler_args_t *args)
{
    task_t* task = args->task;
    ip_header_t *ip_hdr = (ip_header_t*) (task->pkt_mem);
    uint16_t ip_hdr_len = ip_hdr->ihl * sizeof(uint32_t);
    tcp_header_t *tcp_hdr = (tcp_header_t*) (((uint8_t *) task->pkt_mem) + ip_hdr_len);
    uint32_t sip = ip_hdr->saddr;
    uint32_t dip = ip_hdr->daddr;
    uint32_t tcp_segment_size = NTOHS(ip_hdr->tot_len) - ip_hdr_len;

#ifdef ONLY_CHECKSUM
    stcp_cksum(tcp_hdr, sip, dip, tcp_segment_size);
#elif defined (ONLY_BITMAP)
    /* TODO: implement me or remove */
#else
    uint16_t tcp_cksum = tcp_hdr->check;
    bool valid = 1; // stcp_cksum(tcp_hdr, sip, dip, tcp_segment_size);
    STCP_DPRINTF("tcp check: %lx; tcp_segment_size: %lu; ip_hdr_len: %lu; cksum valid: %u\n",
                 tcp_cksum, tcp_segment_size, ip_hdr_len, (uint16_t) valid);
#endif

    stcp_connection_t *connection;
    if (valid) {
#ifdef CONNECTION_LOOKUP
        stcp_connection_t *connection = stcp_conn_lookup(stcp_state, tcp_hdr, sip);
#elif defined (CONNECTION_IN_L1)
        cluster_state_t *cluster_state = ((cluster_state_t **)args->task->scratchpad)[args->task->home_cluster_id];
        stcp_connection_t *connection = &(cluster_state->connection);
#else /* CONNECTION_IN_L2 */
        connection = (stcp_connection_t*) task->handler_mem;
#endif
        stcp_rx(args, connection, tcp_hdr, tcp_segment_size);
    } else {
        STCP_DPRINTF("Error: TCP checksum failed!\n");
    }

    //stcp_tx_ack(connection, tcp_hdr, 0);
}

/*
 * Warning: recall that the TH is not called on the last received packet, but on the last
 * processed packet!!!
 */
__handler__ void tcp_th(handler_args_t *args)
{
    /* TODO: close connection */
}

void init_handlers(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {tcp_hh, tcp_ph, tcp_th};

    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];

    *handler_mem_ptr = (void *)handler_mem;
}
