#!/bin/bash
BANDWIDTHS="5 10 15 20 25"
RATES="21000 42000 84000 105000"
TOTAL=20
RUN=0

echo "bandwidth_mbps,rate_per_sec,status" > scratch/liang-small-summary.csv

for BW in $BANDWIDTHS; do
  for RATE in $RATES; do
    RUN=$((RUN + 1))
    echo ""
    echo "[$RUN/$TOTAL]  BW=${BW}Mbps  Rate=${RATE}/s"
    ./waf --run="liang-small --bw=$BW --rate=$RATE" 2>/dev/null
    if [ $? -eq 0 ]; then
      echo "$BW,$RATE,done" >> scratch/liang-small-summary.csv
      echo "  -> OK"
    else
      echo "$BW,$RATE,FAILED" >> scratch/liang-small-summary.csv
      echo "  -> FAILED"
    fi
  done
done

echo ""
echo "============================================"
echo "All $TOTAL runs complete."
echo "Trace files in scratch/:"
echo "  liang-small-l3-*.txt     (packet counts)"
echo "  liang-small-delay-*.txt  (per-packet delay)"
echo "  liang-small-summary.csv  (run log)"
echo "============================================"
