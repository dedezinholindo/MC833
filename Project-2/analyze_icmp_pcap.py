import sys
from scapy.config import conf
conf.ipv6_enabled = False
from scapy.all import rdpcap
import matplotlib.pyplot as plt

def analyze_pcap(pcap_file: str):
    packets = rdpcap(pcap_file)
    ip_packets = [pkt for pkt in packets if pkt.haslayer("IP")]

    packet_times = [float(pkt.time) for pkt in ip_packets]
    total_packets = len(ip_packets)
    total_bytes = sum(len(pkt) for pkt in ip_packets)

    start_time = packet_times[0]
    end_time = packet_times[-1]
    duration = end_time - start_time if end_time > start_time else 1e-9

    throughput_bps = (total_bytes * 8) / duration
    throughput_mbps = throughput_bps / 1e6

    inter_arrival = [packet_times[i+1] - packet_times[i] for i in range(total_packets-1)]
    mean_inter = sum(inter_arrival) / len(inter_arrival) if inter_arrival else 0

    flows = {(pkt["IP"].src, pkt["IP"].dst) for pkt in ip_packets}

    print(f"Total packets: {total_packets}")
    print(f"Duration: {duration:.6f} seconds")
    print(f"Average throughput: {throughput_mbps:.3f} Mbps")
    print(f"Average inter-arrival: {mean_inter*1000:.3f} ms")
    print("\nUnique IP flows (src -> dst):")
    for src, dst in flows:
        print(f"{src} -> {dst}")

    times = [t - start_time for t in packet_times]
    cumulative_bytes = []
    running_total = 0
    for pkt in ip_packets:
        running_total += len(pkt)
        cumulative_bytes.append(running_total)

    plt.figure()
    plt.plot(times, cumulative_bytes)
    plt.title("Cumulative Bytes Over Time")
    plt.xlabel("Time (s)")
    plt.ylabel("Bytes")
    plt.grid(True)
    plt.tight_layout()
    plt.show()

    plt.figure()
    plt.plot(range(1, len(inter_arrival)+1), inter_arrival)
    plt.title("Inter-arrival Times Between Packets")
    plt.xlabel("Packet Index")
    plt.ylabel("Inter-arrival (s)")
    plt.grid(True)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Uso: python analyze_icmp_pcap.py <pcap_file>")
    else:
        analyze_pcap(sys.argv[1])
