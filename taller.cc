
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/wifi-module.h"
#include "ns3/wifi-helper.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/animation-interface.h"
#include "ns3/command-line.h"
#include "ns3/csma-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/qos-txop.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"


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
    uint32_t stopTime = 20;

    // Activar el CourseChangeCallback
    bool useCourseChangeCallback = true;

    // Set default values for simulation parameters
    Config::SetDefault("ns3::OnOffApplication::PacketSize", StringValue("1472")); //paquetes en bytes
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("100kb/s")); //tasa de transferencia de datos

    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("stopTime", "simulation stop time (seconds)", stopTime);
    cmd.AddValue("useCourseChangeCallback",
        "whether to enable course change tracing",
        useCourseChangeCallback);
    //
    // The system global variables and the local values added to the argument
    // system can be overridden by command line arguments by using this call.
    //
    cmd.Parse(argc, argv);

    if (stopTime < 10)
    {
        std::cout << "Use a simulation stop time >= 10 seconds" << std::endl;
        exit(1);
    }

    // Create nodes for clusters
    NodeContainer clusterA, clusterB, clusterC;
    clusterA.Create(3);
    clusterB.Create(3);
    clusterC.Create(3);

    NS_LOG_INFO("Cluster A created with ConstantPositionMobilityModel");
    NS_LOG_INFO("Cluster B created with RandomWaypointMobilityModel");
    NS_LOG_INFO("Cluster C created with RandomWalk2dMobilityModel");

    // Set up WiFi and channel
    WifiHelper wifi;
    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate54Mbps"));
    // Capa física WiFi
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    //instala WiFi en cada cluster
    NetDeviceContainer devicesA = wifi.Install(wifiPhy, mac, clusterA);
    NetDeviceContainer devicesB = wifi.Install(wifiPhy, mac, clusterB);
    NetDeviceContainer devicesC = wifi.Install(wifiPhy, mac, clusterC);

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
        "MinX", DoubleValue(50.0),
        "MinY", DoubleValue(50.0),
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
        "MinX", DoubleValue(20.0),
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
        "MinX", DoubleValue(80.0),
        "MinY", DoubleValue(50.0),
        "DeltaX", DoubleValue(5.0),
        "DeltaY", DoubleValue(10.0),
        "GridWidth", UintegerValue(3),
        "LayoutType", StringValue("RowFirst"));
    // movilidad (Random Walk)
    mobilityC.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        "Mode", StringValue("Time"),
        "Time", StringValue("2s"),
        "Speed", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
        "Bounds", RectangleValue(Rectangle(-500, 500, -500, 500)));
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
    
    uint16_t port = 9;
    // Create an application to send UDP packets from a node in cluster A to a node in cluster B
    OnOffHelper onoffToB("ns3::UdpSocketFactory", InetSocketAddress(interfacesB.GetAddress(0), port));
    onoffToB.SetConstantRate(DataRate("500kbps"));
    ApplicationContainer appToB = onoffToB.Install(clusterA.Get(0));
    appToB.Start(Seconds(1.0));
    appToB.Stop(Seconds(19.0));

    // Create an application to send UDP packets from a node in cluster C to a node in cluster B
    OnOffHelper onoffToC("ns3::UdpSocketFactory", InetSocketAddress(interfacesC.GetAddress(0), port));
    onoffToC.SetConstantRate(DataRate("500kbps"));
    ApplicationContainer appToC = onoffToC.Install(clusterA.Get(0));
    appToC.Start(Seconds(1.0));
    appToC.Stop(Seconds(19.0));

    // Packet sinks to receive these packets on the destination nodes
    //sink for B
    PacketSinkHelper sinkToB("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkAppToB = sinkToB.Install(clusterB.Get(0));
    sinkAppToB.Start(Seconds(0.0));
    sinkAppToB.Stop(Seconds(20.0));
    //sink for C
    PacketSinkHelper sinkToC("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkAppToC = sinkToC.Install(clusterC.Get(0));
    sinkAppToC.Start(Seconds(0.0));
    sinkAppToC.Stop(Seconds(20.0));
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

    NS_LOG_INFO("Run Simulation.");

    // Set simulation stop time
    Simulator::Stop(Seconds(stopTime));

    // configurar la interfaz de animación
    AnimationInterface anim("manet-simulation.xml");

    // Habilitar la traza de movimientos de nodos
    anim.EnablePacketMetadata(true);

    // Run the simulator
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}