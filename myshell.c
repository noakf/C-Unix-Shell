#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include "LineParser.h"
#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0
#define FREE(X) if(X) free((void*)X)
# define HISTLEN 20

typedef struct process
{
    cmdLine *cmd;         /* the parsed command line*/
    pid_t pid;            /* the process id that is running the command*/
    int status;           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next; /* next process in chain */
} process;
typedef struct histNode {
    char* commandLine;
    struct histNode* next;
} histNode;

histNode* histHead = NULL;
histNode* histTail = NULL;
int histSize = 0;
process *process_list = NULL; 

void addHistory(const char* line) {
    char* copy = strdup(line);
    histNode* newNode = malloc(sizeof(histNode));
    newNode->commandLine = copy;
    newNode->next = NULL;

    if (histTail) {
        histTail->next = newNode;
    } else {
        histHead = newNode;
    }
    histTail = newNode;
    histSize++;

    if (histSize > HISTLEN) {
        histNode* temp = histHead;
        histHead = histHead->next;
        free(temp->commandLine);
        free(temp);
        histSize--;
    }
}

void printHistory() {
    int i = 1;
    histNode* current = histHead;
    while (current) {
        printf("%d: %s\n", i++, current->commandLine);
        current = current->next;
    }
}

void freeHistory() {
    histNode* tmp;
    while (histHead) {
        tmp = histHead;
        histHead = histHead->next;
        free(tmp->commandLine);
        free(tmp);
    }
}

const char* statusToString(int status) {
    switch (status) {
        case RUNNING:
            return "Running";
        case SUSPENDED:
            return "Suspended";
        case TERMINATED:
            return "Terminated";
        default:
            return "Unknown";
    }
}
void updateProcessStatus(process* process_list, int pid, int status){
    process *current = process_list;
    while (current != NULL)
    {
        if (current->pid == pid)
        {
            current->status = status;
            break;
        }

        current = current->next;
    }
}
void updateProcessList(process **process_list){
    process *current = *process_list;
    while (current != NULL)
    {
        int status;
        pid_t result = waitpid(current->pid, &status, WNOHANG  | WUNTRACED);
        if (result == 0)
        {
            // Process is still running
            current->status = RUNNING;
        }
        else if (result == -1)
        {
            // Error occurred
            current->status = TERMINATED;
        }
        else
        {
            // Process has terminated
            if (WIFEXITED(status))
            {
                current->status = TERMINATED;
            }
            else if (WIFSTOPPED(status))
            {
                current->status = SUSPENDED;
            }
        }
        current = current->next;
    }

}
void freeCurrentCmdLine(cmdLine *cmd)
{
    if (cmd == NULL)
        return;
    FREE(cmd->inputRedirect);
    FREE(cmd->outputRedirect);
    for (int i = 0; i < cmd->argCount; i++)
    {
        free(cmd->arguments[i]);
    }
    free(cmd);
}

void freeProcessList(process* process_list){
    int isFreed = 0;
    while(process_list != NULL){
        process *temp = process_list;
        process_list = process_list->next;
        if (!isFreed){
            if(temp->cmd->next != NULL){
                isFreed = 1;
            }
            freeCmdLines(temp->cmd);

        } else {
            isFreed=0;
        }
        free(temp);
    }
}

void addProcess(process **process_list, cmdLine *cmd, pid_t pid)
{
    process *new_process = (process *)malloc(sizeof(process));
    new_process->cmd = cmd;
    new_process->pid = pid;
    new_process->status = RUNNING;
    new_process->next = *process_list;
    *process_list = new_process;
}

void printProcessList(process **process_list)
{
    updateProcessList(process_list);
    process *current = *process_list;
    process *prev = NULL;
    int index = 0;

    printf("Index   PID   Command   STATUS\n");

    while (current != NULL)
    {
        printf("%d  %d  ", index, current->pid);
        for (int i = 0; i < current->cmd->argCount; i++)
        {
            printf("%s ", current->cmd->arguments[i]);
        }
        printf("    %s\n", statusToString(current->status));

        // If TERMINATED, delete from list
        if (current->status == TERMINATED)
        {
            process *toDelete = current;
            if (prev == NULL)
            {
                // head of the list
                *process_list = current->next;
                current = *process_list;
            }
            else
            {
                prev->next = current->next;
                current = current->next;
            }

            // Disconnect cmd->next to avoid freeing the rest of the pipeline
            if (toDelete->cmd != NULL)
                toDelete->cmd->next = NULL;
            freeCurrentCmdLine(toDelete->cmd);
            free(toDelete);
        }
        else
        {
            prev = current;
            current = current->next;
            index++;
        }
    }
}


void execute(cmdLine *pCmdLine, int debug_mode)
{
    int hasNext = 0;
    int pipefd[2]; // pipefd[0] = read end, pipefd[1] = write end
    if (pCmdLine->next != NULL)
    {
        hasNext = 1;
        if (pipe(pipefd) == -1)
        {
            perror("pipe failed");
            exit(1);
        }
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0)
    {
        // child process
        if (pCmdLine->inputRedirect != NULL)
        {
            FILE *inFile = fopen(pCmdLine->inputRedirect, "r");
            if (inFile == NULL)
            {
                perror("failed to open input file");
                _exit(1);
            }
            dup2(fileno(inFile), STDIN_FILENO);
            fclose(inFile);
        }

        if (pCmdLine->outputRedirect != NULL && hasNext)
        {
            perror("output redirection not supported in child2");
            _exit(1);
        }
        else if (!hasNext)
        {
            if (pCmdLine->outputRedirect != NULL)
            {
                FILE *outFile = fopen(pCmdLine->outputRedirect, "w");
                if (outFile == NULL)
                {
                    perror("failed to open output file");
                    _exit(1);
                }
                dup2(fileno(outFile), STDOUT_FILENO);
                fclose(outFile);
            }
        }

        if (hasNext)
        {
            dup2(pipefd[1], STDOUT_FILENO); 
            close(pipefd[1]);               
            close(pipefd[0]);              
        }

        if (debug_mode)
        {
            fprintf(stderr, "About to execvp command: %s\n", pCmdLine->arguments[0]);
        }

        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("execvp failed");
        _exit(1);
    }
    else
    {
        // parent process
        if (debug_mode)
        {
            fprintf(stderr, "PID: %d\n", pid);
            fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
        }
        addProcess(&process_list, pCmdLine, pid); 

        if (hasNext)
        {
            cmdLine *pCmdLine2 = pCmdLine->next;
            pid_t pid2 = fork();
            if (pid2 == -1)
            {
                perror("fork failed");
                exit(1);
            }
            if (pid2 == 0)
            {
                // child2 process
                if (pCmdLine2->inputRedirect != NULL)
                {
                    perror("input redirection not supported in child2");
                    _exit(1);
                }

                if (pCmdLine2->outputRedirect != NULL)
                {
                    FILE *outFile = fopen(pCmdLine2->outputRedirect, "w");
                    if (outFile == NULL)
                    {
                        perror("failed to open output file");
                        _exit(1);
                    }
                    dup2(fileno(outFile), STDOUT_FILENO);
                    fclose(outFile);
                }
                dup2(pipefd[0], STDIN_FILENO); 
                close(pipefd[0]);              
                close(pipefd[1]);              

                if (debug_mode)
                {
                    fprintf(stderr, "About to execvp command: %s\n", pCmdLine2->arguments[0]);
                }
                execvp(pCmdLine2->arguments[0], pCmdLine2->arguments);
                perror("execvp failed");
                _exit(1);
            } else {
                addProcess(&process_list, pCmdLine2, pid2); 
            }

            close(pipefd[0]);
            close(pipefd[1]);

            if (pCmdLine->blocking || pCmdLine2->blocking)
            {
                waitpid(pid, NULL, 0);
                waitpid(pid2, NULL, 0);
            }
        }
        else if (pCmdLine->blocking)
        {
            waitpid(pid, NULL, 0);
        }
    }
}

int main(int argc, char const *argv[])
{
    char inputBuffer[2048];
    char cwd[PATH_MAX];

    int debug_mode = 0;
    if (argc > 1 && strcmp(argv[1], "-d") == 0)
    {
        debug_mode = 1;
    }

    while (1)
    {
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("%s$ ", cwd);
            fflush(stdout);
        }
        else
        {
            perror("getcwd failed");
            exit(1);
        }

        if (fgets(inputBuffer, sizeof(inputBuffer), stdin) == NULL)
        {
            freeProcessList(process_list);
            freeHistory();
            process_list = NULL;
            break; // Control-D
        }

        inputBuffer[strcspn(inputBuffer, "\n")] = '\0';

        if (strcmp(inputBuffer, "!!") == 0)
        {
            if (!histTail)
            {
                fprintf(stderr, "No history yet.\n");
                continue;
            }
            strcpy(inputBuffer, histTail->commandLine); 
        }
        else if (inputBuffer[0] == '!' && isdigit(inputBuffer[1])) 
        {
            int n = atoi(&inputBuffer[1]);
            if (n < 1 || n > histSize)
            {
                fprintf(stderr, "Invalid history reference.\n");
                continue;
            }
            histNode *curr = histHead;
            for (int i = 1; i < n; i++) curr = curr->next;
            strcpy(inputBuffer, curr->commandLine); 
        }

        addHistory(inputBuffer); 

        
        cmdLine *cmd = parseCmdLines(inputBuffer);
        if (cmd == NULL)
        {
            continue;
        }

        if (strcmp(cmd->arguments[0], "quit") == 0)
        {
            freeCmdLines(cmd);
            freeProcessList(process_list);
            freeHistory();
            process_list = NULL;
            break;
        }

        if (strcmp(cmd->arguments[0], "cd") == 0)
        {
            if (cmd->argCount < 2)
            {
                fprintf(stderr, "cd: missing argument\n");
            }
            else
            {
                if (chdir(cmd->arguments[1]) == -1)
                {
                    perror("cd failed");
                }
            }
            freeCmdLines(cmd);
            continue;
        }

        if (strcmp(cmd->arguments[0], "procs") == 0)
        {
            printProcessList(&process_list);
            freeCmdLines(cmd);
            continue;
        }

        if (strcmp(cmd->arguments[0], "hist") == 0)
        {
            printHistory();
            freeCmdLines(cmd);
            continue;
        }

        if (strcmp(cmd->arguments[0], "halt") == 0 ||
            strcmp(cmd->arguments[0], "wakeup") == 0 ||
            strcmp(cmd->arguments[0], "ice") == 0)
        {
            if (cmd->argCount < 2)
            {
                fprintf(stderr, "%s: missing PID argument\n", cmd->arguments[0]);
            }
            else
            {
                int pid = atoi(cmd->arguments[1]);
                int sig;
                if (strcmp(cmd->arguments[0], "halt") == 0)
                {
                    sig = SIGSTOP;
                    updateProcessStatus(process_list, pid, SUSPENDED);
                }
                else if (strcmp(cmd->arguments[0], "wakeup") == 0)
                {
                    sig = SIGCONT;
                    updateProcessStatus(process_list, pid, RUNNING);
                }
                else // ice
                {
                    sig = SIGINT;
                    updateProcessStatus(process_list, pid, TERMINATED);
                }
                if (kill(pid, sig) == -1)
                {
                    perror("kill failed");
                }
            }

            freeCmdLines(cmd);
            continue;
        }

        execute(cmd, debug_mode);
    }

    return 0;
}
