#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "ftp_lib.h"

#define BUF_SIZE 1024

/**
 * @brief Handles the communication with a connected client.
 * @param new_socket The socket descriptor for the connected client.
 * @param ft_root_directory The root directory for the FTP server.
 * @param session_id The session ID for the client connection.
 */
void handle_client(int new_socket, char *ft_root_directory, int session_id);

/**
 * @brief Prints the usage information for the program.
 *
 * @param prog_name The name of the program.
 */
void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s -a server_address -p port -d ft_root_directory\n", prog_name);
    exit(EXIT_FAILURE);
}

/**
 * @brief The main function of the FTP server program.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @return int Returns 0 on success, or a negative value on error.
 */
int main(int argc, char *argv[]) {
    int server_fd, new_socket, session_id = 1;  //Dichiarazione delle variabili per il server e per il socket client
    struct sockaddr_in address; //Struttura per l'indirizzo del server
    int opt = 1; //Opzione per configurare il socket

    char *server_address = NULL; //Indirizzo IP del server
    int port = -1; //Porta del server
    char *ft_root_directory = NULL; //Directory root per il server FTP

    const char *optstring = "a:p:d:"; //Opzioni accetatte dalla riga di commando

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
            case 'a': //Opzione -a:Indirizzo del server
                server_address = optarg;
                break;
            case 'p': //Opzione -p:numero della porta
                port = atoi(optarg);
                break;
            case 'd': //Opzione -d:directory root per il server
                ft_root_directory = optarg;
                break;
            default:
                print_usage(argv[0]); //Stampa il messaggio di utilizzo e termina
                return 1;
        }
    }

    //Controlla se la directory root esiste, altrimenti la crea
    printf("Check dir %s.\n", ft_root_directory);
    struct stat st = {0}; //Struttura per informazioni sui file
    if (stat(ft_root_directory, &st) == -1) { //Controllo se la directory esiste
        if (mkdir(ft_root_directory, 0755) == -1) { //Se non esiste, la crea con permeessi 0755
            perror("Error creating directory!");
            return 1;
        }
        printf("Directory %s created successfully.\n", ft_root_directory);
    } else {
        printf("Directory %s already exists.\n", ft_root_directory);
    }

    // Create socket del server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("errore socket()");
        exit(EXIT_FAILURE);
    }

    // Configura il socket per riutilizzare indirizzo e porta
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("errore setsockopt()");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    //Configurazione della struttura dell'indirizzo del server
    address.sin_family = AF_INET; //IPV4
    address.sin_port = htons(port); //Porta convertita in network byte order
    address.sin_addr.s_addr = INADDR_ANY; //Accetta connessioni su qualsiasi interfaccia

    //Conversione dell'indirizzo IP in binario
    if (inet_pton(AF_INET, server_address, &address.sin_addr) <= 0) {
        perror("errore impostazione IP inet_pton()");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Binding del socket all'indirizzo e alla porta specificati
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    //Il server inizia ad ascoltare le connessioni in entrata (fino a 3 in coda)
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", port);

    //Ciclo infinito per accettare connessioni in entrata
    while (1) {
        struct sockaddr_in client_addr; //Struttura per l'indirizzo del client
        socklen_t client_addr_len = sizeof(client_addr);

        // Attende una nuova connessione in arrivo
        if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        session_id++; // Incrementa l'ID della sessione
        printf("SessionID: %d. Client connected!\n", session_id);

        // Creazione di un nuovo processo per gestire il client con "fork()
        pid_t pid = fork();
        if (pid < 0) { //Errore nella creazione del processo figlio
            perror("fork");
            close(new_socket);
            continue;
        } else if (pid == 0) { //Processo figlio: gestiscfe il client
            close(server_fd); //Chiude il socket del server nel processo figlio
            handle_client(new_socket, ft_root_directory, session_id); //Gestisce la comunicazione con il client
            exit(EXIT_SUCCESS); //Termina il processo figlio dopo aver servito il client
        } else {
            close(new_socket); //Chiude il socket del client (Ora gestito dal processo figlio)
        }
    }
}

/**
 * @brief Handles the communication with a connected client.
 *
 * @param new_socket The socket descriptor for the connected client.
 * @param ft_root_directory The root directory for the FTP server.
 * @param session_id The session ID for the client connection.
 */
void handle_client(int new_socket, char *ft_root_directory, int session_id) {
    char *operation = NULL; //Operazione richiesta dal client (WRITE,READ,LIST)
    char *server_file_path = NULL; //Percorso dei file sul server
    char *status = "LOADING_OPERATION"; //Stato iniziale del server

    char buffer[BUF_SIZE]; //Buffer per ricevere dati dal client
    int n;

    FILE *file; //Puntatore per operazioni di file

    printf("Server STATUS (initial): %s, %d.\n", status, new_socket);

    //CICLO PRINCIPALE: riceve dati dal client finchè la connessione è attiva
    while ((n = recv(new_socket, buffer, BUF_SIZE - 1, 0)) > 0) {
        buffer[n] = '\0'; //Termina correttamente la stringa ricevuta

        //Stampa i dati ricevuti dal client (solo se non è fase di scrittura dati)
        if (strncmp(status, "SERVER_RECEIVING_DATA", 22) != 0 || strncmp(buffer, "WRITE_COMPLETED", 16) == 0) {
            printf("SessionID: %d. Received data from client: %s\n", session_id, buffer);
        }
        //Determina il tipo di operazione richiesta dal client (WRITE,READ,LIST)
        if (strncmp(status, "LOADING_OPERATION", 18) == 0)  {
            if (strncmp(buffer, "WRITE", 6) == 0)  {
                operation = "WRITE";
                status = "SERVER_LOADING_FILE_NAME";
            } else if (strncmp(buffer, "READ", 5) == 0) {
                operation = "READ";
                status = "SERVER_LOADING_FILE_NAME";
            } else if (strncmp(buffer, "LIST", 5) == 0) {
                operation = "LIST";
                status = "SERVER_LOADING_FILE_NAME";
            }
            sendingData(new_socket, status); //Invia lo stato aggiornato dal client
        //Se il server sta caricando il nome del file richiesto dal client
        } else if (strncmp(status, "SERVER_LOADING_FILE_NAME", 25) == 0)  {
            //Alloca memoria per il percorso completo del file sul server
            server_file_path = malloc(strlen(ft_root_directory) + 1 + strlen(buffer) + 1);

            if (server_file_path == NULL) {
                fprintf(stderr, "Memory allocation failed!\n");
                break;
            }
            //Costruisce il percorso completo del file
            strcpy(server_file_path, ft_root_directory);
            strcat(server_file_path, "/");
            strcat(server_file_path, buffer);
            printf("SessionID: %d. Server side file full path: %s\n", session_id, server_file_path);

            //Se il client ha richiesto di scrivere un file (WRITE)
            if (strncmp(operation, "WRITE", 6) == 0)  {
                status = "SERVER_RECEIVING_DATA";

                creatingDirectories(server_file_path); //Crea directory se necessario
                //Controlla se c'è spazio disponibile per scrivere il file

                if (!checkSpaceAvailable(ft_root_directory)) {
                    printf("(4)No free space on storage device\n");
                    printf("SessionID: %d. (4)No free space on storage device\n", session_id);
                    sleep(2);
                    sendingData(new_socket, "No free space on server storage device\n");
                    break;
                }

                printf("SessionID: %d. Waiting for file %s\n", session_id, server_file_path);
                int fd = waitingForFile(server_file_path);
                if (fd == -1) {
                    fprintf(stderr, "Failed to open file: %s!\n", server_file_path);
                    status = "FAILED OPENING FILE";
                } else {
                    file = fopen(server_file_path, "wb"); //Apre il file in modalità scrittura binaria
                    if (file == NULL) {
                        perror("fopen");
                    }
                }

                sendingData(new_socket, status);

                printf("SessionID: %d. Server writing in file %s\n", session_id, server_file_path);
            //Se il client ha richiesto di leggere un file (READ)
            } else if (strncmp(operation, "READ", 5) == 0)  {
                status = "SERVER_TRASFERRING_DATA";

                printf("Check esistenza file %s\n", server_file_path);
                struct stat st = {0};
                //Controlla se il file esiste
                if (stat(server_file_path, &st) == -1) {
                    printf("SessionID: %d. File %s not found.\n", session_id, server_file_path);
                    sendingData(new_socket, "FILE_NOT_FOUND");
                } else {
                    sendingData(new_socket, status);
                    sleep(2);

                    printf("SessionID: %d. Waiting for file %s\n", session_id, server_file_path);
                    waitingForFile(server_file_path); //Attende che il file sia disponibile

                    printf("SessionID: %d. Server sending data to client from file %s\n", session_id, server_file_path);
                    //Apre il file in modalità lettura binaria
                    FILE *file = fopen(server_file_path, "rb");
                    if (!file) {
                        printf("fopen error on %s\n", server_file_path);
                        exit(EXIT_FAILURE);
                    }

                    printf("SessionID: %d. Sleeping to simulate latency...\n", session_id);
                    //Simula latenza prima della trasmissionione
                    sleep(10);
                    while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                        if (socketClosed(new_socket) > 0) {
                            break; //Il client ha chiuso -> smetto di inviare, se il client chiude mentre il server invia se ne accorge e interrompe il trasferimento
                        } //Se il server chiude  la connessione mentre il client sta inviando
                        if (send(new_socket, buffer, n, 0) == -1) {
                            printf("send error");
                            exit(EXIT_FAILURE);
                        }
                    }
                    fclose(file);

                    sleep(5);
                    sendingData(new_socket, "FILE_TRANFER_COMPLETED");
                }
            //Se il client ha richiesto di elencare i file di una directory (LIST)
            } else if (strncmp(operation, "LIST", 5) == 0)  {
                struct stat st;
                if (stat(server_file_path, &st) == 0) {
                    if (S_ISDIR(st.st_mode)) {
                        FILE *fp;
                        char path[1024];
                        char command[256];
                        char *status = "LIST_COMPLETED";

                        snprintf(command, sizeof(command), "ls -al %s", server_file_path);

                        fp = popen(command, "r");
                        if (fp == NULL) {
                            status = "LIST_FAILED";
                            send(new_socket, status, strlen(status), 0);
                            return;
                        }

                        while (fgets(path, sizeof(path), fp) != NULL) {
                            if (send(new_socket, path, strlen(path), 0) == -1) {
                                perror("send");
                                status = "LIST_FAILED";
                                break;
                            }
                        }

                        if (pclose(fp) == -1) {
                            perror("pclose");
                            status = "LIST_FAILED";
                        }

                        sleep(2);
                        sendingData(new_socket, status);

                    } else {
                        printf("SessionID: %d. %s is not a directory.\n\n", session_id, server_file_path);
                        sendingData(new_socket, "NOT_A_DIR");
                    }
                } else {
                    printf("SessionID: %d. Directory %s not found.\n\n", session_id, server_file_path);
                    sendingData(new_socket, "DIR_NOT_FOUND");
                }
            }
        //Fase di ricezione dati per un'operazione WRITE
        } else if (strncmp(status, "SERVER_RECEIVING_DATA", 22) == 0)  {
            if (strncmp(buffer, "WRITE_COMPLETED", 16) == 0)  {
                sleep(2);
                fclose(file);
                printf("SessionID: %d. Server writing completed %s!\n\n", session_id, server_file_path);
            } else if(ends_with(buffer, "WRITE_COMPLETED")) {
                int new_str_length = n - 16 + 1;
                char *new_str = malloc((new_str_length) * sizeof(char));
                if (new_str == NULL) {
                    perror("Failed to allocate memory");
                    exit(EXIT_FAILURE);
                }
                strncpy(new_str, buffer, new_str_length);
                new_str[new_str_length] = '\0';

                printf("SessionID: %d. ...buffer is: %s *\n", session_id, new_str);

                fwrite(new_str, 1, new_str_length, file);
                fclose(file);
                printf("SessionID: %d. Server writing completed %s!\n\n", session_id, server_file_path);

            } else {
                if (!checkSpaceAvailable(ft_root_directory)) {
                    printf("(5)No free space on storage device\n");
                    printf("SessionID: %d. (5)No free space on storage device\n\n", session_id);
                    sleep(2);
                    sendingData(new_socket, "STORAGE_SPACE_ISSUE");
                    break;
                }

                fwrite(buffer, 1, n, file); //Scrive i dati ricevuti nel file
            }
        }
    }
    //Se la ricezione dei dati ha fallito
    if (n < 0) {
        perror("Receive failed!");
    }
}


