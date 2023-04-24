import csv
import re
import sys

def extract_timestamp(raw_data):
    chunks = raw_data.strip().split("][")
    return int(int(chunks[0][1:]) / 1000)

def parse_time(expname):
    n_tenants = int(expname.split(".")[0])

    timestamps = {}

    for tenant_id in range(n_tenants):
        timestamps[str(tenant_id)] = {}

        with open(f"{expname}.log", 'r', errors='replace') as log:
            while line := log.readline():

                if re.search("sent new task to FMQ", line):
                    chunks = line.strip().split(" ")
                    msg_id = chunks[6].strip().split("=")[1]
                    if (msg_id != str(tenant_id)):
                        continue

                    her_addr = chunks[7].strip().split("=")[1]

                    if her_addr not in timestamps[str(tenant_id)].keys():
                        timestamps[str(tenant_id)][her_addr] = []
                    timestamps[str(tenant_id)][her_addr].append([tenant_id,her_addr,extract_timestamp(chunks[0])])
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

    with open(f"{expname}.time.csv", "w", newline="") as f:
        f.write("tenant_id,task_id,receive_t,sched_t,start_t,end_t,feedback_t,loopback_lat\n")
        for msg_id, tenant_storage in timestamps.items():
            for her_addr, records in tenant_storage.items():
                writer = csv.writer(f)
                writer.writerows(records)

parse_time(sys.argv[1])
