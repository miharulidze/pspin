#pragma once

#define FATAL(...)           \
    {                        \
        printf(__VA_ARGS__); \
        fflush(stdout);      \
        fflush(stderr);      \
        exit(1);             \
    }
#define CHECK_ERR(S)                 \
    {                                \
        int res;                     \
        res = S;                     \
        assert(res == SPIN_SUCCESS); \
    }

#define HANDLERS_FILE "build/tcp"
#define SLM_FILES "build/slm_files/"

#define NIC_L2_ADDR 0x1c300000
#define NIC_L2_SIZE 1048576
#define HOST_ADDR 0x4000000000000000ul
#define L1_VMEM_SIZE 0x400000
#define SCRATCHPAD_BASE_ADDR 0x10012400
#define SCRATCHPAD_EXTENT 4 * 1024 * 1024
#define SCRATCHPAD_ADDR(cluster_id) (SCRATCHPAD_BASE_ADDR + (cluster_id)*SCRATCHPAD_EXTENT)

#include <vector>

namespace STCP
{

    class Packet
    {
    public:
        std::vector<uint8_t> data;
        size_t size;
        size_t mtu;
        bool eos;

    public:
        Packet(size_t mtu) : mtu(mtu) 
        {
            data.resize(mtu);
        }
    };

}
