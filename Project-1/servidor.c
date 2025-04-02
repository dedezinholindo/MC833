/******************************************************************************
 * servidor.c
 *
 * Servidor TCP concorrente para gerenciamento de filmes.
 * Usa threads para lidar com múltiplos clientes simultâneos.
 * Armazena dados em arquivo CSV (movies.csv).
 *
 * Compilação:
 *   gcc -o server servidor.c -lpthread
 *
 * Execução:
 *   ./servidor <porta>
 *
 * Exemplo de uso:
 *   ./servidor 7777
 *
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

#define MAX_MOVIES 1000        // Número máximo de filmes no sistema
#define BUFFER_SIZE 1024       // Tamanho do buffer para comunicação

/* Estrutura para armazenar informações de filme */
typedef struct {
    int id;
    char title[100];
    char director[100];
    int year;
    char genres[200];  // Gêneros separados por ponto e vírgula, por exemplo: "Ação;Aventura"
} Movie;

/* Variáveis globais */
Movie movieList[MAX_MOVIES];   // Array estático para filmes
int movieCount = 0;            // Quantidade de filmes carregados
pthread_mutex_t movieMutex;    // Mutex para proteger acesso à movieList


/* Função para carregar filmes do arquivo CSV (movies.csv) */
void loadMoviesFromCSV(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        // Se não existe, começamos sem filmes
        printf("Arquivo '%s' não encontrado. Iniciando sem filmes.\n", filename);
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline do final, se existir
        char* newlinePos = strchr(line, '\n');
        if (newlinePos) {
            *newlinePos = '\0';
        }

        // Quebrar linha em tokens (CSV): id, titulo, diretor, ano, generos
        char* token = strtok(line, ",");
        if (!token) continue;
        int id = atoi(token);

        token = strtok(NULL, ",");
        if (!token) continue;
        char title[100];
        strcpy(title, token);

        token = strtok(NULL, ",");
        if (!token) continue;
        char director[100];
        strcpy(director, token);

        token = strtok(NULL, ",");
        if (!token) continue;
        int year = atoi(token);

        token = strtok(NULL, ",");
        if (!token) continue;
        char genres[200];
        strcpy(genres, token);

        // Adicionar ao array
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

/* Função para salvar todos os filmes no arquivo CSV */
void saveMoviesToCSV(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Erro ao abrir arquivo '%s' para escrita.\n", filename);
        return;
    }

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

/* Gera um novo ID para um filme (maior ID atual + 1) */
int generateNewId() {
    int maxId = 0;
    for (int i = 0; i < movieCount; i++) {
        if (movieList[i].id > maxId) {
            maxId = movieList[i].id;
        }
    }
    return maxId + 1;
}

/* Busca índice de filme pelo ID (retorna -1 se não encontrar) */
int findMovieIndexById(int id) {
    for (int i = 0; i < movieCount; i++) {
        if (movieList[i].id == id) {
            return i;
        }
    }
    return -1;
}


/* (1) Cadastrar um novo filme */
void registerMovie(const char* title, const char* director, int year, const char* genres, char* response) {
    if (movieCount >= MAX_MOVIES) {
        sprintf(response, "Erro: Limite de filmes atingido!\n");
        return;
    }

    int newId = generateNewId();

    // Preenche dados no array
    movieList[movieCount].id = newId;
    strcpy(movieList[movieCount].title, title);
    strcpy(movieList[movieCount].director, director);
    movieList[movieCount].year = year;
    strcpy(movieList[movieCount].genres, genres);

    movieCount++;

    // Salva em disco
    saveMoviesToCSV("movies.csv");

    sprintf(response, "Filme cadastrado com sucesso! ID: %d\n", newId);
}

/* (2) Adicionar um novo gênero a um filme */
void addGenreToMovie(int id, const char* newGenre, char* response) {
    int index = findMovieIndexById(id);
    if (index == -1) {
        sprintf(response, "Erro: Filme com ID %d não encontrado.\n", id);
        return;
    }

    // Verifica se já existe algum gênero
    if (strlen(movieList[index].genres) > 0) {
        // Adiciona ponto e vírgula e depois o novo gênero
        strcat(movieList[index].genres, ";");
        strcat(movieList[index].genres, newGenre);
    } else {
        // Se por acaso não houver nada, só copia diretamente
        strcpy(movieList[index].genres, newGenre);
    }

    // Salva em disco
    saveMoviesToCSV("movies.csv");

    sprintf(response, "Gênero '%s' adicionado ao filme ID %d.\n", newGenre, id);
}

/* (3) Remover um filme pelo identificador */
void removeMovie(int id, char* response) {
    int index = findMovieIndexById(id);
    if (index == -1) {
        sprintf(response, "Erro: Filme com ID %d não existe.\n", id);
        return;
    }

    // "Remove" copiando o último filme do array para a posição index
    movieList[index] = movieList[movieCount - 1];
    movieCount--;

    // Salva em disco
    saveMoviesToCSV("movies.csv");

    sprintf(response, "Filme com ID %d removido com sucesso.\n", id);
}

/* (4) Listar todos os títulos de filmes com seus identificadores */
void listAllMoviesIds(char* response) {
    char temp[256];
    strcpy(response, "Lista de Filmes (ID - Título):\n");

    for (int i = 0; i < movieCount; i++) {
        sprintf(temp, "%d - %s\n", movieList[i].id, movieList[i].title);
        strcat(response, temp);
    }
}

/* (5) Listar informações de todos os filmes */
void listAllMoviesInfo(char* response) {
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
    int index = findMovieIndexById(id);
    if (index == -1) {
        sprintf(response, "Erro: Filme com ID %d não encontrado.\n", id);
        return;
    }

    sprintf(response, "Informações do Filme (ID %d):\nTítulo: %s\nDiretor: %s\nAno: %d\nGêneros: %s\n",
            movieList[index].id,
            movieList[index].title,
            movieList[index].director,
            movieList[index].year,
            movieList[index].genres);
}

/* (7) Listar todos os filmes de um determinado gênero */
void listMoviesByGenre(const char* genre, char* response) {
    char temp[512];
    int foundCount = 0;

    strcpy(response, "Filmes do gênero buscado:\n");
    for (int i = 0; i < movieCount; i++) {
        // Verifica se o gênero está presente em movieList[i].genres
        if (strstr(movieList[i].genres, genre) != NULL) {
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
        strcat(response, "Nenhum filme encontrado para esse gênero.\n");
    }
}

/* Função que trata cada cliente em uma thread */
void* handleClient(void* arg) {
    int clientSocket = *((int*)arg);
    free(arg); // Liberar memória alocada para o socket do cliente

    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE * 4]; // para respostas mais extensas

    while (1) {
        // Zera buffers
        memset(buffer, 0, sizeof(buffer));
        memset(response, 0, sizeof(response));

        // Lê a opção do cliente (1 a 7) ou 0 se deseja encerrar
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            // Cliente desconectou ou ocorreu erro
            printf("Cliente desconectado.\n");
            break;
        }

        int option = atoi(buffer);
        if (option == 0) {
            // Cliente deseja encerrar
            printf("Cliente solicitou encerrar conexão.\n");
            break;
        }

        // Vamos usar um switch para tratar a opção
        switch (option) {
            case 1: {
                // (1) Cadastrar um novo filme
                // Precisamos ler: title, director, year, genres
                char title[100], director[100], genres[200];
                int year;

                // Recebe título
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                strcpy(title, buffer);

                // Recebe diretor
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                strcpy(director, buffer);

                // Recebe ano
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                year = atoi(buffer);

                // Recebe gêneros
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                strcpy(genres, buffer);

                // Protege com mutex
                pthread_mutex_lock(&movieMutex);
                registerMovie(title, director, year, genres, response);
                pthread_mutex_unlock(&movieMutex);

                send(clientSocket, response, strlen(response), 0);
            } break;

            case 2: {
                // (2) Adicionar gênero
                // Precisamos do ID e do gênero a ser adicionado
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                int id = atoi(buffer);

                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                char newGenre[100];
                strcpy(newGenre, buffer);

                pthread_mutex_lock(&movieMutex);
                addGenreToMovie(id, newGenre, response);
                pthread_mutex_unlock(&movieMutex);

                send(clientSocket, response, strlen(response), 0);
            } break;

            case 3: {
                // (3) Remover filme
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                int id = atoi(buffer);

                pthread_mutex_lock(&movieMutex);
                removeMovie(id, response);
                pthread_mutex_unlock(&movieMutex);

                send(clientSocket, response, strlen(response), 0);
            } break;

            case 4: {
                // (4) Listar títulos + ID
                pthread_mutex_lock(&movieMutex);
                listAllMoviesIds(response);
                pthread_mutex_unlock(&movieMutex);

                send(clientSocket, response, strlen(response), 0);
            } break;

            case 5: {
                // (5) Listar info de todos
                pthread_mutex_lock(&movieMutex);
                listAllMoviesInfo(response);
                pthread_mutex_unlock(&movieMutex);

                send(clientSocket, response, strlen(response), 0);
            } break;

            case 6: {
                // (6) Listar info de um ID
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                int id = atoi(buffer);

                pthread_mutex_lock(&movieMutex);
                listMovieById(id, response);
                pthread_mutex_unlock(&movieMutex);

                send(clientSocket, response, strlen(response), 0);
            } break;

            case 7: {
                // (7) Listar filmes de um gênero
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                char genre[100];
                strcpy(genre, buffer);

                pthread_mutex_lock(&movieMutex);
                listMoviesByGenre(genre, response);
                pthread_mutex_unlock(&movieMutex);

                send(clientSocket, response, strlen(response), 0);
            } break;

            default:
                sprintf(response, "Opção inválida.\n");
                send(clientSocket, response, strlen(response), 0);
                break;
        }
    }

    close(clientSocket);
    pthread_exit(NULL);
}

/* Função principal do servidor */
int main(int argc, char* argv[]) {
    if (argc < 2) {
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
    loadMoviesFromCSV("movies.csv");

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

    // Fechamos o socket do servidor
    close(serverSocket);
    // Destruir mutex
    pthread_mutex_destroy(&movieMutex);

    return 0;
}

