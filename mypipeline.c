#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>  

int main(int argc, char *argv[]) {
    int pipefd[2];           // pipefd[0] = read end, pipefd[1] = write end
    pid_t pid;

    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(1);
    }

    fprintf(stderr, "(parent_process>forking…)\n"); 
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid > 0) {
        // Parent process
        fprintf(stderr, "(parent_process>created process with id: %d)\n", pid); 

        fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");
        close(pipefd[1]); // Close the write end of the pipe

        fprintf(stderr, "(parent_process>forking…)\n"); 
        int pid_2 = fork();
        if (pid_2 < 0) {
            perror("fork failed");
            exit(1);
        }
        if (pid_2 == 0) {
            // child2 process
            fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n"); 
            close(0);
            dup(pipefd[0]); // Duplicate the read end of the pipe to stdin
            close(pipefd[0]); // Close duplicated fd

            fprintf(stderr, "(child2>going to execute cmd: wc)\n"); 
            char *wcArgs[] = {"wc", NULL};
            execvp("wc", wcArgs);
            perror("execvp failed"); 
            exit(1); 
        }

        fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n"); 
        close(pipefd[0]); // Close the original read end of the pipe

        fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
        waitpid(pid, NULL, 0); 
        waitpid(pid_2, NULL, 0); // Wait for both child processes to finish

        fprintf(stderr, "(parent_process>exiting…)\n"); 

    } else {
        // child1 process
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n"); // <== הוספה: הודעה לפני dup ב-child1
        close(1);
        dup(pipefd[1]); // Duplicate the write end of the pipe to stdout

        close(pipefd[1]); // Close the original write end of the pipe
        close(pipefd[0]); // Close the original read end of the pipe

        fprintf(stderr, "(child1>going to execute cmd: ls)\n"); 
        char *lsArgs[] = {"ls", "-ls", NULL};
        execvp("ls", lsArgs);
        perror("execvp failed"); 
        exit(1); 
    }

    return 0;
}
