import re
import sys
import csv

def extract_timestamp(raw_data):
    chunks = raw_data.strip().split("][")
    return chunks[0][1:]

# experiment name format:
# <ntenants>.blahblahblah
def parse_hpu_occup(expname):
    n_tenants = int(expname.split(".")[0])
    counter = 0
    timestamps = {}

    for tenant_id in range(n_tenants):
        timestamps[str(tenant_id)] = []
        count = 0

        with open(f"{expname}.log", 'r', errors='replace') as log:
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

                    timestamps[msg_id].append([tenant_id, int(int(time)/1000), count])

        with open(f"{expname}.hpu_occupation.csv", "w", newline="") as f:
            f.write("tenant_id,time,hpu_occup\n")
        for msg_id, records in timestamps.items():
            with open(f"{expname}.hpu_occupation.csv", "a", newline="") as f:
                writer = csv.writer(f)
                writer.writerows(records)

parse_hpu_occup(sys.argv[1])
