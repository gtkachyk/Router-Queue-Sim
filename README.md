# Router Queue Simulator

This program simulates the behavior of a router by processing a series of packet events, calculating queuing delays, and determining packet loss rates.

## Usage

To compile the program, use g++ or any other C++ compiler:

```bash
g++ router.cpp -o router
```

Once compiled, you can run the program with the following command:

```bash
./router <buffer_length> <wlan_bandwidth> <input_file1> <input_file2> ...
```

## Functionality

- **Packet Event Processing:** Reads packet events from input files, where each event consists of a timestamp and packet size.
- **Buffer Management:** Manages the simulated router's buffer space, handling packet arrivals, departures, and drops based on buffer availability.
- **QoS Metrics Calculation:** Calculates various QoS metrics, including packet loss rate and average queuing delay.


