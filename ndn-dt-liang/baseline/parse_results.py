"""
parse_results.py
================
Post-processing script for Liang (2023) small-scale scenario.

Reads the ndnSIM trace files produced by liang-small.cpp and computes:
  - Packet Successful Reception Ratio  (paper Fig. 5)
  - Average Packet Delay in ms         (paper Fig. 6)

Then prints a table and saves results to liang-small-metrics.csv

Usage (run from scratch/ directory or pass --dir):
    python3 /sim/parse_results.py
    python3 /sim/parse_results.py --dir /path/to/scratch
"""

import os
import sys
import csv
import argparse
from collections import defaultdict

# ── Parameters matching the simulation sweep ─────────────────────────────
BANDWIDTHS = [5, 10, 15, 20, 25]        # Mbps
RATES      = [21000, 42000, 84000, 105000]  # packets/s


def parse_l3_tracer(filepath):
    """
    Parse an L3RateTracer file.
    Returns dict with keys 'OutInterests' and 'InData' summed across all nodes.

    L3RateTracer columns:
      Time, Node, FaceId, FaceDescr, Type, Packets, Kilobytes, PacketRaw, KilobytesRaw
    We want:
      Type == OutInterests  -> total interests sent by consumers
      Type == InData        -> total data received by consumers
    """
    out_interests = 0.0
    in_data       = 0.0

    if not os.path.exists(filepath):
        return None

    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split('\t')
            if len(parts) < 6:
                continue
            type_col    = parts[4].strip()
            packets_col = parts[5].strip()
            try:
                count = float(packets_col)
            except ValueError:
                continue

            if type_col == 'OutInterests':
                out_interests += count
            elif type_col == 'InData':
                in_data += count

    if out_interests == 0:
        return None

    reception_ratio = in_data / out_interests
    return {
        'out_interests'   : out_interests,
        'in_data'         : in_data,
        'reception_ratio' : reception_ratio,
    }


def parse_delay_tracer(filepath):
    """
    Parse an AppDelayTracer file.
    Returns average delay in milliseconds.

    AppDelayTracer columns:
      Time, Node, AppId, SeqNo, Type, DelayS, DelayUS, RetxCount, HopCount
    We want Type == LastDelay, DelayUS column.
    """
    delays = []

    if not os.path.exists(filepath):
        return None

    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split('\t')
            if len(parts) < 7:
                continue
            type_col  = parts[4].strip()
            delay_col = parts[6].strip()   # DelayUS
            if type_col == 'LastDelay':
                try:
                    delay_us = float(delay_col)
                    delays.append(delay_us / 1000.0)   # convert to ms
                except ValueError:
                    continue

    if not delays:
        return None

    return sum(delays) / len(delays)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dir', default='scratch',
                        help='Directory containing trace files (default: scratch)')
    args = parser.parse_args()

    trace_dir = args.dir
    print(f"\nReading trace files from: {trace_dir}\n")

    results = []

    for bw in BANDWIDTHS:
        for rate in RATES:
            tag = f"bw{bw}_r{rate}"

            l3_file    = os.path.join(trace_dir,
                                      f"liang-small-l3-{tag}.txt")
            delay_file = os.path.join(trace_dir,
                                      f"liang-small-delay-{tag}.txt")

            l3    = parse_l3_tracer(l3_file)
            delay = parse_delay_tracer(delay_file)

            ratio     = l3['reception_ratio'] if l3    else float('nan')
            avg_delay = delay                  if delay else float('nan')

            results.append({
                'bandwidth_mbps'    : bw,
                'rate_per_sec'      : rate,
                'reception_ratio'   : ratio,
                'avg_delay_ms'      : avg_delay,
            })

    # ── Print table ────────────────────────────────────────────────────────
    print(f"{'BW (Mbps)':<12} {'Rate (/s)':<12} "
          f"{'Reception Ratio':<18} {'Avg Delay (ms)':<16}")
    print("-" * 60)
    for r in results:
        ratio_str = f"{r['reception_ratio']:.4f}" \
                    if r['reception_ratio'] == r['reception_ratio'] else "N/A"
        delay_str = f"{r['avg_delay_ms']:.2f}" \
                    if r['avg_delay_ms'] == r['avg_delay_ms'] else "N/A"
        print(f"{r['bandwidth_mbps']:<12} {r['rate_per_sec']:<12} "
              f"{ratio_str:<18} {delay_str:<16}")

    # ── Save CSV ───────────────────────────────────────────────────────────
    out_path = os.path.join(trace_dir, "liang-small-metrics.csv")
    with open(out_path, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=[
            'bandwidth_mbps', 'rate_per_sec',
            'reception_ratio', 'avg_delay_ms'])
        writer.writeheader()
        writer.writerows(results)

    print(f"\nMetrics saved to: {out_path}")


if __name__ == '__main__':
    main()
