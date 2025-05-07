#include "ftp_lib.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/file.h>


#define BUF_SIZE 1024

/**
 * @brief Attempts to open a specified file and checks if it is locked by another process.
 * If the file is locked, it waits and retries until the file is available.
 *
 * @param file_path The path to the file.
 * @return int Returns 0 if the file becomes available, -1 if it cannot be opened.

 */
int waitingForFile(const char *file_path) {
    int fd;
    struct flock lock; // Initialize a flock structure for file locking

    while (1) {

        // Try to open the file in read/write mode
        fd = open(file_path, O_RDWR);
        if (fd == -1) {
            // The file does not exist
            close(fd);  // Close the file descriptor, even if it is not valid.
            return 0;
        }


        // The flock() function allows checking if the file just opened has been previously opened by another thread,
        // in this case, flock cannot obtain exclusive access to the file.
        // exclusive lock (LOCK_EX)  -  non-blocking way (LOCK_NB)
        if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
            perror("flock");
            printf("...file is locked by another process. Retrying in %d seconds...\n", 5);
        } else {
            return 0;
        }

        // If the file is locked, wait and retry
        close(fd);
        sleep(5);
    }
}

/**
 * @brief Sends a status message to a client via a socket connection.
 *
 * @param client_socket The socket descriptor of the client.
 * @param status The status message to be sent.
 */
void sendingData(int client_socket, char *status)
{
    char buffer_out[BUF_SIZE]; // Buffer to hold the status message
    strcpy(buffer_out, status); // Copy the status into the output buffer
    send(client_socket, buffer_out, strlen(buffer_out), 0); // Send the buffer content to the client via the socket
    printf("Sent data: %s\n", buffer_out);
}

/**
 * @brief Takes a path as input and creates all directories in the specified path.
 * If the directories already exist, the function skips them and continues with the next path.
 *
 * @param path The path where directories should be created.
 */
void creatingDirectories(const char *path) {
    char temp[1024];
    char *pos = temp;

    // Copy the path into a temporary buffer
    strncpy(temp, path, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';  // Ensure null termination

    // Iterate through the path
    while (*pos) {
        if (*pos == '/' || *pos == '\\') {
            *pos = '\0';

            if (strlen(temp) > 0 && (strlen(temp) != 2 || temp[1] != ':')) {
                // Check if the directory exists, otherwise create it
                if (mkdir(temp, 0755) != 0 && errno != EEXIST) {
                    // Check if an error other than "directory already exists" occurs
                    fprintf(stderr, "Error creating directory %s: %s\n", temp, strerror(errno));
                    return;
                }
            }
            *pos = '/';  // Restore the separator
        }
        pos++;
    }
}

/**
 * @brief Checks if a socket has been closed by the remote side.
 *
 * @param sockfd The socket descriptor.
 * @return int Returns 1 if the socket is closed, otherwise returns 0.
 */
int socketClosed(int sockfd) {
    fd_set readfds; // Set of file descriptors
    struct timeval tv; // Timeout structure
    int ret; // Return value of select()
    char buffer[1];

    // Initialize the set of file descriptors
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    // Set the timeout to zero seconds (no timeout)
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    //0 SOCKET ATTIVO, 1 SOCKET CHIUSO
    // Use select() to check if there is data available on the socket.
    ret = select(sockfd + 1, &readfds, NULL, NULL, &tv);
    if (ret == -1) {
        perror("select");
        return 1; // Assume the socket is closed in case of error
    } else if (ret == 0) {
        // No data available, the socket is still open
        return 0;
    } else {
        if (FD_ISSET(sockfd, &readfds)) {
            // The socket has data available for reading or has been closed
            ret = recv(sockfd, buffer, sizeof(buffer), MSG_PEEK);
            if (ret == 0) {
                // The connection has been closed by the remote side
                return 1;
            } else if (ret < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No data available for reading, the socket is still open
                    return 0;
                } else {
                    // An error occurred
                    perror("recv");
                    return 1;
                }
            }
        }
    }
    return 0;
}

/**
 * @brief Checks if there is enough space available in a specified directory.
 *
 * @param dir_path The path to the directory.
 * @return bool Returns true if there is enough space, otherwise returns false.
 */
bool checkSpaceAvailable(const char *dir_path) {
    struct statvfs stat; // Get file system statistics

    // Get the file system information of the specified directory
    if (statvfs(dir_path, &stat) == 0) {
        // Calculate the free space in bytes
        unsigned long long free_space = stat.f_bsize * stat.f_bavail;
        printf("Free space in directory: %llu bytes\n", free_space);

        // Check if the free space is greater than 2048 bytes
        if (free_space > 2048) {
            return true;  // There is enough space available
        } else {
            return false;  // There is not enough space available
        }
    } else {
        // Unable to get the file system information
        perror("statvfs");
        return false;
    }
}

/**
 * @brief Checks if a string (buffer) ends with a specified suffix.
 *
 * @param buffer The buffer to be checked.
 * @param suffix The suffix to be checked for.
 * @return bool Returns true if the buffer ends with the suffix, otherwise returns false.
 */
bool ends_with(const char *buffer, const char *suffix) {
    size_t buffer_len = strlen(buffer);
    size_t suffix_len = strlen(suffix);

    // If the suffix is longer than the buffer string, it cannot be a suffix
    if (suffix_len > buffer_len) {
        return false;
    }

    // Compare the end of the buffer string with the suffix
    return strncmp(buffer + buffer_len - suffix_len, suffix, suffix_len) == 0;
}