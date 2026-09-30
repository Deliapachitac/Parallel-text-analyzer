#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid;
    
    // Fork a child process
    pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        return 1;
    }
    
    if (pid == 0) {
        // Child process: it will sleep for 10 seconds
        printf("Child process started with PID %d\n", getpid());
        sleep(10); // Simulate some work
        printf("Child process finished\n");
        exit(0);
    } else {
        // Parent process: it will send a SIGTERM to the child after 2 seconds
        printf("Parent process will terminate the child with SIGTERM\n");
        sleep(2);
        if (kill(pid, SIGTERM) == -1) {
            perror("kill failed");
            return 1;
        }
        printf("Signal sent to terminate the child process\n");
        sleep(1); // Wait to see if the child is terminated
    }
    
    return 0;
}
