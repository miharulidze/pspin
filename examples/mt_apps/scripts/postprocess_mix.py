import csv
import re
import sys
from pathlib import Path

n_tenants = 4
logs_dir = sys.argv[1]

def extract_timestamp(raw_data):
    chunks = raw_data.strip().split("][")
    return int(int(chunks[0][1:]) / 1000)

def parse_time(logsdir, mode, mix_name, trace_name, trial, outcsv):
    timestamps = {}
    filename = f"{logsdir}/{mode}.{mix_name}.{trace_name}.log"

    if not Path(filename).is_file():
        return

    print(filename)

    for tenant_id in range(n_tenants):
        timestamps[str(tenant_id)] = {}
        fmq_size = 0
        count = 0
        pkt_sizes = []
        with open(filename, 'r', errors='replace') as log:
            while line := log.readline():
                if re.search("src=src_addr", line):
                    chunks = line.strip().split(" ")
                    msg_id = chunks[6].strip().split("=")[1]
                    if (msg_id != str(tenant_id)):
                        continue
                    pkt_size = chunks[4].strip().split("=")[1]
                    pkt_sizes.append(pkt_size)

                if re.search("sent new task to FMQ", line):
                    chunks = line.strip().split(" ")
                    msg_id = chunks[6].strip().split("=")[1]
                    if (msg_id != str(tenant_id)):
                        continue

                    pkt_size = pkt_sizes.pop(0)
                    her_addr = chunks[7].strip().split("=")[1]

                    if her_addr not in timestamps[str(tenant_id)].keys():
                        timestamps[str(tenant_id)][her_addr] = []
                    timestamps[str(tenant_id)][her_addr].append([mix_name,trial,mode,tenant_id,her_addr,pkt_size,extract_timestamp(chunks[0])])
                    continue

                if re.search("sent task to scheduler", line):
                    chunks = line.strip().split(" ")
                    msg_id = chunks[6].strip().split("=")[1]
                    if (msg_id != str(tenant_id)):
                        continue

                    her_addr = chunks[5].strip().split("=")[1]
                    timestamps[str(tenant_id)][her_addr][-1].append(extract_timestamp(chunks[0]))
                    fmq_size = chunks[7].strip().split("=")[1]
                    timestamps[str(tenant_id)][her_addr][-1].append(fmq_size)
                    continue

                if re.search("started task execution", line):
                    chunks = line.strip().split(" ")
                    msg_id = chunks[6].strip().split("=")[1]
                    if (msg_id != str(tenant_id)):
                        continue

                    count +=1
                    her_addr = chunks[7].strip().split("=")[1]
                    timestamps[str(tenant_id)][her_addr][-1].append(count)
                    timestamps[str(tenant_id)][her_addr][-1].append(extract_timestamp(chunks[0]))
                    continue

                if re.search("finished task execution", line):
                    chunks = line.strip().split(" ")
                    msg_id = chunks[6].strip().split("=")[1]
                    if (msg_id != str(tenant_id)):
                        continue

                    count -= 1
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


def parse_logs(logsdir, expname, modes, mixes, traces):
    with open(f"{logsdir}/{expname}.csv", "w", newline="") as outcsv:
        outcsv.write("mix_name,trial,mode,tenant_id,task_id,pkt_size,receive_t,sched_t,fmq_size,hpu_occup,start_t,end_t,feedback_t,loopback_lat\n")
        for mix in mixes:
            for mode in modes:
                for trial, trace in enumerate(traces):
                    parse_time(logsdir, mode, mix, trace, trial, outcsv)


parse_logs(
    logsdir=logs_dir,
    expname="mixes",
    modes=["baseline","osmosis"],
    mixes=["compute","io"],
    traces=["1000.0"]
)

parse_logs(logsdir=logs_dir, expname="compute_mix",modes=["baseline","osmosis"],mixes=["compute"],traces=["500.0"])
parse_logs(logsdir=logs_dir, expname="io_mix",modes=["baseline","osmosis"],mixes=["io"],traces=["500.0"])
