#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> // Include ARPA Internet library for internet address manipulation
#include <errno.h> // Include error number library for error handling
#include <fcntl.h> // Include file control library for file control operations
#include <stdbool.h> // Include boolean library for boolean type support

#include "ftp_lib.h"

#define BUFFER_SIZE 1024

/**
 * @brief Prints the usage information for the program.
 *
 * @param prog_name The name of the program.
 */
void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s -w -a server_address -p port -f local_path/filename_local -o remote_path/filename_remote\n", prog_name);
    fprintf(stderr, "Usage: %s -r -a server_address -p port -f remote_path/filename_remote -o local_path/filename_local\n", prog_name);
    fprintf(stderr, "Usage: %s -l -a server_address -p port -f remote_path_dir\n", prog_name);
    exit(EXIT_FAILURE);
}

/**
 * @brief The main function of the client program.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @return int Returns 0 on success, or a negative value on error.
 */
int main(int argc, char *argv[]) {

    int opt;    // Variabile per memorizzare l'opzione corrente
    char *operation = NULL; // Operazione richiesta dal client (WRITE, READ, LIST)
    char *server_address = NULL; // Indirizzo IP del server
    int port = -1; // Porta del server
    char *source_file_path = NULL; // Percorso del file sorgente
    char *target_file_path = NULL; // Percorso del file destinazione
    char *list_path = NULL; // Percorso della directory da elencare

    char *status = "IDLE"; //Stato iniziale del client

    const char *optstring = "wrl:::a:p:f:o:"; //Opzioni accettate da getopt()

    FILE *localFile; //Puntatore per opeazioni di file

    // La funzione getopt in C è utilizzata per analizzare gli argomenti della riga di comando passati
    // a un programma. Questa funzione facilita la gestione degli argomenti e delle opzioni della
    // riga di comando, permettendo di specificare opzioni con o senza argomenti associati
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
            case 'w':
                operation = "WRITE"; // Operazione di scrittura
                break;
            case 'r':
                operation = "READ"; // Operazione di lettura
                break;
            case 'l':
                operation = "LIST"; // Operazione di elenco
                break;
            case 'a':
                server_address = optarg; // Indirizzo IP del server
                break;
            case 'p':
                port = atoi(optarg); // Porta del server
                break;
            case 'f':
                source_file_path = optarg; // Percorso del file sorgente
                list_path = optarg; //Percorso della direcotry da elencare
                break;
            case 'o':
                target_file_path = optarg; //Percorso del file di destinazione
                break;
            default:
                print_usage(argv[0]); //Stampa l'uso corretto se c'è un errore
                return 1;
        }
    }

    // In caso il remote_path/filename_remote è omesso dal client, dovrò scrivere/leggere il file sul server con lo stesso nome del local_path/filename_local
    if (source_file_path == NULL) {
        source_file_path = target_file_path;
    } else if (target_file_path == NULL) {
        target_file_path = source_file_path;
    }

    // Verifica che tutti i parametri obbligatori siano presenti
    if (operation == NULL) {
        printf("-w, -r or -l option is missing \n");
        print_usage(argv[0]);
    }

    if (source_file_path == NULL) {
        printf("-f option is missing \n");
        print_usage(argv[0]);
    }

    if (server_address == NULL || port == -1) {
        printf("-a or -p option is missing \n");
        print_usage(argv[0]);
    }

    // DEBUG: stampa i parametri ricevuti dal client
    printf("operation: %s\n", operation);
    printf("server_address: %s\n", server_address);
    printf("port: %d\n", port);
    printf("source_file_path: %s\n", source_file_path);
    printf("target_file_path: %s\n", target_file_path);


    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Crea il file descriptor del socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error!\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET; //Importa il tipo di indirizzo (IPV4)
    serv_addr.sin_port = htons(port); //Converte la porta in formato big-endian

    // Converti l'indirizzo IP da testo a binario
    if (inet_pton(AF_INET, server_address, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address / Address not supported\n");
        return -1;
    }

    // Connetti il client al server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Invia il tipo di operazione al server
    send(sock, operation, strlen(operation), 0);
    printf("Client sent: %s", operation);
    printf("\n");

    //Ascolta le risposte del server
    int n;
    while ((n = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[n] = '\0'; //Termina correttamente la stringa ricevuta

        if (strncmp(status, "READING", 8) != 0) {
            printf("Client received: %s\n", buffer);
        }
        //Se il server chiede il nome del file
        if (strncmp(buffer, "SERVER_LOADING_FILE_NAME", 25) == 0) {
            if (strncmp(operation, "WRITE", 5) == 0) {
                sendingData(sock, target_file_path);
            } else if (strncmp(operation, "READ", 4) == 0) {
                sendingData(sock, source_file_path);
            } else if (strncmp(operation, "LIST", 4) == 0) {
                sendingData(sock, list_path);
            }
        //Se il server è pronto a ricevere i dati
        } else if (strncmp(buffer, "SERVER_RECEIVING_DATA", 22) == 0) {
            printf("Client: Sending file data\n");

            // INVIO IL FILE
            FILE *file = fopen(source_file_path, "rb");
            if (!file) {
                printf("fopen error on %s\n", source_file_path);
                exit(EXIT_FAILURE);
            }
            printf("Sleeping to simulate latency\n");
            sleep(10); // SIMULAZIONE LATENZA
            while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                // VERIFICA STATO DELLA CONNESSIONE ED ESCO SE INTERROTTA
                if (socketClosed(sock) > 0) {
                    break;
                }
                // INVIO DATO AL SERVER
                if (send(sock, buffer, n, 0) == -1) {
                    printf("send error");
                    exit(EXIT_FAILURE);
                }
            }
            fclose(file);


            sendingData(sock, "WRITE_COMPLETED");
            break;

        } else if (strncmp(buffer, "SERVER_TRASFERRING_DATA", 20) == 0) {

            // SE NON ESISTE IL PATH CREO LE DIRECTORY
            creatingDirectories(target_file_path);

            // APRO IL FILE
            printf("Client waiting for file: %s\n", target_file_path);
            int fd = waitingForFile(target_file_path);
            if (fd == -1) {
                fprintf(stderr, "Failed to open file: %s\n", target_file_path);
                status = "FAILED OPENING FILE";
            } else {
                // Apro il file per SCRIVERE O LEGGERE i dati
                localFile = fopen(target_file_path, "wb");
                if (localFile == NULL) {
                    perror("fopen");
                    // GESTIRE EVENTUALE ERRORE
                }
            }

            // VERIFICA SPAZIO STORAGE
            if (!checkSpaceAvailable(target_file_path)) {
                printf("(1)No free space on storage device\n");
                break;
            }

            printf("Client: Receiving data from server\n");
            printf("Client writing in file %s\n", target_file_path);
            status = "READING";

        } else if (strncmp(status, "READING", 8) == 0) {

            // VERIFICA SPAZIO STORAGE PER SCRIVERE
            if (!checkSpaceAvailable(target_file_path)) {
                fclose(localFile);
                printf("(2)No free space on storage device %s\n", target_file_path);
                break;
            }

            if (strncmp(buffer, "FILE_TRANFER_COMPLETED", 23) == 0) {
                fclose(localFile);
                printf("Client writing completed %s\n\n", target_file_path);
                break;
            } else if (ends_with(buffer, "FILE_TRANFER_COMPLETED")) {

                int new_str_legth = n - 23 + 1;
                char *new_str = malloc((new_str_legth) * sizeof(char));
                if (new_str == NULL) {
                    perror("Failed to allocate memory");
                    exit(EXIT_FAILURE);
                }

                // Rimuovo il messaggio FILE_TRANFER_COMPLETED dal buffer
                strncpy(new_str, buffer, new_str_legth); // Copia il messaggio senza il suffisso
                new_str[new_str_legth] = '\0'; // Assicuro la terminazione della stringa

                fwrite(new_str, 1, new_str_legth, localFile);
                fclose(localFile);
                printf("Client writing completed %s\n\n", target_file_path);
                break;
            } else {
                fwrite(buffer, 1, n, localFile);
            }

        } else if (strncmp(buffer, "FILE_NOT_FOUND", 15) == 0) {
            printf("File %s not found!\n", source_file_path);
            break;
        } else if (strncmp(buffer, "LIST_COMPLETED", 15) == 0) {
            printf("List command completed!\n");
            break;
        } else if (strncmp(buffer, "LIST_FAILED", 15) == 0) {
            printf("List command failed!\n");
            break;
        } else if (strncmp(buffer, "DIR_NOT_FOUND", 14) == 0) {
            printf("Directory %s not found!\n", target_file_path);
            break;
        } else if (strncmp(buffer, "NOT_A_DIR", 10) == 0) {
            printf("%s is not a directory!\n", target_file_path);
            break;
        } else if (strncmp(buffer, "STORAGE_SPACE_ISSUE", 20) == 0) {
            printf("Space issue on storage");
            break;
        }
    }

    // Chiudi il socket
    close(sock);

    return 0;
}

//gcc -c -fPIC ftp_lib.c -o ftp_lib.o
//gcc -shared -o libftp_lib.so ftp_lib.o

//gcc -o myFTServer myFTserver.c -L. -lftp_lib
//gcc -o myFTClient myFTclient.c -L. -lftp_lib

