/******************************************************************************
 * Implementação de cliente TCP para consultar/cadastrar/remover informações de
 * filmes em um servidor.
 * - Compilação:
 *      gcc -o cliente cliente.c
 * - Execução:
 *      ./cliente <IP_do_servidor> <porta desejada>
 * - Exemplo de uso:
 *      ./cliente 192.168.0.20 8000
 ******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define BUFFER_SIZE 1024    // Tamanho em bits do buffer para comunicação


/* Função auxiliar para ler string do usuário */
void readLine(char* buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        // Remove newline, se houver
        char* newlinePos = strchr(buffer, '\n');
        if (newlinePos) {
            *newlinePos = '\0';
        }
    }
}


/* Função principal do cliente */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        // Caso não tenha IP ou porta informada, exibe mensagem de ajuda
        printf("Uso: %s <IP_do_servidor> <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* serverIp = argv[1];
    int port = atoi(argv[2]);

    // Cria socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configura endereço do servidor
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIp, &serverAddr.sin_addr) <= 0) {
        perror("Endereço IP inválido");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Conecta ao servidor
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Erro na conexão");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor %s:%d\n", serverIp, port);

    // Loop do menu
    while (1) {
        printf("\n==============================================\n");
        printf("          MENU DE OPÇÕES\n");
        printf("     SEJA BEM VINDO AO NERDFLIX!\n");
        printf("================================================\n");
        printf("1. Cadastrar um novo filme\n");
        printf("2. Adicionar um novo gênero a um filme\n");
        printf("3. Remover um filme pelo identificador\n");
        printf("4. Listar todos os títulos de filmes com seus identificadores\n");
        printf("5. Listar informações de todos os filmes\n");
        printf("6. Listar informações de um filme específico\n");
        printf("7. Listar todos os filmes de um determinado gênero\n");
        printf("0. Encerrar conexão\n");
        printf("Escolha uma opção: ");

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));
        readLine(buffer, sizeof(buffer));
        int option = atoi(buffer);

        // Envia a opção ao servidor
        send(sock, buffer, strlen(buffer), 0);

        if (option == 0) {
            // Sai do loop
            printf("Encerrando conexão com o servidor...\n");
            break;
        }

        switch (option) {
            case 1: {
                // (1) Cadastrar um novo filme
                char title[100], director[100], genres[200], yearStr[20];

                printf("Digite o título do filme: ");
                readLine(title, sizeof(title));

                printf("Digite o nome do diretor: ");
                readLine(director, sizeof(director));

                printf("Digite o ano de lançamento (YYYY): ");
                readLine(yearStr, sizeof(yearStr));

                printf("Digite os gêneros (separados por ponto-e-vírgula e sem espaço): ");
                readLine(genres, sizeof(genres));

                // Envia título
                send(sock, title, strlen(title), 0);
                usleep(100000);

                // Envia diretor
                send(sock, director, strlen(director), 0);
                usleep(100000);

                // Envia ano
                send(sock, yearStr, strlen(yearStr), 0);
                usleep(100000);

                // Envia gêneros
                send(sock, genres, strlen(genres), 0);

                // Recebe resposta
                memset(buffer, 0, sizeof(buffer));
                int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
                if (bytesRead > 0) {
                    printf("\n--- Resposta do Servidor ---\n%s\n", buffer);
                }
            } break;

            case 2: {
                // (2) Adicionar um novo gênero a um filme
                char idStr[20], genre[100];

                printf("Digite o ID do filme: ");
                readLine(idStr, sizeof(idStr));

                printf("Digite o novo gênero a ser adicionado: ");
                readLine(genre, sizeof(genre));

                // Envia ID
                send(sock, idStr, strlen(idStr), 0);
                usleep(100000);

                // Envia gênero
                send(sock, genre, strlen(genre), 0);

                // Recebe resposta
                memset(buffer, 0, sizeof(buffer));
                int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
                if (bytesRead > 0) {
                    printf("\n--- Resposta do Servidor ---\n%s\n", buffer);
                }
            } break;

            case 3: {
                // (3) Remover um filme pelo identificador
                char idStr[20];

                printf("Digite o ID do filme a remover: ");
                readLine(idStr, sizeof(idStr));

                // Envia ID
                send(sock, idStr, strlen(idStr), 0);

                // Recebe resposta
                memset(buffer, 0, sizeof(buffer));
                int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
                if (bytesRead > 0) {
                    printf("\n--- Resposta do Servidor ---\n%s\n", buffer);
                }
            } break;

            case 4: {
                // (4) Listar todos os títulos de filmes com seus identificadores
                // Recebe resposta
                memset(buffer, 0, sizeof(buffer));
                int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
                if (bytesRead > 0) {
                    printf("\n--- Resposta do Servidor ---\n%s\n", buffer);
                }
            } break;

            case 5: {
                // (5) Listar informações de todos os filmes
                // Recebe resposta
                memset(buffer, 0, sizeof(buffer));
                int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
                if (bytesRead > 0) {
                    printf("\n--- Resposta do Servidor ---\n%s\n", buffer);
                }
            } break;

            case 6: {
                // (6) Listar informações de um filme específico
                char idStr[20];
                printf("Digite o ID do filme: ");
                readLine(idStr, sizeof(idStr));

                // Envia ID
                send(sock, idStr, strlen(idStr), 0);

                // Recebe resposta
                memset(buffer, 0, sizeof(buffer));
                int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
                if (bytesRead > 0) {
                    printf("\n--- Resposta do Servidor ---\n%s\n", buffer);
                }
            } break;

            case 7: {
                // (7) Listar todos os filmes de um determinado gênero
                char genre[100];
                printf("Digite o gênero: ");
                readLine(genre, sizeof(genre));

                // Envia gênero
                send(sock, genre, strlen(genre), 0);

                // Recebe resposta
                memset(buffer, 0, sizeof(buffer));
                int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
                if (bytesRead > 0) {
                    printf("\n--- Resposta do Servidor ---\n%s\n", buffer);
                }
            } break;

            default:
                printf("Opção inválida!\n");
                // Recebe possível resposta
                memset(buffer, 0, sizeof(buffer));
                recv(sock, buffer, sizeof(buffer), 0);
                printf("Resposta do servidor: %s\n", buffer);
                break;
        }

    }

    // Fecha o socket do cliente
    close(sock);

    return 0;
}

