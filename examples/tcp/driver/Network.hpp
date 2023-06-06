#pragma once

#include "stcp_host.h"

#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <queue>
#include <random>
#include <assert.h>

namespace STCP
{
    class ReorderPacket
    {
    public:
        Packet *pkt;
        uint64_t ready_time;

    public:
        ReorderPacket(Packet *pkt) : pkt(pkt) {}
        ReorderPacket() : pkt(NULL) {}
    };

    class Network
    {
    private:
        std::queue<ReorderPacket> fast_queue;
        std::queue<ReorderPacket> slow_queue;

        std::mt19937 gen32;
        double k;

        uint64_t current_time; //ps
        uint64_t fast_link_lat, slow_link_lat; //ps

    public:
        Network(uint64_t fast_link_lat, uint64_t slow_link_lat, double k)
            : k(k), current_time(0), fast_link_lat(fast_link_lat*100), slow_link_lat(slow_link_lat*100)
        {
        }

        bool reorder()
        {
            return ((gen32() % 100) < k * 100);
        }

        void injectPacket(Packet *pkt)
        {
            printf("Network gets packet %p\n", pkt);
            ReorderPacket rpkt(pkt);
            if (reorder())
            {
                rpkt.ready_time = current_time + slow_link_lat;
                slow_queue.push(rpkt);
            }
            else
            {
                rpkt.ready_time = current_time + fast_link_lat;
                fast_queue.push(rpkt);
            }
        }

        Packet* extractPacket()
        {
            assert(isReady());

            uint64_t fast_queue_front_time = fast_queue.empty() ? UINT64_MAX : fast_queue.front().ready_time;
            uint64_t slow_queue_front_time = slow_queue.empty() ? UINT64_MAX : slow_queue.front().ready_time;
            printf("Current time: %lu; Fast queue: [size: %lu; next: %lu]; Slow queue: [size: %lu; next: %lu]\n", current_time, fast_queue.size(), fast_queue_front_time, slow_queue.size(), slow_queue_front_time);

            std::queue<ReorderPacket> &qmin = (fast_queue_front_time < slow_queue_front_time) ? fast_queue : slow_queue;
            
            assert(!qmin.empty());
            Packet *pkt = qmin.front().pkt;
            qmin.pop();

            printf("Network is delivering pkt %p (fast_queue size: %lu; slow_queue size: %lu)\n", pkt, fast_queue.size(), slow_queue.size());
            assert(pkt!=NULL);
            return pkt;
        }

        bool isReady()
        {
            return (!fast_queue.empty() && fast_queue.front().ready_time <= current_time) ||
                   (!slow_queue.empty() && slow_queue.front().ready_time <= current_time);
        }

        void tick(uint64_t sim_time)
        {
            current_time = sim_time;
        }
    };
}