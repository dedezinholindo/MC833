"""
analyze_icmp_pcap.py


Script para analisar um arquivo PCAP contendo tráfego ICMP.
Suas funcionalidades são:


- Carregar pacotes (Scapy)
- Contar pacotes e bytes
- Calcular throughput médio
- Calcular intervalo médio entre pacotes
- Listar endereços IP origem/destino
- Gerar dois gráficos (Matplotlib)


Uso:
	python analyze_icmp_pcap.py captura_icmp.pcap
"""


# Importações
from scapy.all import rdpcap
import matplotlib.pyplot as plt
import sys


# Função principal
def analyze_pcap(pcap_file: str) -> None:
    """
	Lê o PCAP, extrai métricas e gera gráficos.
	"""

    # Carregamento dos pacotes do arquivo
    packets = rdpcap(pcap_file)

    # Filtro: mantemos apenas os pacotes que possuem camada IP
    ip_packets = [pkt for pkt in packets if pkt.haslayer("IP")]

    # Métricas básicas
    # Lista de timestamps
    packet_times   = [float(pkt.time) for pkt in ip_packets]
    # Contagem total de pacotes
    total_packets  = len(ip_packets)
    # Contagem total de bytes
    total_bytes    = sum(len(pkt) for pkt in ip_packets)
    # Duração da captura
    start_time = packet_times[0]
    end_time   = packet_times[-1]
    duration   = end_time - start_time if end_time > start_time else 1e-9

    # Throughput médio em Bps e kBps
    throughput_bps  = (total_bytes * 8) / duration          # Bps
    throughput_kbps = throughput_bps / 1e3                  # kBps

    # Lista de intervalos de tempo entre chegadas de pacotes consecutivos
    inter_arrival = [
        packet_times[i + 1] - packet_times[i]
        for i in range(total_packets - 1)
    ]
    # Cálculo do intervalo médio entre pacotes
    mean_inter = sum(inter_arrival) / len(inter_arrival) if inter_arrival else 0

    # Conjunto (único) de fluxos IP (origem -> destino)
    flows = {(pkt["IP"].src, pkt["IP"].dst) for pkt in ip_packets}

    # Impressão das métricas
    print(f"Contagem de pacotes (total)             : {total_packets}")
    print(f"Contagem de bytes (total)               : {total_bytes} B")
    print(f"Duração da captura                      : {duration:.3f} s")
    print(f"Throughput médio                        : {throughput_kbps:.3f} kBps")
    print(f"Intervalo de tempo médio entre pacotes  : {mean_inter * 1000:.3f} ms")
    print("\nEndereços IP: origem -> destino:")
    for src, dst in sorted(flows):
        print(f"    - {src} -> {dst}")

    # Construção dos gráficos
    # Eixo-x normalizado (tempo em relativo ao início da captura)
    times = [t - start_time for t in packet_times]

    # Gráfico 1: Bytes acumulados ao longo do tempo
    cumulative_bytes = []
    running_total = 0
    for pkt in ip_packets:
        running_total += len(pkt)
        cumulative_bytes.append(running_total)

    plt.figure(figsize=(10, 4))
    plt.subplot(1, 2, 1)
    plt.plot(times, cumulative_bytes)
    plt.title("Bytes Acumulados ao Longo do Tempo")
    plt.xlabel("Tempo (s)")
    plt.ylabel("Bytes")
    plt.grid(True)

    # Gráfico 2: Intervalos de tempo entre pacotes
    plt.subplot(1, 2, 2)
    plt.plot(range(1, len(inter_arrival) + 1), inter_arrival)
    plt.title("Intervalos de Tempo entre Chegada dos Pacotes")
    plt.xlabel("Índice do pacote")
    plt.ylabel("Intervalo (s)")
    plt.grid(True)
    
    plt.tight_layout()
    plt.show()

# Execução do script
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Uso: python analyze_icmp_pcap.py <arquivo_pcap>")
        sys.exit(1)

    analyze_pcap(sys.argv[1])
