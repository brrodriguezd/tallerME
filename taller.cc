
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/wifi-helper.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ManetSimulation");

// se define para rastrear los cambios de posición de los nodos.
void CourseChangeCallback(std::string path, Ptr<const MobilityModel> model)
{
    Vector position = model->GetPosition();
    std::cout << "CourseChange " << path << " x=" << position.x << ", y=" << position.y
        << ", z=" << position.z << std::endl;
}

int main(int argc, char* argv[])
{
    // Log Component
    LogComponentEnable("ManetSimulation", LOG_LEVEL_INFO);

    // Local Variables
    uint32_t backboneNodes = 10;
    uint32_t infraNodes = 2;
    uint32_t lanNodes = 2;
    uint32_t stopTime = 20;
    // Activar el CourseChangeCallback
    bool useCourseChangeCallback = false;

    // Set default values for simulation parameters
    Config::SetDefault("ns3::OnOffApplication::PacketSize", StringValue("1472"));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("100kb/s"));

    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("backboneNodes", "number of backbone nodes", backboneNodes);
    cmd.AddValue("infraNodes", "number of leaf nodes", infraNodes);
    cmd.AddValue("lanNodes", "number of LAN nodes", lanNodes);
    cmd.AddValue("stopTime", "simulation stop time (seconds)", stopTime);
    cmd.AddValue("useCourseChangeCallback",
        "whether to enable course change tracing",
        useCourseChangeCallback);
    cmd.Parse(argc, argv);

    // Create nodes for clusters
    NodeContainer clusterA, clusterB, clusterC;
    clusterA.Create(3);
    clusterB.Create(3);
    clusterC.Create(3);

    // Install WiFi and Internet stack
    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("DsssRate1Mbps"));

    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());

    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devicesA = wifi.Install(wifiPhy, wifiMac, clusterA);
    NetDeviceContainer devicesB = wifi.Install(wifiPhy, wifiMac, clusterB);
    NetDeviceContainer devicesC = wifi.Install(wifiPhy, wifiMac, clusterC);

    // Install Internet stack and AODV routing protocol
    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(clusterA);
    internet.Install(clusterB);
    internet.Install(clusterC);

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfacesA = ipv4.Assign(devicesA);
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfacesB = ipv4.Assign(devicesB);
    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interfacesC = ipv4.Assign(devicesC);

    // Set mobility for Cluster A (static)
    MobilityHelper mobilityA;
    mobilityA.SetPositionAllocator("ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(5.0),
        "DeltaY", DoubleValue(10.0),
        "GridWidth", UintegerValue(3),
        "LayoutType", StringValue("RowFirst"));
    // movilidad estatica
    mobilityA.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityA.Install(clusterA);

    // Set mobility for Cluster B (Random Waypoint)
    MobilityHelper mobilityB;
    mobilityB.SetPositionAllocator("ns3::GridPositionAllocator",
        "MinX", DoubleValue(50.0),
        "MinY", DoubleValue(50.0),
        "DeltaX", DoubleValue(5.0),
        "DeltaY", DoubleValue(10.0),
        "GridWidth", UintegerValue(3),
        "LayoutType", StringValue("RowFirst"));
    // movilidad (Random Waypoint)
    mobilityB.SetMobilityModel("ns3::RandomWaypointMobilityModel",
        "Speed", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=5.0]"),
        "Pause", StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
        "PositionAllocator", PointerValue(CreateObject<RandomRectanglePositionAllocator>()));
    mobilityB.Install(clusterB);

    // Set mobility for Cluster C (Random Walk)
    MobilityHelper mobilityC;
    mobilityC.SetPositionAllocator("ns3::GridPositionAllocator",
        "MinX", DoubleValue(100.0),
        "MinY", DoubleValue(100.0),
        "DeltaX", DoubleValue(5.0),
        "DeltaY", DoubleValue(10.0),
        "GridWidth", UintegerValue(3),
        "LayoutType", StringValue("RowFirst"));
    // movilidad (Random Walk)
    mobilityC.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        "Mode", StringValue("Time"),
        "Time", StringValue("2s"),
        "Speed", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
        "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobilityC.Install(clusterC);

    // ---------------- Activar el CourseChangeCallback -----------------
    // se activa arriba
    // permiten registrar y mostrar las posiciones de los nodos cada vez que cambian de curso (posición).
    if (useCourseChangeCallback)
    {
        Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange",
            MakeCallback(&CourseChangeCallback));
    }
    // ------------------------------------------------------------------

    // ---------------- Definir el Tráfico de Red -----------------
    
    // Create an application to send UDP packets from a node in cluster A to a node in cluster B
    uint16_t port = 9;
    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(interfacesB.GetAddress(0), port));
    onoff.SetConstantRate(DataRate("500kbps"));
    ApplicationContainer app = onoff.Install(clusterA.Get(0));
    app.Start(Seconds(1.0));
    app.Stop(Seconds(19.0));

    // Packet sink to receive these packets on the destination node
    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    app = sink.Install(clusterB.Get(0));
    app.Start(Seconds(0.0));
    app.Stop(Seconds(20.0));
    // ------------------------------------------------------------


    // ----------- Configurar el Registro de Resultados -----------
    // Se supone que configura un registro para capturar medidas de rendimiento:
    // 
    // Enable PCAP tracing for each node
    wifiPhy.EnablePcap("clusterA", devicesA);
    wifiPhy.EnablePcap("clusterB", devicesB);
    wifiPhy.EnablePcap("clusterC", devicesC);

    // Enable ASCII tracing
    AsciiTraceHelper ascii;
    wifiPhy.EnableAsciiAll(ascii.CreateFileStream("manet-simulation.tr"));
    // ------------------------------------------------------------

    // Set simulation stop time
    Simulator::Stop(Seconds(20.0));

    // Run the simulator
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}