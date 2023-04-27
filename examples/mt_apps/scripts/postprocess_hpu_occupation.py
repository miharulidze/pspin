import re
import sys
import csv

logsdir = sys.argv[1]

def extract_timestamp(raw_data):
    chunks = raw_data.strip().split("][")
    return chunks[0][1:]

# experiment name format:
# <ntenants>.blahblahblah
def parse_hpu_occup(logsdir, trace, n_tenants, cycles, prios, arbiter, csvfile):
    counter = 0

    timestamps = {}

    for tenant_id in range(n_tenants):
        timestamps[str(tenant_id)] = []
        count = 0

        #hpu_contention.v256.a512.BVT.t2mix
        with open(f"{logsdir}/hpu_contention.v{cycles[0]}.a{cycles[1]}.{arbiter}.{trace}.log", 'r', errors='replace') as log:
            assert count == 0
            while line := log.readline():
                if re.search("hpu_driver.sv", line) and len(line.strip().split(" ")) > 5:
                    chunks = line.strip().split(" ")
                    time = extract_timestamp(chunks[0])
                    msg_id = chunks[6].strip().split("=")[1]

                    if (msg_id != str(tenant_id)):
                        continue

                    if (chunks[1] == 'started'):
                        count += 1

                    elif (chunks[1] == 'finished'):
                        count -= 1

                    record = [trace, arbiter, tenant_id, prios[tenant_id], cycles[tenant_id], int(int(time)/1000), count]
                    timestamps[msg_id].append(record)

        for msg_id, records in timestamps.items():
            writer = csv.writer(csvfile)
            writer.writerows(records)

n_tenants = 2;
cycles = {}
cycles[0] = 256
cycles[1] = 512
prios = {}
prios[0] = 0
prios[1] = 0

with open(f"{logsdir}/hpu_occupation.csv", "w") as outcsv:
    outcsv.write("trace,arbiter,tenant_id,tenant_prio,tenant_cycles,time,hpu_occup")
    with open(f"{logsdir}/hpu_occupation.csv", "a") as outcsv:
        for trace in ["t2mix100.128b","t2mix300.128b","t2mix100","t2mix300"]:
            for arbiter in ["RR","BVT"]:
                parse_hpu_occup(logsdir, trace, n_tenants, cycles, prios, arbiter, outcsv)
