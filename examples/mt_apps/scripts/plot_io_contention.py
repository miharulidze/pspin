from matplotlib.ticker import FormatStrFormatter
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
plt.rcParams.update({'font.size': 18})

# Load data, add the completion time column and filter tenant congested traces
df = pd.read_csv("logs/send_packet_contention.csv")
df = df[df["tenant_id"] == 0]  # 1 for attacker
df = df[df["trace"] == "congested"]
df = df[(df["chunk_size"] == "baseline") | (df["chunk_size"] == "64") | (df["chunk_size"] == "512")]
df.loc[df["bmark_name"] == "send_packet_ph", "bmark_name"] = "baseline (none)"
df.loc[df["bmark_name"] == "osmosis_send_packet_ph", "bmark_name"] = "SW"  # + " " + df["chunk_size"] + "B"
df.loc[df["bmark_name"] == "osmosis_axi_send_packet_ph", "bmark_name"] = "HW"  # + " " + df["chunk_size"] + "B"
df.loc[df["chunk_size"] == "baseline", "chunk_size"] = "none"
df.loc[df["chunk_size"] == "64", "chunk_size"] = "64 B"  # + " " + df["chunk_size"] + "B"
df.loc[df["chunk_size"] == "512", "chunk_size"] = "512 B"  # + " " + df["chunk_size"] + "B"
df_l = df.copy()
df_l["latency"] = df_l["end_t"] - df_l["start_t"]
df_l = df_l.sort_values("bmark_name")
df_l = df_l.rename(columns={'bmark_name': 'Fragmentation', 'chunk_size': 'Fragment size'})
# for attacker_size in df["attacker_size"].unique():
#     mean = df.loc[(df["attacker_size"] == attacker_size) & (df["chunk_size"] == "baseline"), "completion_time"].mean()
#     df.loc[df["attacker_size"] == attacker_size, "completion_time"] /= mean
#     print(df.loc[df["attacker_size"] == attacker_size, "completion_time"])
df_t = df.copy()
df_t = df_t.groupby(by=["attacker_size", "bmark_name", "chunk_size"], as_index=False)\
    .agg(min=pd.NamedAgg(column='start_t', aggfunc='min'), max=pd.NamedAgg(column='end_t', aggfunc='max'))
df_t["throughput"] = 32 / (df_t['max'] - df_t['min']) * 10**3
df_t = df_t.sort_values("bmark_name")
df_t = df_t.rename(columns={'bmark_name': 'Fragmentation', 'chunk_size': 'Fragment size'})
print(df_t)
# order = ["baseline",
#          "HW fragmentation 512B", "HW fragmentation 64B",
#          "SW fragmentation 512B", "SW fragmentation 64B"]

# List the attacker sizes and perturb slightly each x value
attacker_sizes = list(df["attacker_size"].unique())
# delta = np.array([2.5 ** i for i in range(len(attacker_sizes))])
# chunk_sizes = list(df["chunk_size"].unique())
# chunk_sizes = chunk_sizes[1:] + [chunk_sizes[0]]
# for index, chunk_size in enumerate(chunk_sizes):
#     for attacker_index, attacker_size in enumerate(attacker_sizes):
#         df.loc[(df["chunk_size"] == chunk_size) & (df["attacker_size"] == attacker_size), "attacker_size"]\
#             += (delta * (index + 1 - len(attacker_sizes) // 2))[attacker_index]

# Plot the results
order = [str(64*2**i) for i in range(0, 8)]
f, ax = plt.subplots(2, 1, figsize=(10, 7))
sns.lineplot(x="attacker_size", y="throughput", hue="Fragmentation", style="Fragment size", markers=True, dashes=False,
             data=df_t, err_style="bars", err_kws={'capsize': 5, 'capthick': 3, 'elinewidth': 3, "barsabove": True},
             linewidth="2", markersize=13, alpha=1, ax=ax[0])
sns.lineplot(x="attacker_size", y="latency", hue="Fragmentation", style="Fragment size", markers=True, dashes=False,
             data=df_l, err_style="bars", err_kws={'capsize': 5, 'capthick': 3, 'elinewidth': 3, "barsabove": True},
             linewidth="2", markersize=13, alpha=1, ax=ax[1])

# Improve the plotting
ax[0].set_xscale("log")
ax[0].set_yscale("log")
ax[0].set_xticks(attacker_sizes, attacker_sizes)
ax[0].tick_params(axis='x', which='minor', bottom=False)
ax[0].grid(which="major")
ax[0].set_xlabel("")
ax[0].set_ylabel("Congestor\nThroughput\n[Mpps]")
ax[0].legend().remove()
lim = ax[0].get_xlim()
ax[0].axvline(x=512, color="black", zorder=0, linewidth=3)
ax[0].axvspan(lim[0], 512, facecolor='green', alpha=0.1)
ax[0].axvspan(512, lim[1], facecolor='red', alpha=0.1)
ax[0].text(560, 1.6, "Egress Bottleneck", style="italic", weight="bold")
ax[0].set_xlim(lim[0], lim[1])
ax[0].set_axisbelow(True)
ax[1].set_xscale("log")
ax[1].set_yscale("log")
ax[1].axvspan(lim[0], 512, facecolor='green', alpha=0.1)
ax[1].axvspan(512, lim[1], facecolor='red', alpha=0.1)
ax[1].set_xticks(attacker_sizes, attacker_sizes)
ax[1].tick_params(axis='x', which='minor', bottom=False)
ax[1].grid(which="major")
ax[1].set_xlabel("Congestor Size [bytes]")
ax[1].set_ylabel("Victim Completion\n time [cycles]")
ax[1].legend(bbox_to_anchor=(0.01, 0.7, 0.5, 0.5))
ax[1].axvline(x=512, color="black", zorder=0, linewidth=3)
ax[1].set_xlim(lim[0], lim[1])
ax[1].text(70, 300, "Ingress Bottleneck", style="italic", weight="bold")
ax[1].set_axisbelow(True)
plt.subplots_adjust(hspace=0.01)
plt.savefig("./figures/dma_hol_throughput.pdf",  bbox_inches='tight')
plt.show()
