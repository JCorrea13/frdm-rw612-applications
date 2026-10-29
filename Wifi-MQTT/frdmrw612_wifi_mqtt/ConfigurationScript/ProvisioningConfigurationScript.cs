#:package Tmds.MDns@0.10.1

using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using Tmds.MDns;

const int DefaultDiscoveryDurationSeconds = 10;
const string DefaultServiceType = "_services._dns-sd._udp.local";

var (serviceType, discoveryDurationSeconds) = ParseArguments(args);

/**
Command Example:
- dotnet .\ProvisioningConfigurationScript.cs --duration 3  --type _wifi-prov._tcp

Creds Example:
- ssid=HomeWiFi&password=correct-horse-battery-staple
*/

Console.WriteLine("==================================================");
Console.WriteLine(" Provisioning Configuration Script");
Console.WriteLine($" Discovery service type : {serviceType}");
Console.WriteLine($" Discovery duration     : {discoveryDurationSeconds} seconds");
Console.WriteLine("==================================================");

var discoveredServices = new Dictionary<string, DiscoveredService>(StringComparer.Ordinal);

var browser = new ServiceBrowser();

browser.ServiceAdded += (_, eventArgs) =>
{
    var announcement = eventArgs.Announcement;
    if (!IsTcpService(announcement.Type))
    {
        return;
    }

    var address = ResolveAddress(announcement);
    if (address is null)
    {
        return;
    }

    lock (discoveredServices)
    {
        discoveredServices[announcement.Instance] = new DiscoveredService(
            announcement.Instance,
            announcement.Type,
            announcement.Hostname,
            address,
            announcement.Port);
    }

    Console.WriteLine($"[+] Found TCP service: {announcement.Instance} @ {announcement.Hostname}:{announcement.Port}");
};

browser.ServiceRemoved += (_, eventArgs) =>
{
    Console.WriteLine($"[-] Removed service: {eventArgs.Announcement.Instance}");
};

browser.StartBrowse(serviceType);

for (var remaining = discoveryDurationSeconds; remaining > 0; remaining--)
{
    Console.Write($"\rDiscovering TCP servers... {remaining}s remaining");
    await Task.Delay(1000);
}

browser.StopBrowse();

Console.WriteLine();
Console.WriteLine();
Console.WriteLine("==================================================");
Console.WriteLine($" Discovery complete. TCP services found: {discoveredServices.Count}");
Console.WriteLine("==================================================");

if (discoveredServices.Count == 0)
{
    Console.WriteLine("No TCP services were discovered on the network.");
    return;
}

var services = discoveredServices.Values
    .OrderBy(service => service.Instance, StringComparer.OrdinalIgnoreCase)
    .ToList();

for (var index = 0; index < services.Count; index++)
{
    var service = services[index];
    Console.WriteLine($"[{index + 1}] Service Instance: {service.Instance}");
    Console.WriteLine($"    Service Type : {service.Type}");
    Console.WriteLine($"    Hostname     : {service.Hostname}");
    Console.WriteLine($"    Host         : {service.Address}");
    Console.WriteLine($"    Port         : {service.Port}");
    Console.WriteLine(new string('-', 50));
}

var selectedIndex = -1;
while (selectedIndex < 1 || selectedIndex > services.Count)
{
    Console.Write($"Select a service to connect to (1-{services.Count}): ");
    var selectedValue = Console.ReadLine();

    if (!int.TryParse(selectedValue, out selectedIndex))
    {
        Console.WriteLine("Please enter a valid numeric selection.");
        continue;
    }

    if (selectedIndex < 1 || selectedIndex > services.Count)
    {
        Console.WriteLine("The selected index is out of range.");
    }
}

var selectedService = services[selectedIndex - 1];
Console.WriteLine();
Console.WriteLine($"Connecting to {selectedService.Hostname} ({selectedService.Address}:{selectedService.Port})...");

using var client = new TcpClient();
await client.ConnectAsync(selectedService.Address, selectedService.Port);
using var stream = client.GetStream();

Console.WriteLine("Connected. Type the message to send and press Enter.");
Console.Write("Message> ");
var message = Console.ReadLine() ?? string.Empty;

if (string.IsNullOrEmpty(message))
{
    Console.WriteLine("No message was entered. Exiting without sending any data.");
    return;
}

var payload = Encoding.UTF8.GetBytes(message + Environment.NewLine);
await stream.WriteAsync(payload, 0, payload.Length);

Console.WriteLine();
Console.WriteLine($"Sent: {message}");
Console.WriteLine("Closing connection.");

static (string ServiceType, int Duration) ParseArguments(string[] arguments)
{
    var serviceType = DefaultServiceType;
    var duration = DefaultDiscoveryDurationSeconds;

    for (var index = 0; index < arguments.Length; index++)
    {
        var argument = arguments[index];

        if (argument == "--type" && index + 1 < arguments.Length)
        {
            serviceType = arguments[++index];
            continue;
        }

        if (argument == "--duration" && index + 1 < arguments.Length && int.TryParse(arguments[++index], out var parsedDuration))
        {
            duration = parsedDuration;
            continue;
        }

        if (string.IsNullOrWhiteSpace(serviceType) || serviceType == DefaultServiceType)
        {
            serviceType = argument;
        }
    }

    if (duration <= 0)
    {
        duration = DefaultDiscoveryDurationSeconds;
    }

    return (serviceType, duration);
}

static bool IsTcpService(string? serviceType)
{
    return !string.IsNullOrWhiteSpace(serviceType)
        && (serviceType.Contains("._tcp.", StringComparison.OrdinalIgnoreCase)
            || serviceType.EndsWith("._tcp", StringComparison.OrdinalIgnoreCase)
            || serviceType.EndsWith("._tcp.local", StringComparison.OrdinalIgnoreCase));
}

static IPAddress? ResolveAddress(ServiceAnnouncement announcement)
{
    var addresses = announcement.Addresses;
    if (addresses is { Count: > 0 })
    {
        return addresses.FirstOrDefault(address => address.AddressFamily is AddressFamily.InterNetwork or AddressFamily.InterNetworkV6)
            ?? addresses.FirstOrDefault();
    }

    if (string.IsNullOrWhiteSpace(announcement.Hostname))
    {
        return null;
    }

    try
    {
        var resolvedAddresses = Dns.GetHostAddresses(announcement.Hostname);
        return resolvedAddresses.FirstOrDefault(address => address.AddressFamily is AddressFamily.InterNetwork or AddressFamily.InterNetworkV6)
            ?? resolvedAddresses.FirstOrDefault();
    }
    catch (SocketException)
    {
        return null;
    }
    catch (ArgumentException)
    {
        return null;
    }
}

internal readonly record struct DiscoveredService(
    string Instance,
    string Type,
    string Hostname,
    IPAddress Address,
    int Port);