import numpy as np
import sys
import os


def get_pkt_size_lognorm(mean, sigma, min_pkt_size, max_pkt_size):
    assert min_pkt_size > 0 and max_pkt_size <= 4096
    res = 0
    while (res < min_pkt_size or res > max_pkt_size):
        res = int(8*round((np.random.lognormal(mean,sigma))/8.))
    return res


def generate_arrival_sequence(n_tenants, total_packets):
    return np.random.randint(0, high=n_tenants, size=total_packets, dtype=int).tolist()


def generate_packet_sizes(n_tenants, arrival_sequence, pkt_size_gens):
    assert len(pkt_size_gens) == n_tenants
    new_sequence = []
    for tenant_id in arrival_sequence:
        assert tenant_id < n_tenants
        new_sequence.append([tenant_id, pkt_size_gens[tenant_id]()])

    return new_sequence


def dump_sequence(n_tenants, packets, path, src_prefix, dst_prefix):
    last_pkt_indices = {}
    max_pkt_size = 0
    for tenant_id in range(n_tenants):
        for pkt_id, pkt in enumerate(packets):
            if pkt[0] == tenant_id:
                last_pkt_indices[tenant_id] = pkt_id
            if pkt[1] > max_pkt_size:
                max_pkt_size = pkt[1]

    with open(path, "w") as trace_file:
        trace_file.write(f"{n_tenants} {len(packets)} {max_pkt_size}\n")
        for pkt_id, pkt in enumerate(packets):
            is_last = int(pkt_id == last_pkt_indices[pkt[0]])
            trace_file.write(f"{src_prefix} {dst_prefix}{pkt[0]} {pkt[1]} {is_last}\n")


gen_64_128_pkt_sizes = lambda: get_pkt_size_lognorm(4.0, 0.3, 64, 128)
gen_2048_4096_pkt_sizes = lambda: get_pkt_size_lognorm(9.0, 0.2, 3072, 4096)

n_tenants=4
total_packets=500
gens = [gen_64_128_pkt_sizes, gen_64_128_pkt_sizes,       # 0 and 1 are victims
        gen_2048_4096_pkt_sizes, gen_2048_4096_pkt_sizes] # 2 and 3 are congestors

n_traces=10
for i in range(n_traces):
    arrival_sequence = generate_arrival_sequence(n_tenants, total_packets)
    sequence = generate_packet_sizes(n_tenants, arrival_sequence, gens)
    dump_sequence(n_tenants, sequence, f"./t{n_tenants}.{total_packets}.{i}.trace", "src_addr", "192.168.0.")
