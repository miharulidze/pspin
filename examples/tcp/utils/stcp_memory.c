#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../stcp.h"

#define RAND_SEED 3963280

#define MIN_PORT_NUM 0
#define MAX_PORT_NUM 65535

//tcp recv window
#define SHADOW_BUFF_SIZE STCP_SHADOW_BUFFER_SIZE
#define SHADOW_BUFF_START ((void*)0x4000000)

#define MAX_PTRS 1024

void init_conn(stcp_connection_t *conn, uint32_t tx_isn)
{
    //set main buffer
    conn->buffer.host_ptr = NULL;
    conn->buffer.length = 0;
    conn->buffer.valid = 0;
    conn->buffer.bytes_left = 0;

    //set shadow buffer
    conn->shadow_buffer.host_ptr = SHADOW_BUFF_START;
    conn->shadow_buffer.length = SHADOW_BUFF_SIZE;
    conn->shadow_buffer.valid = 1;
    conn->shadow_buffer.bytes_left = SHADOW_BUFF_SIZE;

    conn->refs = 0;
    conn->curr_state = STCP_LISTEN;

    conn->rx_isn = 0;
    conn->tx_isn = tx_isn;

    conn->fin_seqno = 0;
    conn->fin_seen = false;

    conn->next_to_read = 0;
    conn->next_to_receive = 1;
    conn->next_to_ack = 0;

    //init bitmap
    uint32_t words = (SHADOW_BUFF_SIZE + 8*sizeof(uint32_t) - 1) / (8*sizeof(uint32_t));
    conn->bitmap.num_words=words;
    for (uint32_t i=0; i<conn->bitmap.num_words; i++){
        conn->bitmap.words[i] = 0;
    }
    conn->bitmap.words[words] = 0;

    conn->bitmap_lock = 0;
}

void init_random_conn(stcp_connection_t *conn)
{
    uint32_t sip = rand();
    uint16_t dport = (uint16_t) (MIN_PORT_NUM + (rand() % (MAX_PORT_NUM - MIN_PORT_NUM)));
    uint16_t sport = (uint16_t) (MIN_PORT_NUM + (rand() % (MAX_PORT_NUM - MIN_PORT_NUM)));
    init_conn(conn, 0);
}

int main(int argc, char **argv)
{

    uint32_t tx_isn = 0;
    stcp_connection_t conn;
    init_conn(&conn, tx_isn);

    uint8_t *raw = (uint8_t *) &conn;

    printf("#pragma once\n");
    printf("volatile __attribute__((section(\".l2_handler_data\"))) uint8_t handler_mem[] = {");

    for (uint32_t i=0; i<sizeof(stcp_connection_t); i++)
    {
        printf("0x%x, ", raw[i]);
    }
    printf(" 0x0};\n");

}
