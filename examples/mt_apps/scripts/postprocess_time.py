import csv
import re
import sys
from pathlib import Path

n_tenants = 2
logs_dir = sys.argv[1]

def extract_timestamp(raw_data):
    chunks = raw_data.strip().split("][")
    return int(int(chunks[0][1:]) / 1000)


def parse_time(logsdir, bmark_name, victim_size, attacker_size, chunk_size, trace, outcsv):
    timestamps = {}

    filename = f"{logsdir}/2.{bmark_name}.victim{victim_size}.attacker{attacker_size}.c{chunk_size}.{trace}.log"

    if bmark_name == "dma_to_host_ph":
        filename = f"{logsdir}/2.{bmark_name}.victim{victim_size}.atacker{attacker_size}.c{chunk_size}.{trace}.log"

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
                    timestamps[str(tenant_id)][her_addr].append([bmark_name,victim_size,attacker_size,chunk_size,trace,tenant_id,her_addr,extract_timestamp(chunks[0])])
                    continue

                if re.search("sent task to scheduler", line):
                    chunks = line.strip().split(" ")
                    msg_id = chunks[5].strip().split("=")[1]
                    if (msg_id != str(tenant_id)):
                        continue

                    her_addr = chunks[6].strip().split("=")[1]
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


def parse_logs(logsdir, expname, bmark_names, victim_sizes, attacker_sizes, chunk_sizes, traces):
    with open(f"{logsdir}/{expname}.csv", "w", newline="") as outcsv:
        outcsv.write("bmark_name,victim_size,attacker_size,chunk_size,trace,tenant_id,task_id,receive_t,sched_t,start_t,end_t,feedback_t,loopback_lat\n")
        for bmark_name in bmark_names:
            for victim_size in victim_sizes:
                for attacker_size in attacker_sizes:
                    for chunk_size in chunk_sizes:
                        for trace in traces:
                            parse_time(logsdir, bmark_name, victim_size, attacker_size, chunk_size, trace, outcsv)

parse_logs(
    logsdir=logs_dir,
    expname="dma_to_host_contention",
    bmark_names=["dma_to_host_ph","osmosis_dma_to_host_ph"],
    victim_sizes=["64"],
    attacker_sizes=["64","128","256","512","1024","2048","4096"],
    chunk_sizes=["64","128","256","512","baseline"],
    traces=["no_congestion","congested"]
)
