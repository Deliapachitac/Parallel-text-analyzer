#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>


int main() {
    // Open a file for writing
    int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return 1;
    }

    // Redirect stdout to the file descriptor fd
    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2 failed");
        return 1;
    }

    // Now writing to stdout will actually write to output.txt
    printf("This is redirected output to output.txt\n");

    // Close the file descriptor
    close(fd);

    return 0;
}

// int main() {
//     // Open a file for writing (create or truncate it)
//     int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
//     if (fd == -1) {
//         perror("open failed");
//         return 1;
//     }

//     // Duplicate the file descriptor, duplicating the standard output (stdout)
//     int newfd = dup(fd);
//     if (newfd == -1) {
//         perror("dup failed");
//         return 1;
//     }

//     // Now write to the new file descriptor
//     dprintf(newfd, "This is redirected output to output.txt\n");

//     // Close the file descriptors
//     close(fd);
//     close(newfd);

//     return 0;
// }
