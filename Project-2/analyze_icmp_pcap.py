from scapy.all import rdpcap
import matplotlib.pyplot as plt
import sys

def analyze_pcap(pcap_file: str) -> None:
    # Carrega pacotes
    packets = rdpcap(pcap_file)
    ip_packets = [pkt for pkt in packets if pkt.haslayer("IP")]

    # Métricas básicas
    packet_times   = [float(pkt.time) for pkt in ip_packets]
    total_packets  = len(ip_packets)
    total_bytes    = sum(len(pkt) for pkt in ip_packets)

    start_time = packet_times[0]
    end_time   = packet_times[-1]
    duration   = end_time - start_time if end_time > start_time else 1e-9

    throughput_bps  = (total_bytes * 8) / duration          # bits/s
    throughput_mbps = throughput_bps / 1e6                  # Mbit/s

    inter_arrival = [
        packet_times[i + 1] - packet_times[i]
        for i in range(total_packets - 1)
    ]
    mean_inter = sum(inter_arrival) / len(inter_arrival) if inter_arrival else 0

    flows = {(pkt["IP"].src, pkt["IP"].dst) for pkt in ip_packets}

    print(f"Contagem de pacotes (total de pacotes): {total_packets}")
    print(f"Duração da captura: {duration:.3f} s")
    print(f"Throughput (taxa de transferência) médio: {throughput_mbps:.3f} Mbps")
    print(
        "Intervalo médio entre pacotes (tempo entre chegadas de pacotes): "
        f"{mean_inter * 1000:.3f} ms"
    )
    print("\nEndereços IP de origem e de destino:")
    for src, dst in sorted(flows):
        print(f"{src} -> {dst}")

    # --------- Gráficos ---------
    times = [t - start_time for t in packet_times]
    cumulative_bytes = []
    running_total = 0
    for pkt in ip_packets:
        running_total += len(pkt)
        cumulative_bytes.append(running_total)

    plt.figure()
    plt.plot(times, cumulative_bytes)
    plt.title("Cumulative Bytes Over Time")
    plt.xlabel("Tempo (s)")
    plt.ylabel("Bytes acumulados")
    plt.grid(True)
    plt.tight_layout()
    plt.show()

    plt.figure()
    plt.plot(range(1, len(inter_arrival) + 1), inter_arrival)
    plt.title("Inter-arrival Times Between Packets")
    plt.xlabel("Índice do pacote")
    plt.ylabel("Intervalo (s)")
    plt.grid(True)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Uso: python analyze_icmp_pcap.py <arquivo_pcap>")
        sys.exit(1)
    analyze_pcap(sys.argv[1])
