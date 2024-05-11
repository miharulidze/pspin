import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import metrics
import seaborn as sns
from matplotlib.lines import Line2D
plt.rcParams.update({'font.size': 18})
WINDOW_SIZE = 1000
LINE_WIDTH = 2
TOTAL_NUMBER_OF_HPUS = 32

colors = sns.color_palette().as_hex()

def jain(x1, t1, f1, x2, t2, f2, x3, t3, f3, x4, t4, f4):
    results = []
    values1 = {}
    values2 = {}
    values3 = {}
    values4 = {}
    for value, time, fmq in zip(x1, t1, f1):
        values1[time] = (value, fmq)
    for value, time, fmq in zip(x2, t2, f2):
        values2[time] = (value, fmq)
    for value, time, fmq in zip(x3, t3, f3):
        values3[time] = (value, fmq)
    for value, time, fmq in zip(x4, t4, f4):
        values4[time] = (value, fmq)
    start = np.min([t1[0], t2[0], t3[0], t4[0]])
    end = np.max([t1[-1], t2[-1], t3[-1], t4[-1]])
    times = [i for i in range(start, end + 1)]
    for time in times:
        values = []
        if time in values1 and values1[time][1] != 0:
            values.append(values1[time][0])
        if time in values2 and values2[time][1] != 0:
            values.append(values2[time][0])
        if time in values3 and values3[time][1] != 0:
            values.append(values3[time][0])
        if time in values4 and values4[time][1] != 0:
            values.append(values4[time][0])
        results.append(metrics.jain(np.array(values)))

    return results, times


def convolve(lst):
    return np.convolve(lst, np.ones(WINDOW_SIZE) / WINDOW_SIZE, mode='same')


def parse_events(dataframe, tenant_id):
    current_df = dataframe[(dataframe["tenant_id"] == tenant_id) & (dataframe["trial"] == 0)].sort_values("sched_t")
    min_t = current_df["sched_t"].min()
    max_t = current_df["end_t"].max()
    times = [i for i in range(min_t, max_t + 1)]
    occupations = [0 for _ in range(min_t, max_t + 1)]
    throughputs = [0 for _ in range(min_t, max_t + 1)]
    fmqs = [0 for i in range(min_t, max_t + 1)]
    for index, row in enumerate(current_df.iterrows()):
        row = row[1]
        start_i = row['start_t'] - min_t
        sched_i = row['sched_t'] - min_t
        end_i = row['end_t'] - min_t
        fmq = row['fmq_size']
        throughput = row['io_throughput']
        for i in range(sched_i, max_t - min_t + 1):
            if i >= start_i and i <= end_i:
                occupations[i] += 1
                throughputs[i] += throughput
            fmqs[i] = fmq

    return times, occupations, fmqs


# Load data, add the completion time column and filter tenant congested traces
df = pd.read_csv("logs/compute_mix.csv")
df = df[df["mix_name"] == "compute"]
df["completion_time"] = df["end_t"] - df["start_t"]
df["io_throughput"] = df["pkt_size"] / (df["end_t"] - df["start_t"])
df.loc[(df["tenant_id"] == 0) | (df["tenant_id"] == 2), "io_throughput"] *= 2
fig, ax = plt.subplots(3, 1, figsize=(14, 10), gridspec_kw={'height_ratios': [1.4, 1, 1]})

# WLBVT
current = df[df["mode"] == "osmosis"]
app1_times, app1_occupations, app1_fmqs = parse_events(current, 0)
app2_times, app2_occupations, app2_fmqs = parse_events(current, 1)
app3_times, app3_occupations, app3_fmqs = parse_events(current, 2)
app4_times, app4_occupations, app4_fmqs = parse_events(current, 3)
fairness, fairness_times = jain(app1_occupations, app1_times, app1_fmqs, app2_occupations, app2_times, app2_fmqs,
                                app3_occupations, app3_times, app3_fmqs, app4_occupations, app4_times, app4_fmqs)

mean_wlbvt_fairness = np.mean(fairness)
fairness = convolve(fairness)
max_WLBVT = []
time_WLBVT = []
for tenant_id in current["tenant_id"].unique():
    max_WLBVT.append(current.loc[(current["tenant_id"] == tenant_id) & (current["trial"] == 0), "end_t"].max())
    time_WLBVT.append(current.loc[(current["tenant_id"] == 1) & (current["trial"] == 0), "start_t"].min() - max_WLBVT[-1])

# Plot the results
ax[2].plot(app1_times, [i/TOTAL_NUMBER_OF_HPUS*100 for i in app1_occupations], label="Tenant 1", linewidth=LINE_WIDTH)
ax[2].plot(app2_times, [i/TOTAL_NUMBER_OF_HPUS*100 for i in app2_occupations], label="Tenant 2", linewidth=LINE_WIDTH)
ax[2].plot(app3_times, [i/TOTAL_NUMBER_OF_HPUS*100 for i in app3_occupations], label="Tenant 3", linewidth=LINE_WIDTH)
ax[2].plot(app4_times, [i/TOTAL_NUMBER_OF_HPUS*100 for i in app4_occupations], label="Tenant 4", linewidth=LINE_WIDTH)
ax[2].set_ylabel("PU occup. [%]")
ax[2].set_xlabel("Simulated Time [cycles]")
ax[0].plot(fairness_times, fairness, label="WLBVT", color="green", linewidth=LINE_WIDTH)

# RR
current = df[df["mode"] == "baseline"]
app1_times, app1_occupations, app1_fmqs = parse_events(current, 0)
app2_times, app2_occupations, app2_fmqs = parse_events(current, 1)
app3_times, app3_occupations, app3_fmqs = parse_events(current, 2)
app4_times, app4_occupations, app4_fmqs = parse_events(current, 3)
fairness, fairness_times = jain(app1_occupations, app1_times, app1_fmqs, app2_occupations, app2_times, app2_fmqs,
                                app3_occupations, app3_times, app3_fmqs, app4_occupations, app4_times, app4_fmqs)
mean_rr_fairness = np.mean(fairness)
fairness = convolve(fairness)
max_RR = []
time_RR = []
for tenant_id in current["tenant_id"].unique():
    max_RR.append(current.loc[(current["tenant_id"] == tenant_id) & (current["trial"] == 0), "end_t"].max())
    time_RR.append(current.loc[(current["tenant_id"] == 1) & (current["trial"] == 0), "start_t"].min() - max_RR[-1])

# Plot the results
ax[1].plot(app1_times, [i/TOTAL_NUMBER_OF_HPUS*100 for i in app1_occupations]   , label="Tenant 1", linewidth=LINE_WIDTH)
ax[1].plot(app2_times, [i/TOTAL_NUMBER_OF_HPUS*100 for i in app2_occupations], label="Tenant 2", linewidth=LINE_WIDTH)
ax[1].plot(app3_times, [i/TOTAL_NUMBER_OF_HPUS*100 for i in app3_occupations], label="Tenant 3", linewidth=LINE_WIDTH)
ax[1].plot(app4_times, [i/TOTAL_NUMBER_OF_HPUS*100 for i in app4_occupations], label="Tenant 4", linewidth=LINE_WIDTH)
ax[1].set_ylabel("PU occup. [%]")
ax[0].set_ylabel("Fairness")
ax[0].get_xaxis().set_ticklabels([])
ax[0].plot(fairness_times, fairness, label="RR", color="red", linewidth=LINE_WIDTH)
ax[2].text(41000, 18, "WLBVT", fontweight="bold")
ax[1].text(42300, 18, "RR", fontweight="bold")

for index in range(len(max_RR)):
    ax[1].axvline(max_RR[index], linewidth=3, color=colors[index], alpha=0.3, linestyle="--")
    ax[2].axvline(max_RR[index], linewidth=3, color=colors[index], alpha=0.3, linestyle="--")
    ax[2].axvline(max_WLBVT[index], linewidth=3, color=colors[index], alpha=0.3, linestyle="--")
    ax[2].annotate("", xy=(max_WLBVT[index], 9.5-(index % 2)*6), xytext=(max_RR[index], 9.5-(index % 2)*6),
                   arrowprops={"arrowstyle": '<->', "color": colors[index], "linewidth": 2})
    if index == 3:
        ax[2].text((max_WLBVT[index] + max_RR[index]) / 2 + 200, 10 - (index % 2) * 6,
                   f"{(1 - time_WLBVT[index] / time_RR[index]) * 100:.2g}%",
                   fontweight="bold", color=colors[index], fontsize=19)
    elif index == 2:
        ax[2].text((max_WLBVT[index] + max_RR[index]) / 2 - 2300, 10 - (index % 2) * 6 - 4,
                   f"{(1 - time_WLBVT[index] / time_RR[index]) * 100:.2g}%",
                   fontweight="bold", color=colors[index], fontsize=19)
    else:
        ax[2].text((max_WLBVT[index]+max_RR[index])/2-1000, 10-(index % 2)*6, f"{(1-time_WLBVT[index]/time_RR[index])*100:.2g}%",
               fontweight="bold", color=colors[index], fontsize=19)

ax[1].get_xaxis().set_ticklabels([])
ax[0].legend(loc="center", frameon=False, bbox_to_anchor=(0.5, 0.35),
             labels=[r"WLBVT mean score: $\bf{" + f"{mean_wlbvt_fairness:.3g}" + "}$",
                     r"RR mean score: $\bf{" + f"{mean_rr_fairness:.3g}" + "}$"])
ax[0].set_ylim(top=1.05)
ticks = ax[2].get_xticks()
limits = ax[2].get_xlim()
ax[1].set_xticks(ticks=ticks)
ax[1].set_xlim(limits)
ax[0].set_xticks(ticks=ticks)
ax[0].set_xlim(limits)
ax[2].legend(handles=[Line2D([0], [0], color=colors[0], lw=4), Line2D([0], [0], color=colors[1], lw=4),
                      Line2D([0], [0], color=colors[2], lw=4), Line2D([0], [0], color=colors[3], lw=4)],
             loc="upper center", frameon=False, bbox_to_anchor=(0.5, 1.23), ncols=4, columnspacing=0.8,
             labels=["Reduce (V)", "Histogram (V)", "Reduce (C)", "Histogram (C)"])
plt.subplots_adjust(hspace=0.13)

for index in range(3):
    ax[index].grid()
    ax[index].set_axisbelow(True)

plt.savefig("./figures/hpu_mixes_compute.pdf", bbox_inches='tight')
plt.show()
