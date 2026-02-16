# Custom Linux Shell Implementation

A fully functional Linux Shell developed in C, demonstrating core Operating Systems concepts including process management, system calls, inter-process communication (IPC), and signal handling. This project was developed as part of an academic course (Lab 2 and Lab C).

## Key Features

* **Command Execution:** Supports standard Unix commands using fork(), execvp(), and waitpid().
* **Pipeline Support:** Implements complex pipelines (e.g., ls -l | tail -n 5) using the pipe() system call and file descriptor redirection (dup).
* **I/O Redirection:** Supports input (<) and output (>) redirection for commands.
* **Process Manager (Job Control):** A built-in system to track and manage child processes using a linked list.
    * procs: List all running, suspended, and recently terminated processes.
    * halt <pid>: Suspends a running process (sends SIGSTOP).
    * wakeup <pid>: Resumes a suspended process (sends SIGCONT).
    * ice <pid>: Terminates a process (sends SIGINT).
* **Command History:** Maintains an internal history of the last 20 commands.
    * hist: Displays the command history.
    * !!: Re-executes the last command.
    * !n: Re-executes the n-th command from the history list.
* **Modular Design:** Built to be extensible, handling both foreground and background (&) execution.

## Components

### 1. myshell.c
The main shell engine. It handles the infinite user-input loop, parsing via LineParser, and the execution logic for both internal commands (cd, history, procs) and external programs.

### 2. mypipeline.c
A standalone implementation of a 2-process pipeline (equivalent to "ls -ls | wc"). This serves as a demonstration of mastering synchronization and file descriptor redirection before integrating it into the main shell.

### 3. looper.c
A utility program for testing signal handling. It intercepts and reports SIGINT, SIGTSTP, and SIGCONT, used to verify the shell's process management capabilities.

## Technical Skills Demonstrated

* **Low-Level C Programming:** Manual memory management and dynamic data structures (Linked Lists).
* **Unix System Programming:** Direct use of fork, exec, waitpid, pipe, dup, and kill system calls.
* **Signal Handling:** Implementing custom signal handlers to manage process states.
* **File Descriptor Management:** Manipulating standard input, output, and error streams for redirection and piping.

## How to Build and Run

1. Clone the repository:
   git clone https://github.com/your-username/your-repo-name.git
   cd your-repo-name

2. Compile the project:
   The provided Makefile compiles both myshell and mypipeline.
   make

3. Run the shell:
   ./myshell

## Requirements
* Linux/Unix environment.
* gcc compiler.
* make utility.
