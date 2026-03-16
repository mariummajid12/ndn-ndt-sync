# Baseline: Liang et al. (2023)

**"Named Data Networking (NDN) for Data Collection of Digital Twins-based IoT Systems"**  
Hengshuo Liang, Cheng Qian, Chao Lu, Lauren Burgess, John Mulo, Wei Yu — IEEE SERA 2023

---

## Why This Baseline

Liang (2023) is the closest prior work to the protocol we want to design that actually runs NDN + Digital Twin data collection end-to-end in ndnSIM. Its key finding motivates the core design decision of our protocol:

> At 30+ sub-networks under continuous high-rate sync, **packet reception drops to ~50%**. The network saturates. NDN alone, without intelligent sync control, cannot handle DT data collection at scale.

This is the empirical justification for protocol's selective, ODTE-triggered synchronisation instead of naive continuous polling.

---

## What This Simulation Replicates

**Scenario 1 — Small-Scale (3×3 grid)** from the paper's Table I and Figures 5–6.

### Topology

```
[0] --- [1] --- [2]      ← Consumers (DT instances)
 |       |       |
[3] --- [4] --- [5]      ← Intermediate NDN routers
 |       |       |
[6] --- [7] --- [8]      ← Producers (IoT / gNB devices)
```

- 9 nodes, 12 point-to-point links
- Content prefix: `/dt/data`
- Payload size: 1024 bytes

### Parameter Sweep (Table I)

| Parameter | Values |
|-----------|--------|
| Link bandwidth | 5, 10, 15, 20, 25 Mbps |
| Total request rate | 21,000 / 42,000 / 84,000 / 105,000 packets/s |
| Link delay | 10 ms (fixed) |
| Queue size | 10 packets per link (DropTail) |
| Simulation duration | 10 seconds |

Total runs: **20** (5 BW × 4 rates)

### Metrics

- **Packet Successful Reception Ratio** — InData / OutInterests per consumer node (paper Fig. 5)
- **Average Packet Delay (ms)** — end-to-end from Interest send to Data receive (paper Fig. 6)

---

## Environment Setup

### Option A — Docker (Recommended)

The official ndnSIM Docker image includes ns-3, ndnSIM, ndn-cxx, and NFD pre-built. This avoids all dependency and compiler issues, including Apple Silicon / ARM64 compatibility.

**1. Install Docker Desktop**  
Download from [docker.com](https://www.docker.com/products/docker-desktop/). No additional configuration needed.

**2. Pull the ndnSIM image**

```bash
docker pull ndnsim/ndnsim:latest
```

**3. Clone this repo**

```bash
git clone https://github.com/<your-username>/ndn-ndt-sync.git
cd ndn-ndt-sync
```

**4. Start the container, mounting the repo**

```bash
docker run -it --rm \
  -v $(pwd):/sim \
  ndnsim/ndnsim:latest bash
```

You are now inside the container. The repo is at `/sim`.

**5. Copy simulation file into ndnSIM scratch directory**

```bash
cp /sim/baseline/liang-2023/liang-small.cc /home/ndn/ndnSIM/ns-3/scratch/
cd /home/ndn/ndnSIM/ns-3
```

**6. Build**

```bash
./waf
```

Build takes ~1–2 minutes on first run. Subsequent builds are incremental.

---

### Option B — Native macOS (Not recommended for Apple Silicon)

ndnSIM 2.x officially supports macOS 10.10–10.12 (Intel). It does not build cleanly on Apple Silicon (M1/M2/M3/M4) without significant patching. Use Docker instead.

---

## Running the Simulation

### Single run (manual)

```bash
# Inside container, from /home/ndn/ndnSIM/ns-3
./waf --run="liang-small --bw=5 --rate=21000"
```

Arguments:
- `--bw` — link bandwidth in Mbps (5 / 10 / 15 / 20 / 25)
- `--rate` — total request rate in packets/s (21000 / 42000 / 84000 / 105000)

### Full sweep (all 20 runs)

```bash
# Inside container, from /home/ndn/ndnSIM/ns-3
bash /sim/baseline/liang-2023/run_all.sh
```

This runs each of the 20 combinations as a **separate process**. This is necessary — looping inside a single C++ process causes a Boost graph assertion failure in ndnSIM's global routing helper when `CalculateRoutes()` is called more than once.

Each run takes ~30–60 seconds. Full sweep: ~15–20 minutes.

### Output files

All output is written to `scratch/` inside the ns-3 directory:

| File | Contents |
|------|----------|
| `liang-small-l3-bwX_rY.txt` | L3 rate tracer — packet counts per second per node per face |
| `liang-small-delay-bwX_rY.txt` | App delay tracer — per-packet delay from consumer |
| `liang-small-summary.csv` | Run completion log (bandwidth, rate, status) |

---

## Parsing Results

After all runs complete, parse the L3 tracer files to compute reception ratios:

```bash
# Inside container, from /home/ndn/ndnSIM/ns-3
python3 /sim/baseline/liang-2023/parse_results.py
```

This produces `scratch/liang-small-metrics.csv` with one row per (bandwidth, rate) combination:

```
bandwidth_mbps, rate_per_sec, reception_ratio, avg_delay_ms
5, 21000, 0.97, 24.3
5, 42000, 0.81, 38.1
...
```

Parsed results are committed to `results/liang-2023/`.

---

## Expected Results

At low load (5 Mbps / 21,000 pkt/s), reception ratio is near 1.0 and delay is close to the 2-hop minimum (~20–30 ms). As load increases toward 105,000 pkt/s, queues saturate and reception ratio drops significantly — replicating the paper's finding.

The saturation behaviour at high load is the key result that motivates this protocol: an intelligent protocol that sends less, not more.

---

## Known Issues

### Boost graph crash on repeated `CalculateRoutes()`

**Symptom:** `Assertion '(std::size_t)i < pm.n' failed` in `two_bit_color_map.hpp` from run 2 onwards.

**Cause:** ndnSIM's `GlobalRoutingHelper` uses a Boost graph whose internal state is not fully reset between simulator runs in the same process.

**Fix:** Each run is a separate process (handled by `run_all.sh`). Do not loop multiple `RunScenario()` calls inside a single `main()`.

### Namespace ambiguity (`StackHelper` not found)

**Symptom:** `error: 'StackHelper' was not declared in this scope`

**Fix:** Add `using namespace ns3::ndn;` after `using namespace ns3;`. This tells the compiler to use ndnSIM's `ndn` namespace rather than ndn-cxx's conflicting `::ndn` namespace.

---

## File Reference

| File | Description |
|------|-------------|
| `liang-small.cc` | ndnSIM simulation — single run, parameterised via command line |
| `run_all.sh` | Bash sweep script — 20 runs, one process each |
| `parse_results.py` | Parses L3 tracer output → metrics CSV |

---

## Citation

```
H. Liang, C. Qian, C. Lu, L. Burgess, J. Mulo, W. Yu,
"Named Data Networking (NDN) for Data Collection of Digital Twins-based IoT Systems,"
IEEE SERA 2023.
```

