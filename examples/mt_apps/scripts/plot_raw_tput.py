import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import numpy as np
plt.rcParams.update({'font.size': 18})

# Load data, add the completion time column and filter tenant congested traces
df = pd.read_csv("logs/raw_tput.csv")
df = df.groupby(by=["bmark_name", "mode", "pkt_size"], as_index=False)\
    .agg(min=pd.NamedAgg(column='start_t', aggfunc='min'), max=pd.NamedAgg(column='end_t', aggfunc='max'),
         count=pd.NamedAgg(column='start_t', aggfunc='count'))

df["throughput"] = df['count'] / (df['max'] - df['min']) * 10**3
df["relative_throughput"] = df["throughput"]

for application in df["bmark_name"].unique():
    for pkt_size in df["pkt_size"].unique():
        rows = (df["bmark_name"] == application) & (df["pkt_size"] == pkt_size)
        baseline = float(df.loc[rows & (df["mode"] == "baseline"), "relative_throughput"])
        osmosis = float(df.loc[rows & (df["mode"] == "osmosis"), "relative_throughput"])
        # print(application, pkt_size, baseline, osmosis)
        df.loc[rows & (df["mode"] == "osmosis"), "relative_throughput"] = osmosis/baseline * 100
df = df[df["mode"] == "osmosis"]
for pkt_size in df["pkt_size"].unique():
    df.loc[df["pkt_size"] == pkt_size, "pkt_size"] = f"{pkt_size} B"
df.loc[df["bmark_name"] == "aggregate_global_ph", "bmark_name"] = "Aggregate"
df.loc[df["bmark_name"] == "filtering_ph", "bmark_name"] = "Filtering"  # + " " + df["chunk_size"] + "B"
df.loc[df["bmark_name"] == "histogram_l1_ph", "bmark_name"] = "Histogram"  # + " " + df["chunk_size"] + "B"
df.loc[df["bmark_name"] == "io_read_ph", "bmark_name"] = "IO read"  # + " " + df["chunk_size"] + "B"
df.loc[df["bmark_name"] == "io_write_ph", "bmark_name"] = "IO write"  # + " " + df["chunk_size"] + "B"
df.loc[df["bmark_name"] == "reduce_l1_ph", "bmark_name"] = "Reduce"  # + " " + df["chunk_size"] + "B"
order = ["Aggregate", "Reduce", "Histogram", "IO read", "IO write", "Filtering"]

# Plot the results
print(df)
f, ax = plt.subplots(figsize=(10, 4))

sns.barplot(data=df, x="bmark_name", y="relative_throughput", hue="pkt_size", order=order)
plt.setp(ax.patches, linewidth=3)
for index, pkt_size in enumerate(df["pkt_size"].unique()):
    labels = []
    for bmark_name in order:
        value = float(df.loc[(df["pkt_size"] == pkt_size) & (df["bmark_name"] == bmark_name), "throughput"])
        labels.append(f"{value:.3g}")
    ax.bar_label(ax.containers[index], label_type='edge', rotation=90, labels=labels, padding=3)

# Improve the plotting
# plt.xscale("log")
plt.minorticks_off()
plt.legend(title="", bbox_to_anchor=(-0.10, 1.0), loc="lower left", ncols=5, frameon=False, columnspacing=0.8)
ax.set_ylim(50, 110)
ax.set_axisbelow(True)
# plt.xticks(attacker_sizes, attacker_sizes)
plt.grid(axis="y", which="major")
plt.xlabel("")
plt.ylabel("Relative Packet Throughput [%]")
plt.savefig("./figures/application_tput.pdf",  bbox_inches='tight')
plt.show()
