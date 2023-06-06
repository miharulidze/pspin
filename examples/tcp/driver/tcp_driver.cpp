#include "TCPSender.hpp"
#include "TCPDummySender.hpp"
#include "TCPReceiver.hpp"
#include "Network.hpp"
#include "cmdline.h"

#define SLM_FILES "build/slm_files/"
#define MAX_PKT_LEN 8192
#define SHADOW_BUFF_SIZE STCP_SHADOW_BUFFER_SIZE

using namespace STCP;

TCPDummySender *sender;
TCPReceiver *receiver;
Network *network;

#define IPSRC 0
#define IPDST 10
#define START_SEQNO 1000

uint32_t mtu;

void pkt_out(uint8_t* data, size_t len);
void pcie_slv_write(uint64_t addr, uint8_t* data, size_t len);
void pcie_slv_read(uint64_t addr, uint8_t* data, size_t len);
void pcie_mst_write_complete(void *user_ptr);
void pcie_mst_read_complete(void *user_ptr);

int main(int argc, char**argv)
{
    struct gengetopt_args_info ai;
    if (cmdline_parser(argc, argv, &ai) != 0)
    {
        exit(1);
    }
    
    pspin_conf_t conf;
    pspinsim_default_conf(&conf);
    conf.slm_files_path = SLM_FILES;

    mtu = ai.dummy_mtu_arg;

    pspinsim_init(argc, argv, &conf);
    pspinsim_cb_set_pkt_out(pkt_out);
    pspinsim_cb_set_pcie_slv_write(pcie_slv_write);
    pspinsim_cb_set_pcie_slv_read(pcie_slv_read);
    pspinsim_cb_set_pcie_mst_write_completion(pcie_mst_write_complete);
    pspinsim_cb_set_pcie_mst_read_completion(pcie_mst_read_complete);

    // NOTE: the sender is only functional. Bandwidth shaping is done at the receiver side (I know, this is ugly).
    sender = new TCPDummySender(ai.dummy_bytes_arg, mtu, IPSRC, IPDST, START_SEQNO);
    receiver = new TCPReceiver(SHADOW_BUFF_SIZE, ai.G_arg);
    network = new Network(ai.ooo_fast_lat_arg, ai.ooo_slow_lat_arg, ai.ooo_k_arg);

    // main loop
    uint8_t sim_complete = 0;
    while (!sim_complete)
    {
        network->tick(pspinsim_time());
        
        if (sender->isReady())
        {
            Packet* pkt = new Packet(mtu);
            // just get as many packets as we can from the sender, they will be paced in the receiver. 
            size_t pkt_len = sender->getNextPacket(pkt);
            assert(pkt->size <= pkt->mtu);
            network->injectPacket(pkt);
        }

        if (network->isReady())
        {
            Packet *pkt = network->extractPacket();
            receiver->injectPacket(pkt);
            delete pkt; // the receiver will make a copy of the pkt, so this is safe 
        }

        pspinsim_run_tick(&sim_complete);
    }

    pspinsim_fini();

    uint32_t bytes_sent = sender->bytesSent();
    uint32_t bytes_recv = receiver->bytesReceived();

    printf("Total payload bytes sent: %d; receveid: %u\n", bytes_sent, bytes_recv);
    if (bytes_sent != bytes_recv)
    {
        // no assert here otherwise traces are not flushed and we might want to see them at this point
        printf("#### Error: bytes_sent!=bytes_recv!!!");
    }

    

    return 0;
}

void pkt_out(uint8_t* data, size_t len)
{
    // mainly for ACKs
    sender->notifyPacket(data, len);
}

void pcie_slv_write(uint64_t addr, uint8_t* data, size_t len)
{
    receiver->notifyWriteToHost(addr, data, len);
}

void pcie_slv_read(uint64_t addr, uint8_t* data, size_t len)
{
    printf("NIC wants to read %lu bytes from addr %lx\n", len, addr);
    assert(0);
}

void pcie_mst_write_complete(void *user_ptr)
{
    printf("Write to NIC memory completed (user_ptr: %p)\n", user_ptr);
    receiver->notifyWriteToNICComplete(user_ptr);
}

void pcie_mst_read_complete(void *user_ptr)
{
    printf("Read from NIC memory completed (user_ptr: %p)\n", user_ptr);
    assert(0);
}
