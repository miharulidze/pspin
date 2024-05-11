import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import metrics
plt.rcParams.update({'font.size': 14})
WINDOW_SIZE = 1
LINE_WIDTH = 2
TOTAL_NUMBER_OF_HPUS = 32
START_TIME = 1115


def jain(x1, t1, f1, x2, t2, f2):
    results = []
    values1 = {}
    values2 = {}
    for value, time, fmq in zip(x1, t1, f1):
        values1[time] = (value, fmq)
    for value, time, fmq in zip(x2, t2, f2):
        values2[time] = (value, fmq)
    start = np.min([t1[0], t2[0]])
    end = np.max([t1[-1], t2[-1]])
    times = [i for i in range(start, end + 1)]
    for time in times:
        values = []
        if time in values1 and values1[time][1] != 0:
            values.append(values1[time][0])
        if time in values2 and values2[time][1] != 0:
            values.append(values2[time][0])
        results.append(metrics.jain(np.array(values)))

    return results, times


def parse_events(hpu_events, hpu_occupation, fmq_size):
    times = []
    occupations = []
    fmqs = []

    for index, event in enumerate(hpu_events[:-1]):
        rgn = range(event, hpu_events[index + 1])
        times += list(rgn)
        occupations += [hpu_occupation[index] for _ in rgn]
        fmqs += [fmq_size[index] for _ in rgn]
    times.append(hpu_events[-1])
    occupations.append(hpu_occupation[-1])
    fmqs.append(fmq_size[-1])

    return times, occupations, fmqs


# Load data, add the completion time column and filter tenant congested traces
df = pd.read_csv("logs/hpu_occupation.csv")
df = df[df["trace"] == "t2mix100.128b"]
fig, ax = plt.subplots(3, 1, figsize=(7,3), gridspec_kw={'height_ratios': [1.4, 1, 1]})

# RR
current = df[df["arbiter"] == "RR"]
app1 = current[current["tenant_id"] == 0].sort_values(by="time")
app1_times, app1_occupations, app1_fmqs = parse_events(list(app1["time"]), list(app1["hpu_occup"]), list(app1["fmq_size"]))
if WINDOW_SIZE > 1:
    app1_occupations = np.convolve(app1_occupations, np.ones(WINDOW_SIZE)/WINDOW_SIZE, mode='same')
app2 = current[current["tenant_id"] == 1].sort_values("time")
app2_times, app2_occupations, app2_fmqs = parse_events(list(app2["time"]), list(app2["hpu_occup"]), list(app2["fmq_size"]))
if WINDOW_SIZE > 1:
    app2_occupations = np.convolve(app2_occupations, np.ones(WINDOW_SIZE)/WINDOW_SIZE, mode='same')
# gaussian_filter1d(app2_occupations, 50, mode="nearest")
fairness, fairness_times = jain(app1_occupations, app1_times, app1_fmqs, app2_occupations, app2_times, app2_fmqs)

# Plot the results
print(min(app1_times), min(app2_times))
ax[1].plot([i-START_TIME for i in app1_times], [i/TOTAL_NUMBER_OF_HPUS*100 for i in app1_occupations], label="Tenant 1", linewidth=LINE_WIDTH)
ax[1].plot([i-START_TIME for i in app2_times], [i/TOTAL_NUMBER_OF_HPUS*100 for i in app2_occupations], label="Tenant 2", linewidth=LINE_WIDTH)
ax[1].set_ylabel("")
ax[0].set_ylabel("fairness")
ax[0].get_xaxis().set_ticklabels([])
ax[0].plot([i-START_TIME for i in fairness_times], fairness, label="RR", color="red", linewidth=LINE_WIDTH)
# plt.scatter(list(app1["time"]), list(app1["hpu_occup"]))
# plt.scatter(list(app2["time"]), lis   t(app2["hpu_occup"]))

# WLBVT
current = df[df["arbiter"] == "WLBVT"]
app1 = current[current["tenant_id"] == 0].sort_values("time")
app1_times, app1_occupations, app1_fmqs = parse_events(list(app1["time"]), list(app1["hpu_occup"]), list(app1["fmq_size"]))
if WINDOW_SIZE > 1:
    app1_occupations = np.convolve(app1_occupations, np.ones(WINDOW_SIZE)/WINDOW_SIZE, mode='same')
app2 = current[current["tenant_id"] == 1].sort_values("time")
app2_times, app2_occupations, app2_fmqs = parse_events(list(app2["time"]), list(app2["hpu_occup"]), list(app2["fmq_size"]))
if WINDOW_SIZE > 1:
    app2_occupations = np.convolve(app2_occupations, np.ones(WINDOW_SIZE)/WINDOW_SIZE, mode='same')
# gaussian_filter1d(app2_occupations, 50, mode="nearest")
fairness, fairness_times = jain(app1_occupations, app1_times, app1_fmqs, app2_occupations, app2_times, app2_fmqs)

# Plot the results
ax[2].plot([i-START_TIME for i in app1_times], [i/TOTAL_NUMBER_OF_HPUS*100 for i in app1_occupations], label="Tenant 1", linewidth=LINE_WIDTH)
ax[2].plot([i-START_TIME for i in app2_times], [i/TOTAL_NUMBER_OF_HPUS*100 for i in app2_occupations], label="Tenant 2", linewidth=LINE_WIDTH)
ax[2].set_ylabel("PU occup. [%]")
ax[2].yaxis.set_label_coords(-0.09, 1.0)
ax[2].set_xlabel("Time [cycles]")
ax[2].set_ylim((0, 98))
ax[1].set_ylim((0, 98))
ax[2].text(3950-START_TIME, 1000/TOTAL_NUMBER_OF_HPUS, "WLBVT", fontweight="bold")
ax[1].text(4100-START_TIME, 1000/TOTAL_NUMBER_OF_HPUS, "RR", fontweight="bold")
ax[1].get_xaxis().set_ticklabels([])
ax[0].plot([i-START_TIME for i in fairness_times], fairness, label="WLBVT", color="green", linewidth=LINE_WIDTH)
ax[0].legend(bbox_to_anchor=(1.015, -0.07), loc="lower right")
ax[0].set_ylim(top=1.05)

ticks = ax[2].get_xticks()
limits = ax[2].get_xlim()
ax[1].set_xticks(ticks=ticks)
ax[1].set_xlim(limits)
ax[0].set_xticks(ticks=ticks)
ax[0].set_xlim(limits)
for index in range(3):
    ax[index].grid()
    ax[index].set_axisbelow(True)

plt.subplots_adjust(hspace=0.1)
plt.savefig("./figures/hpu_occupation.pdf",  bbox_inches='tight')
plt.show()
