import csv
import re
import sys
from pathlib import Path

n_tenants = 1
logs_dir = sys.argv[1]

def extract_timestamp(raw_data):
    chunks = raw_data.strip().split("][")
    return int(int(chunks[0][1:]) / 1000)

def parse_time(logsdir, mode, bmark_name, packet_size, outcsv):
    timestamps = {}

    filename = f"{logsdir}/raw_tput.{mode}.{bmark_name}.t1.100.{packet_size}b.log"

    if not Path(filename).is_file():
        return

    print(filename)

    for tenant_id in range(n_tenants):
        timestamps[str(tenant_id)] = {}

        with open(filename, 'r', errors='replace') as log:
            while line := log.readline():
                if re.search("sent new task to FMQ", line):
                    chunks = line.strip().split(" ")
                    msg_id = chunks[6].strip().split("=")[1]
                    if (msg_id != str(tenant_id)):
                        continue

                    her_addr = chunks[7].strip().split("=")[1]

                    if her_addr not in timestamps[str(tenant_id)].keys():
                        timestamps[str(tenant_id)][her_addr] = []
                    timestamps[str(tenant_id)][her_addr].append([bmark_name,mode,packet_size,her_addr,extract_timestamp(chunks[0])])
                    continue

                if re.search("sent task to scheduler", line):
                    chunks = line.strip().split(" ")
                    msg_id = chunks[6].strip().split("=")[1]
                    if (msg_id != str(tenant_id)):
                        continue

                    her_addr = chunks[5].strip().split("=")[1]
                    timestamps[str(tenant_id)][her_addr][-1].append(extract_timestamp(chunks[0]))
                    continue

                if re.search("started task execution", line):
                    chunks = line.strip().split(" ")
                    msg_id = chunks[6].strip().split("=")[1]
                    if (msg_id != str(tenant_id)):
                        continue

                    her_addr = chunks[7].strip().split("=")[1]
                    timestamps[str(tenant_id)][her_addr][-1].append(extract_timestamp(chunks[0]))
                    continue

                if re.search("finished task execution", line):
                    chunks = line.strip().split(" ")
                    msg_id = chunks[6].strip().split("=")[1]
                    if (msg_id != str(tenant_id)):
                        continue

                    her_addr = chunks[7].strip().split("=")[1]
                    timestamps[str(tenant_id)][her_addr][-1].append(extract_timestamp(chunks[0]))
                    continue

                if re.search("loopback_latency", line):
                    chunks = line.strip().split(" ")
                    msg_id = chunks[6].strip().split("=")[1]
                    if (msg_id != str(tenant_id)):
                        continue

                    her_addr = chunks[7].strip().split("=")[1]
                    timestamps[str(tenant_id)][her_addr][-1].append(extract_timestamp(chunks[0]))
                    timestamps[str(tenant_id)][her_addr][-1].append(int(int(chunks[5].strip().split("=")[1]) / 1000))

                    continue


    for msg_id, tenant_storage in timestamps.items():
        for her_addr, records in tenant_storage.items():
            writer = csv.writer(outcsv)
            writer.writerows(records)


def parse_logs(logsdir, expname, modes, bmark_names, pkt_sizes):
    with open(f"{logsdir}/{expname}.csv", "w", newline="") as outcsv:
        outcsv.write("bmark_name,mode,pkt_size,task_id,receive_t,sched_t,start_t,end_t,feedback_t,loopback_lat\n")
        for bmark_name in bmark_names:
            for mode in modes:
                for packet_size in pkt_sizes:
                    parse_time(logsdir, mode, bmark_name, packet_size, outcsv)


parse_logs(
    logsdir=logs_dir,
    expname="raw_tput",
    modes=["baseline","osmosis"],
    bmark_names=["filtering_ph","histogram_l1_ph","aggregate_global_ph","io_write_ph","io_read_ph","reduce_l1_ph"],
    pkt_sizes=["64","512","1024","2048","4096"],
)
