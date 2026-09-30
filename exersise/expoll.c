#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

#include <stdlib.h>
#include <sys/select.h>


int main() {
    fd_set readfds;
    struct timeval timeout;
    int ret;

    // Open a file for monitoring
    int fd = open("ExclusionList.txt", O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file");
        return 1;
    }

    // Initialize the file descriptor set
    FD_ZERO(&readfds);  // Clear the fd set
    FD_SET(STDIN_FILENO, &readfds);  // Monitor stdin
    FD_SET(fd, &readfds);  // Monitor the file descriptor

    // Set the timeout
    timeout.tv_sec = 5;   // Wait for 5 seconds
    timeout.tv_usec = 0;

    // Call select() to check for available I/O
    ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
    if (ret == -1) {
        perror("select failed");
        close(fd);
        return 1;
    } else if (ret == 0) {
        printf("No data available within the timeout period.\n");
    } else {
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char buffer[128];
            ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';  // Null-terminate the string
                printf("Data available on stdin: %s\n", buffer);
            }
        }

        if (FD_ISSET(fd, &readfds)) {
            char buffer[128];
            ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';  // Null-terminate the string
                printf("Data available in file: %s\n", buffer);
            }
        }
    }

    // Close the file descriptor
    close(fd);

    return 0;
}

// int main() {
//     struct pollfd fds[2];
    
//     // Monitor standard input (fd 0)
//     fds[0].fd = STDIN_FILENO;
//     fds[0].events = POLLIN;  // Check for input availability
    
//     // Monitor a file descriptor (assuming fd 3 is open and valid)
//     int fd = open("ExclusionList.txt", O_RDONLY);
//     if (fd == -1) {
//         perror("open failed");
//         return 1;
//     }
//     fds[1].fd = fd;
//     fds[1].events = POLLIN;  // Check for input availability
    
//     int timeout = 5000;  // Timeout of 5 seconds

//     int ret = poll(fds, 2, timeout);
//     if (ret == -1) {
//         perror("poll failed");
//         return 1;
//     } else if (ret == 0) {
//         printf("Timeout occurred! No data available.\n");
//     } else {
//         for (int i = 0; i < 2; i++) {
//             if (fds[i].revents & POLLIN) {
//                 printf("File descriptor %d is ready for reading.\n", fds[i].fd);
//             }
//         }
//     }
    
//     close(fd);
//     return 0;
// }
