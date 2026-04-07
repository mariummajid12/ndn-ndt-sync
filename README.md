# ndn-ndt-sync

**NDN-based Network Digital Twin Synchronisation Protocol for O-RAN / 6G**

This repository contains the simulation work and protocol design for synchronization between Physical Twin and Network Digital Twin — an MSc research internship project at [LIST Luxembourg](https://www.list.lu), supervised by Dr. Ayat Zaki Hindi.

---

## Research Summary

Network Digital Twins (NDTs) require continuous, low-latency state synchronisation from physical network nodes to their virtual replicas. Existing solutions use IP-based protocols (gRPC/gNMI, NETCONF/YANG) which suffer from redundant data transfer, lack of caching, and poor mobility support in 6G O-RAN architectures.

This project proposes using **Named Data Networking (NDN)** as the synchronisation substrate, with synchronisation decisions driven by **intent class** (URLLC / eMBB / mMTC) and the **ODTE** (Operational Digital Twin Effectiveness) quality metric.

### Key Contribution

A three-tier sync protocol:

| Tier | Name | Trigger | Mechanism |
|------|------|---------|-----------|
| 1 | Predictive Sync | ML-predicted state volatility | Interest sent early; compensates for T_transfer variability |
| 2 | Corrective Sync | ODTE trending below threshold | Increased sync frequency for affected data types |
| 3 | Emergency Sync | ODTE below threshold | Aggressive Interest rate, MustBeFresh, sensitivity-ranked variable selection |

---

## Repository Structure

```
ndn-ndt-sync/
│
├── baseline/
│   └── liang-2023/          ← Replication of Liang et al. (2023) — NDN for DT-IoT
│       ├── liang-small.cc   ← ndnSIM simulation (3×3 grid, Scenario 1)
│       ├── run_all.sh       ← Parameter sweep script (5 BW × 4 rates = 20 runs)
│       ├── parse_results.py ← Parses L3 tracer output → metrics CSV
│       └── README.md        ← Full setup and reproduction guide
│
├── results/
│   └── liang-2023/
│       ├── liang-small-metrics.csv   ← Parsed reception ratio + delay per run
│       └── liang-small-summary.csv  ← Run completion log
│
└── docs/
    └── problem-definition.md
```

> **Raw trace files** (`*-l3-*.txt`, `*-delay-*.txt`) are excluded via `.gitignore` — they are large and fully reproducible from the simulation source.

---

## Key Papers

| Paper | Role in this project |
|-------|---------------------|
| Liang et al. (2023) — *NDN for DT-IoT Data Collection* | Baseline: proves naive NDN sync saturates at ~50% packet drop beyond 30 sub-networks. Motivates INDS selective sync. |
| Bellavista et al. (2024) — *ODTE Metric* | ODTE = T × R × A. Dropping below threshold is the trigger for INDS sync tiers. |
| Kalasapura (2023) — *TwinSync* | Variable sensitivity logic used in Tier 3 emergency sync. |
| Sengendo & Granelli (2024) — *NDT Sync Delay Chain* | Formal delay decomposition; negative delay via ML prediction motivates Tier 1. |

---

## Status

- [x] Problem definition formalised
- [x] Liang (2023) baseline simulation implemented and validated
- [ ] Protocol design (in progress)
- [ ] ndnSIM implementation of Tier 1
- [ ] Evaluation against gNMI baseline

---

## Environment

Simulations run inside the official **ndnSIM Docker container**. See [`baseline/liang-2023/README.md`](baseline/liang-2023/README.md) for full setup instructions.

```bash
# Quick start (after Docker setup)
docker run -it --rm -v $(pwd):/sim ndnsim/ndnsim:latest bash
```

---

## Author

Marium — MSc Cybersecurity & Cyber Defence, University of Luxembourg  
Research Internship, LIST Luxembourg
