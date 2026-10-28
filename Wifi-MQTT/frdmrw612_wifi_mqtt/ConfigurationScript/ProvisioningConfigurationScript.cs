#:package Tmds.MDns@0.10.1

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Tmds.MDns;

// ------------------------------------------------------------------
// Configuration
// ------------------------------------------------------------------
// Time to discover services in seconds
const int DiscoveryDurationSeconds = 10;

// The DNS-SD service type to look for.
// Examples:
//   "_services._dns-sd._udp.local" -> Discover all service types on the network
//   "_http._tcp.local"             -> Web servers
//   "_workstation._tcp.local"       -> Local machines
string serviceType = "_services._dns-sd._udp.local";

// Allow passing custom service type via command line arguments
if (args.Length > 0)
{
    serviceType = args[0];
}

Console.WriteLine("==================================================");
Console.WriteLine($" Starting mDNS / DNS-SD Service Discovery");
Console.WriteLine($" Target Service Type: {serviceType}");
Console.WriteLine($" Listening Duration : {DiscoveryDurationSeconds} seconds");
Console.WriteLine("==================================================\n");

// Collection to store discovered services across multiple threads safely
var discoveredServices = new ConcurrentDictionary<string, ServiceAnnouncement>();

// Instantiate the mDNS ServiceBrowser
using var browser = new ServiceBrowser();

// Event handler: Triggered when a new service is found or updated
browser.ServiceAdded += (sender, e) =>
{
    // Key by unique service name or instance name
    discoveredServices[e.Announcement.Instance] = e.Announcement;
    Console.WriteLine($"[+] Found service: {e.Announcement.Instance}");
};

// Event handler: Triggered when a service leaves or is removed
browser.ServiceRemoved += (sender, e) =>
{
    discoveredServices.TryRemove(e.Announcement.Instance, out _);
    Console.WriteLine($"[-] Removed service: {e.Announcement.Instance}");
};

// Start browsing for the given service type
browser.StartBrowse(serviceType);

// Wait for the configured discovery period while showing a progress countdown
for (int i = DiscoveryDurationSeconds; i > 0; i--)
{
    Console.Write($"\rDiscovering... {i}s remaining ");
    await Task.Delay(1000);
}

// Stop browsing
browser.StopBrowse();

Console.WriteLine("\n\n==================================================");
Console.WriteLine($" Discovery Finished. Total Found: {discoveredServices.Count}");
Console.WriteLine("==================================================\n");

if (discoveredServices.IsEmpty)
{
    Console.WriteLine("No services were discovered on the network.");
    return;
}

// Display summary list of all services found
int index = 1;
foreach (var kvp in discoveredServices)
{
    ServiceAnnouncement service = kvp.Value;
    
    Console.WriteLine($"[{index++}] Service Instance: {service.Instance}");
    Console.WriteLine($"    Service Type : {service.Type}");
    Console.WriteLine($"    Hostname     : {service.Hostname}");
    
    // Display IP addresses (IPv4 & IPv6)
    var ips = service.Addresses != null 
        ? string.Join(", ", service.Addresses) 
        : "N/A";
    Console.WriteLine($"    IP Address(es): {ips}");
    Console.WriteLine($"    Port         : {service.Port}");

    // Display TXT records (metadata) if any exist
    if (service.Txt != null && service.Txt.Any())
    {
        Console.WriteLine("    TXT Records  :");
        foreach (var txtEntry in service.Txt)
        {
            Console.WriteLine($"      - {txtEntry}");
        }
    }
    
    Console.WriteLine(new string('-', 50));
}