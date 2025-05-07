#include <stdbool.h>
#ifndef FTPLIB_H
#define FTPLIB_H

/**
 * @brief Waits for a file to be available at the specified path.
 *
 * @param file_path The path to the file.
 * @return int Returns 0 if the file is available, otherwise returns an error code.
 */
int waitingForFile(const char *file_path);

/**
 * @brief Sends data to the client socket with a given status.
 *
 * @param client_socket The socket descriptor of the client.
 * @param status The status message to be sent.
 */
void sendingData(int client_socket, char *status);

/**
 * @brief Creates directories along the specified path.
 *
 * @param path The path where directories should be created.
 */
void creatingDirectories(const char *path);

/**
 * @brief Checks if the socket is closed.
 *
 * @param sockfd The socket descriptor.
 * @return int Returns 1 if the socket is closed, otherwise returns 0.
 */
int socketClosed(int sockfd);

/**
 * @brief Checks if there is enough space available in the specified directory.
 *
 * @param dir_path The path to the directory.
 * @return bool Returns true if there is enough space, otherwise returns false.
 */
bool checkSpaceAvailable(const char *dir_path);

/**
 * @brief Checks if the buffer ends with the specified suffix.
 *
 * @param buffer The buffer to be checked.
 * @param suffix The suffix to be checked for.
 * @return bool Returns true if the buffer ends with the suffix, otherwise returns false.
 */
bool ends_with(const char *buffer, const char *suffix);

#endif

