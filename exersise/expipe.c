#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

// int main() {
//     pid_t pid = fork();

//     if (pid < 0) {
//         perror("Fork failed");
//         return 1;
//     } else if (pid == 0) {
//         // Child process
//         printf("Child process running with PID %d\n", getpid());
//         sleep(2);  // Simulate some work
//         return 42;  // Exit with a custom status
//     } else {
//         // Parent process
//         int status;
//         pid_t result = waitpid(pid, &status, 0);  // Wait for the specific child (blocking)

//         if (result > 0) {
//             if (WIFEXITED(status)) {
//                 printf("Child %d exited with status %d\n", result, WEXITSTATUS(status));
//             }
//         } else {
//             perror("waitpid failed");
//         }
//     }

//     return 0;
// }

int main() {
    int pipefd[2];  // Array to hold the file descriptors for the pipe
    pid_t pid;
    char writeMsg[] = "Hello from parent to child";
    char readBuffer[128];

    // Create the pipe
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return 1;
    }

    // Create a new process
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {  // Child process
        close(pipefd[1]);  // Close the write end of the pipe

        // Read data from the pipe
        read(pipefd[0], readBuffer, sizeof(readBuffer));
        printf("Child received: %s\n", readBuffer);

        close(pipefd[0]);  // Close the read end of the pipe
    } else {  // Parent process
        close(pipefd[0]);  // Close the read end of the pipe

        // Write data to the pipe
        write(pipefd[1], writeMsg, strlen(writeMsg) + 1);
        printf("Parent sent: %s\n", writeMsg);

        close(pipefd[1]);  // Close the write end of the pipe
    }

    return 0;
}

   // // Loop to create numofsplitters child processes
    // for (i = 0; i < numOfSplitter; i++) {
    //     pid = fork(); // Create a new process (fork)

    //     if (pid == 0) {
    //         // Child process
    //         printf("Child %d PID: %d\n", i+1, getpid()); // Print child process ID
    //         sleep(1); // Simulate some work
    //         exit(0); // Terminate child process
    //     } else if (pid < 0) {

    //         perror("fork failed");
    //         exit(1);
    //     }
    // }
    
    // // Parent process
    // for (i = 0; i < numOfSplitter; i++) {
    //     // Wait for each child process to terminate
    //     wait(NULL); // Wait for child processes to terminate
    // }
    // // Print parent process ID
    // printf("Parent PID: %d\n", getpid()); // Print parent process ID
    
