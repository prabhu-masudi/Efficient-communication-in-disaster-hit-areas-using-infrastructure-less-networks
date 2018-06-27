#include "ns3/core-module.h"
#include "ns3/aodv-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-module.h"
//#include "myapp.h"

using namespace ns3;

int main (int argc, char* argv[])
{
  uint32_t size = 30; // number of nodes
  double totalTime =180; // simulation time
  std::string phyMode ("DsssRate1Mbps");
  bool pcap =false;
  bool enableFlowMonitor = true;
 SeedManager::SetSeed(123);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);


  cmd.Parse (argc, argv);


  std::cout << "Creating " << (unsigned)size << " nodes \n";
 

// devices setting

NetDeviceContainer devices;
  //  nodes
  NodeContainer nodes;

  nodes.Create(size);


MobilityHelper mobility;
ObjectFactory pos;

pos.SetTypeId ("ns3::RandomDiscPositionAllocator");

pos.Set ("X",StringValue ("100.0"));

pos.Set ("Y", StringValue ("100.0"));

Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                           "Speed", StringValue ("ns3::UniformRandomVariable[Min=30|Max=60]"),
                           "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
				"PositionAllocator", PointerValue (taPositionAlloc));

mobility.SetPositionAllocator (taPositionAlloc);
mobility.Install (nodes);





  //  wifi
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes);

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodv"));
    }


  // set AODV routing protocol
  AodvHelper aodv;
  //AodvHelper malicious_aodv;
  InternetStackHelper internet;
  internet.SetRoutingHelper (aodv);
internet.Install (nodes); 
//aodv.Install ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

 
  // set IP to all devices

 Ipv4InterfaceContainer interfaces;


  Ipv4AddressHelper ipadd;
ipadd.SetBase("10.0.0.0", "255.255.252.0");
ipadd.Assign(devices);
interfaces = ipadd.Assign (devices);

 
 // set UDP connection
  uint16_t port = 5000;  // well-known echo port number
  uint32_t packetSize = 1024; // size of the packets being transmitted
  uint32_t maxPacketCount = 50; // number of packets to transmit
  Time interPacketInterval = Seconds (1.); // interval between packet transmissions

  // Set-up  a server Application on the bottom-right node of the grid
  UdpEchoServerHelper server1 (port);
  uint16_t server1Position = size-1; //bottom right
  ApplicationContainer apps = server1.Install (nodes.Get(server1Position));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (totalTime-0.1));

  // Set-up a client Application, connected to 'server1', to be run on the top-left node of the grid
  UdpEchoClientHelper client1 (interfaces.GetAddress (server1Position), port);
  client1.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client1.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client1.SetAttribute ("PacketSize", UintegerValue (packetSize));
  uint16_t client1Position = 0; //top left
  apps = client1.Install (nodes.Get (client1Position));
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (totalTime-0.1));

  // Set-up a server Application on the top-right node of the grid
  UdpEchoServerHelper server2 (port);
  uint16_t server2Position = 9;
  apps = server2.Install (nodes.Get(server2Position)); //top right
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (totalTime-0.1));

  // Set-up a client Application, connected to 'server2', to be run on the bottom-left node of the grid
  UdpEchoClientHelper client2 (interfaces.GetAddress (server2Position), port);
  client2.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client2.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client2.SetAttribute ("PacketSize", UintegerValue (packetSize));
  uint16_t client2Position = size-9; //bottom left
  apps = client2.Install (nodes.Get (client2Position));
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (totalTime-0.1));

  Simulator::Stop(Seconds(totalTime));
  //wifiPhy.EnablePcap("test", device.Get(0),true);
 // AnimationInterface anim ("manet.xml");



FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Simulator::Stop(Seconds(totalTime ));


AnimationInterface anim ("aodv_final_30_5.xml");
anim.SetMaxPktsPerTraceFile(500000000);//
  Simulator::Run();

double tx=0,rx=0,tp=0,delay=0,totaltp=0,totaldelay=0,pdr=0;
  int j=0;
  monitor->CheckForLostPackets ();

  // calculate tx, rx, throughput, end to end delay, dand packet delivery ratio
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
    // if ((t.sourceAddress=="10.0.0.12" && t.destinationAddress == "10.0.0.10"))
   // {
          std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        tx=tx+ i->second.txBytes;
          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
           rx=rx+ i->second.rxBytes;
tp=i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024;
      	  std::cout << "  Throughput: " << tp  << " Mbps\n";

                totaltp=totaltp+tp;
delay=i->second.delaySum.GetSeconds() / i->second.txPackets;
          std::cout << "  End to End Delay: " << delay <<"\n";
totaldelay=totaldelay+delay;
          std::cout << "  Packet Delivery Ratio: " << (float)((i->second.rxPackets * 100) / i->second.txPackets) << "%" << "\n";
     //}
j=j+1;

     }


std::cout << "\n\n -----------------------AODV-------------------------\n\n";
std::cout << size<<" nodes "<<totalTime<< " seconds \n\n";
std::cout << "  Total Tx Bytes:   " << tx << "\n";
std::cout << "  Total Rx Bytes:   " << rx << "\n";
//std::cout << "  flow:   " << j<< "\n";
std::cout << "  Total Throughput:   " << totaltp << "\n";
totaltp=totaltp/j;
std::cout << "  Final Throughput (average):   " << totaltp << "\n";
std::cout << "  Total Delay:   " << totaldelay << "\n";
pdr=rx*100/tx;
std::cout << "  Final Packet Delivery Ratio:   " << pdr << "\n";



  Simulator::Destroy();

}
