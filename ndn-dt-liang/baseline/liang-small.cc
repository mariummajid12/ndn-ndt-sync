/* =============================================================
 * Replication of Liang et al. (2023)
 * "NDN for Data Collection of Digital Twins-based IoT Systems"
 * IEEE SERA 2023 — SCENARIO 1: Small-Scale 3x3 Grid
 *
 * Runs ONE bandwidth+rate combination per execution.
 * Use run_all.sh to sweep all 20 combinations automatically.
 *
 * Usage:
 *   ./waf --run="liang-small --bw=5 --rate=21000"
 *
 * Topology (9 nodes):
 *   [0] --- [1] --- [2]      <- Consumers
 *    |       |       |
 *   [3] --- [4] --- [5]      <- Intermediate
 *    |       |       |
 *   [6] --- [7] --- [8]      <- Producers
 * ============================================================= */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include <iostream>
#include <string>

using namespace ns3;
using namespace ns3::ndn;

int main(int argc, char *argv[])
{
  // ── Command-line parameters ────────────────────────────────────────────
  double   bwMbps          = 5.0;
  uint32_t totalRatePerSec = 21000;

  CommandLine cmd;
  cmd.AddValue("bw",   "Link bandwidth in Mbps",        bwMbps);
  cmd.AddValue("rate", "Total request rate (packets/s)", totalRatePerSec);
  cmd.Parse(argc, argv);

  LogComponentEnableAll(LOG_NONE);

  std::cout << "Running: BW=" << bwMbps << "Mbps"
            << "  Rate=" << totalRatePerSec << "/s" << std::endl;

  // ── 1. Create 9 nodes ──────────────────────────────────────────────────
  NodeContainer nodes;
  nodes.Create(9);

  // ── 2. Links: 10ms delay, variable BW, queue=10 (Table I) ─────────────
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate",
      StringValue(std::to_string((int)bwMbps) + "Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("10ms"));
  p2p.SetQueue("ns3::DropTailQueue<Packet>",
               "MaxSize", StringValue("10p"));

  // ── 3. Wire 3x3 grid ──────────────────────────────────────────────────
  p2p.Install(nodes.Get(0), nodes.Get(1)); // row 0
  p2p.Install(nodes.Get(1), nodes.Get(2));
  p2p.Install(nodes.Get(3), nodes.Get(4)); // row 1
  p2p.Install(nodes.Get(4), nodes.Get(5));
  p2p.Install(nodes.Get(6), nodes.Get(7)); // row 2
  p2p.Install(nodes.Get(7), nodes.Get(8));
  p2p.Install(nodes.Get(0), nodes.Get(3)); // col 0
  p2p.Install(nodes.Get(3), nodes.Get(6));
  p2p.Install(nodes.Get(1), nodes.Get(4)); // col 1
  p2p.Install(nodes.Get(4), nodes.Get(7));
  p2p.Install(nodes.Get(2), nodes.Get(5)); // col 2
  p2p.Install(nodes.Get(5), nodes.Get(8));

  // ── 4. NDN stack ───────────────────────────────────────────────────────
  StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // ── 5. Routing ─────────────────────────────────────────────────────────
  GlobalRoutingHelper routingHelper;
  routingHelper.InstallAll();

  // ── 6. Producers on nodes 6, 7, 8 ─────────────────────────────────────
  AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/dt/data");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(nodes.Get(6));
  producerHelper.Install(nodes.Get(7));
  producerHelper.Install(nodes.Get(8));

  routingHelper.AddOrigins("/dt/data", nodes.Get(6));
  routingHelper.AddOrigins("/dt/data", nodes.Get(7));
  routingHelper.AddOrigins("/dt/data", nodes.Get(8));

  // ── 7. Consumers on nodes 0, 1, 2 ─────────────────────────────────────
  double perConsumerFreq = (double)totalRatePerSec / 3.0;

  AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/dt/data");
  consumerHelper.SetAttribute("Frequency",
      StringValue(std::to_string(perConsumerFreq)));
  consumerHelper.Install(nodes.Get(0));
  consumerHelper.Install(nodes.Get(1));
  consumerHelper.Install(nodes.Get(2));

  // ── 8. Routes ──────────────────────────────────────────────────────────
  GlobalRoutingHelper::CalculateRoutes();

  // ── 9. Tracers ─────────────────────────────────────────────────────────
  std::string tag = "bw" + std::to_string((int)bwMbps)
                  + "_r"  + std::to_string(totalRatePerSec);

  L3RateTracer::InstallAll(
      "scratch/liang-small-l3-" + tag + ".txt", Seconds(1.0));
  AppDelayTracer::InstallAll(
      "scratch/liang-small-delay-" + tag + ".txt");

  // ── 10. Run ────────────────────────────────────────────────────────────
  Simulator::Stop(Seconds(10.0));
  Simulator::Run();
  Simulator::Destroy();

  std::cout << "  DONE" << std::endl;
  return 0;
}
