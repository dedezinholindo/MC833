/******************************************************************************
 * Implementação de servidor TCP concorrente para gerenciamento de dados de
 * filmes.
 * - Usa threads para lidar com múltiplos clientes simultâneos.
 * - Armazena dados em um arquivo CSV.
 * - Operações:
 *      - cadastrar um novo filme;
 *      - adicionar um novo genêro a um filme;
 *      - remover um filme;
 *      - listar todos títulos dos filmes;
 *      - listar todas informações dos filmes;
 *      - listar informações de um filme;
 *      - listar todos filmes de um gênero.
 * - Compilação:
 *      gcc -o server servidor.c -lpthread
 * - Execução:
 *      ./servidor <porta desejada>
 * - Exemplo de uso:
 *     ./servidor 8000
 ******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>


#define MAX_MOVIES 1000             // Máximo de filmes no sistema
#define BUFFER_SIZE 1024            // Tamanho em bits do buffer para comunicação
#define CSV_FILE_NAME "movies.csv"  // Nome do arquivo CSV para armazenar filmes


/* Estrutura para armazenar informações de filme */
typedef struct {
    int id;             // ID (identificador único)
    char title[100];    // Título
    char director[100]; // Nome do diretor
    int year;           // Ano de lançamento
    char genres[200];   // Gêneros separados por ponto e vírgula, ex: "ação;aventura"
} Movie;


/* Variáveis globais */
Movie movieList[MAX_MOVIES];   // Array estático para filmes
int movieCount = 0;            // Quantidade de filmes carregados

pthread_mutex_t movieMutex;    // Mutex para proteger acesso à movieList


/* Funções auxiliares internas */
/* Carregar filmes do arquivo CSV para o array */
void loadMoviesFromCSV(const char* filename) {
    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        // Se não encontra o arquivo, inicializa com zero filmes
        printf("Arquivo '%s' não encontrado. Inicializando sem filmes registrados.\n", filename);
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        // Substitue newline do final por null, se existir
        char* newlinePos = strchr(line, '\n');
        if (newlinePos) {
            *newlinePos = '\0';
        }

        // Dividir linha em tokens (CSV): id, titulo, diretor, ano, generos
        // ID
        char* token = strtok(line, ",");
        if (!token) continue;
        int id = atoi(token);

        // Título
        token = strtok(NULL, ",");
        if (!token) continue;
        char title[100];
        strcpy(title, token);

        // Nome do diretor
        token = strtok(NULL, ",");
        if (!token) continue;
        char director[100];
        strcpy(director, token);

        // Ano de lançamento
        token = strtok(NULL, ",");
        if (!token) continue;
        int year = atoi(token);

        // Gêneros
        token = strtok(NULL, ",");
        if (!token) continue;
        char genres[200];
        strcpy(genres, token);

        // Adicionar ao array de filmes
        movieList[movieCount].id = id;
        strcpy(movieList[movieCount].title, title);
        strcpy(movieList[movieCount].director, director);
        movieList[movieCount].year = year;
        strcpy(movieList[movieCount].genres, genres);
        movieCount++;

        if (movieCount >= MAX_MOVIES) {
            printf("Limite máximo de filmes atingido!\n");
            break;
        }
    }

    fclose(file);
    printf("Carregados %d filmes do arquivo '%s'.\n", movieCount, filename);
}

/* Salvar todos os filmes do array no arquivo CSV */
void saveMoviesToCSV(const char* filename) {
    FILE* file = fopen(filename, "w");

    if (file == NULL) {
        // Se não consegue abrir o arquivo, não salva nada
        printf("Erro ao abrir arquivo '%s' para escrita.\n", filename);
        return;
    }

    // Salva as informações de cada filme no formato CSV
    for (int i = 0; i < movieCount; i++) {
        fprintf(file, "%d,%s,%s,%d,%s\n",
                movieList[i].id,
                movieList[i].title,
                movieList[i].director,
                movieList[i].year,
                movieList[i].genres);
    }

    fclose(file);
}

/* Gerar um novo ID para um filme */
int generateNewId() {
    // Gera um novo ID somando 1 ao maior ID existente
    int maxId = 0;
    for (int i = 0; i < movieCount; i++) {
        if (movieList[i].id > maxId) {
            maxId = movieList[i].id;
        }
    }
    return maxId + 1;
}

/* Buscar índice de filme no array pelo ID (retorna -1 se não encontrar) */
int findMovieIndexById(int id) {
    for (int i = 0; i < movieCount; i++) {
        if (movieList[i].id == id) {
            return i;
        }
    }
    return -1;
}


/* Funções para operações de usuário */
/* (1) Cadastrar um novo filme */
void registerMovie(
    const char* title,
    const char* director,
    int year,
    const char* genres,
    char* response
) {
    if (movieCount >= MAX_MOVIES) {
        sprintf(response, "Erro: Limite de filmes atingido!\n");
        return;
    }

    // Gera ID para o filme
    int newId = generateNewId();

    // Adiciona o filme ao array estático
    movieList[movieCount].id = newId;
    strcpy(movieList[movieCount].title, title);
    strcpy(movieList[movieCount].director, director);
    movieList[movieCount].year = year;
    strcpy(movieList[movieCount].genres, genres);

    movieCount++;

    // Salva os dados atualizados no arquivo CSV
    saveMoviesToCSV(CSV_FILE_NAME);

    sprintf(response, "Filme cadastrado com sucesso! ID: %d\n", newId);
}

/* (2) Adicionar um novo gênero a um filme */
void addGenreToMovie(int id, const char* newGenre, char* response) {
    // Recupera o index do filme no array
    int index = findMovieIndexById(id);

    if (index == -1) {
        // Se não encontrar o filme no array, retorna erro
        sprintf(response, "Erro: Filme com ID %d não encontrado.\n", id);
        return;
    }

    // Adiciona o novo gênero ao filme
    if (strlen(movieList[index].genres) > 0) {
        // Se já tem algum gênero, adiciona ponto e vírgula antes
        strcat(movieList[index].genres, ";");
    } 
    strcpy(movieList[index].genres, newGenre);

    // Salva os dados atualizados no arquivo CSV
    saveMoviesToCSV(CSV_FILE_NAME);

    sprintf(response, "Gênero '%s' adicionado ao filme ID %d.\n", newGenre, id);
}

/* (3) Remover um filme pelo identificador */
void removeMovie(int id, char* response) {
    // Recupera o index do filme no array
    int index = findMovieIndexById(id);

    if (index == -1) {
        // Se não encontrar o filme no array, retorna erro
        sprintf(response, "Erro: Filme com ID %d não existe.\n", id);
        return;
    }

    // "Remove" o filme do array copiando o último filme do array para a posição
    // do filme removido e decrementando o contador de filmes do array
    movieList[index] = movieList[movieCount - 1];
    movieCount--;

    // Salva os dados atualizados no arquivo CSV
    saveMoviesToCSV(CSV_FILE_NAME);

    sprintf(response, "Filme com ID %d removido com sucesso.\n", id);
}

/* (4) Listar todos os títulos de filmes com seus identificadores */
void listAllMoviesIds(char* response) {
    if (movieCount == 0) {
        // Se não há filmes cadastrados, retorna mensagem apropriada
        strcat(response, "Nenhum filme cadastrado.\n");
        return;
    }

    // Prepara a resposta com os títulos e IDs dos filmes
    strcpy(response, "Lista de Filmes (ID - Título):\n");
    char temp[256];
    for (int i = 0; i < movieCount; i++) {
        sprintf(temp, "%d - %s\n", movieList[i].id, movieList[i].title);
        strcat(response, temp);
    }
}

/* (5) Listar informações de todos os filmes */
void listAllMoviesInfo(char* response) {
    if (movieCount == 0) {
        // Se não há filmes cadastrados, retorna mensagem apropriada
        strcat(response, "Nenhum filme cadastrado.\n");
        return;
    }

    char temp[512];
    strcpy(response, "Informações de Todos os Filmes:\n");
    for (int i = 0; i < movieCount; i++) {
        sprintf(temp, "ID: %d | Título: %s | Diretor: %s | Ano: %d | Gêneros: %s\n",
                movieList[i].id,
                movieList[i].title,
                movieList[i].director,
                movieList[i].year,
                movieList[i].genres);
        strcat(response, temp);
    }
}

/* (6) Listar informações de um filme específico */
void listMovieById(int id, char* response) {
    // Recupera o index do filme no array
    int index = findMovieIndexById(id);

    if (index == -1) {
        // Se não encontrar o filme no array, retorna erro
        sprintf(response, "Erro: Filme com ID %d não encontrado.\n", id);
        return;
    }

    // Prepara a resposta com as informações do filme
    sprintf(response, "Informações do Filme (ID %d):\nTítulo: %s\nDiretor: %s\nAno: %d\nGêneros: %s\n",
            movieList[index].id,
            movieList[index].title,
            movieList[index].director,
            movieList[index].year,
            movieList[index].genres);
}

/* (7) Listar todos os filmes de um determinado gênero */
void listMoviesByGenre(const char* genre, char* response) {
    if (movieCount == 0) {
        // Se não há filmes cadastrados, retorna mensagem apropriada
        strcat(response, "Nenhum filme cadastrado.\n");
        return;
    }

    char temp[512];
    int foundCount = 0;

    // Prepara a resposta com os filmes do gênero solicitado
    strcpy(response, "Filmes do gênero buscado:\n");
    for (int i = 0; i < movieCount; i++) {
        // Verifica se o gênero está presente em movieList[i].genres
        if (strstr(movieList[i].genres, genre) != NULL) {
            // Se o gênero for encontrado, adiciona as informações do filme à
            // resposta
            sprintf(temp, "ID: %d | Título: %s | Diretor: %s | Ano: %d | Gêneros: %s\n",
                    movieList[i].id,
                    movieList[i].title,
                    movieList[i].director,
                    movieList[i].year,
                    movieList[i].genres);
            strcat(response, temp);
            foundCount++;
        }
    }

    if (foundCount == 0) {
        // Se nenhum filme do gênero for encontrado, adiciona mensagem
        // apropriada
        strcat(response, "Nenhum filme encontrado para esse gênero.\n");
    }
}


/* Função de tratamento de cliente */
/* Trata cada cliente em uma thread */
void* handleClient(void* arg) {
    int clientSocket = *((int*)arg);
    free(arg); // Liberar memória alocada para o socket do cliente

    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE * 4]; // para respostas mais extensas

    while (1) {
        // Zera buffers
        memset(buffer, 0, sizeof(buffer));
        memset(response, 0, sizeof(response));

        // Lê a opção do cliente (0 a 7)
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            // Cliente desconectou ou ocorreu erro
            printf("Cliente desconectado.\n");
            break;
        }
        int option = atoi(buffer);

        // (0) Encerrar conexão
        if (option == 0) {
            // Cliente deseja encerrar
            printf("Cliente solicitou encerrar conexão.\n");
            break;
        }

        // Tratamento das demais opções
        switch (option) {
            case 1: {
                // (1) Cadastrar um novo filme
                // Recebe título
                char title[100];
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                strcpy(title, buffer);

                // Recebe diretor
                char director[100];
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                strcpy(director, buffer);

                // Recebe ano
                int year;
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                year = atoi(buffer);

                // Recebe gêneros
                char genres[200];
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                strcpy(genres, buffer);

                // Registra o filme protegendo com mutex
                pthread_mutex_lock(&movieMutex);
                registerMovie(title, director, year, genres, response);
                pthread_mutex_unlock(&movieMutex);

                // Envia resposta ao cliente
                send(clientSocket, response, strlen(response), 0);
            } break;

            case 2: {
                // (2) Adicionar um novo gênero a um filme
                // Recebe ID
                int id;
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                id = atoi(buffer);

                // Recebe novo gênero
                char newGenre[100];
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                strcpy(newGenre, buffer);

                // Adiciona gênero ao filme protegendo com mutex
                pthread_mutex_lock(&movieMutex);
                addGenreToMovie(id, newGenre, response);
                pthread_mutex_unlock(&movieMutex);

                // Envia resposta ao cliente
                send(clientSocket, response, strlen(response), 0);
            } break;

            case 3: {
                // (3) Remover um filme pelo identificador
                // Recebe ID
                int id;
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                id = atoi(buffer);

                // Remove filme do array protegendo com mutex
                pthread_mutex_lock(&movieMutex);
                removeMovie(id, response);
                pthread_mutex_unlock(&movieMutex);

                // Envia resposta ao cliente
                send(clientSocket, response, strlen(response), 0);
            } break;

            case 4: {
                // (4) Listar todos os títulos de filmes com seus
                // identificadores
                // Lista os títulos e identificadores de todos os filmes
                // protegendo com mutex
                pthread_mutex_lock(&movieMutex);
                listAllMoviesIds(response);
                pthread_mutex_unlock(&movieMutex);

                // Envia resposta ao cliente
                send(clientSocket, response, strlen(response), 0);
            } break;

            case 5: {
                // (5) Listar informações de todos os filmes
                // Lista as informações de todos os filmes protegendo com mutex
                pthread_mutex_lock(&movieMutex);
                listAllMoviesInfo(response);
                pthread_mutex_unlock(&movieMutex);

                // Envia resposta ao cliente
                send(clientSocket, response, strlen(response), 0);
            } break;

            case 6: {
                // (6) Listar informações de um filme específico
                // Recebe ID
                int id;
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                id = atoi(buffer);

                // Lista as informações do filme protegendo com mutex
                pthread_mutex_lock(&movieMutex);
                listMovieById(id, response);
                pthread_mutex_unlock(&movieMutex);

                // Envia resposta ao cliente
                send(clientSocket, response, strlen(response), 0);
            } break;

            case 7: {
                // (7) Listar todos os filmes de um determinado gênero
                // Recebe gênero
                char genre[100];
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                strcpy(genre, buffer);

                // Lista os filmes do gênero protegendo com mutex
                pthread_mutex_lock(&movieMutex);
                listMoviesByGenre(genre, response);
                pthread_mutex_unlock(&movieMutex);

                // Envia resposta ao cliente
                send(clientSocket, response, strlen(response), 0);
            } break;

            default:
                // Opção inválida
                // Envia mensagem de erro ao cliente
                sprintf(response, "Opção inválida.\n");
                send(clientSocket, response, strlen(response), 0);
                break;
        }
    }

    // Fecha o socket do cliente
    close(clientSocket);
    pthread_exit(NULL);
}


/* Função principal do servidor */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        // Caso não tenha porta informada, exibe mensagem de ajuda
        printf("Uso: %s <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize;

    // Inicializa mutex
    pthread_mutex_init(&movieMutex, NULL);

    // Carrega filmes do arquivo CSV
    loadMoviesFromCSV(CSV_FILE_NAME);

    // Cria socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configura endereço do servidor
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    // Faz bind
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Erro no bind");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Escuta
    if (listen(serverSocket, 5) < 0) {
        perror("Erro no listen");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Servidor iniciado na porta %d. Aguardando conexões...\n", port);

    // Loop para aceitar conexões
    while (1) {
        addrSize = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrSize);
        if (clientSocket < 0) {
            perror("Erro no accept");
            continue;
        }

        printf("Cliente conectado.\n");

        // Cria thread para atender o cliente
        pthread_t threadId;
        int* newSocket = malloc(sizeof(int));
        *newSocket = clientSocket;

        if (pthread_create(&threadId, NULL, handleClient, (void*)newSocket) != 0) {
            perror("Erro ao criar thread");
        }

        pthread_detach(threadId);
    }

    // Fecha o socket do servidor
    close(serverSocket);
    // Destrói o mutex
    pthread_mutex_destroy(&movieMutex);

    return 0;
}

