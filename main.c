#include <stdbool.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <fcntl.h>

#define clear() printf("\033[H\033[J")

#define MAXLET 1000 //Maximum Number of Letters
#define MAXCOM 100 //Maximum Number of Commands

volatile sig_atomic_t current_child = -1;
volatile sig_atomic_t sigint_received = 0;

void handle_sigint(int sig) {
    if (current_child > 0) {
        kill(-current_child, SIGINT);  // <- FIX: negative PGID kills whole pipeline
    } else {
        write(STDOUT_FILENO, "\n", 1);
        rl_on_new_line();
        rl_replace_line("", 0);
        rl_redisplay();   
    }
}

void init_shell()
{
    clear();
    printf("\n\n\n\n******************"
        "************************");
    printf("\n\n\n\t****Yet Another Shell****");
    printf("\n\n\t-USE AT YOUR OWN RISK-");
    printf("\n\n\n\n*******************"
        "***********************");
    printf("\n");
    sleep(1);
    clear();
}

int letter_Occurance(char** command ,char* searchToken)
{
    int index = -1;

    for (int j = 0; command[j] != NULL; j++) {
        if (strcmp(command[j], searchToken) == 0) {
            index = j;
            break;
        }
    }
    return index;
}

void handle_redirection(char** command)
{
    int redirect_index = letter_Occurance(command ,">");
    
    if (redirect_index != -1) {
        int sz;
        if (command[redirect_index + 1] == NULL) 
        {
            printf("Yash> syntax error near unexpected token newline");
            exit(EXIT_FAILURE);   
        }

        int workingFile = open(command[redirect_index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (workingFile < 0) {
            perror("r1");
            exit(1);
        }

        dup2(workingFile, STDOUT_FILENO);
        close(workingFile);
        command[redirect_index] = NULL;
    } 
}

void handle_pipe(char** command)
{
    int cmd_start[MAXCOM];
    int cmd_count = 0;

    cmd_start[cmd_count++] = 0;

    for (int i = 0; command[i] != NULL; i++) {
        if (strcmp(command[i], "|") == 0) {
            command[i] = NULL; 
            cmd_start[cmd_count++] = i + 1;
        }
    }

    int prev_fd = -1;
    pid_t pgid = 0;

    for (int i = 0; i < cmd_count; i++) {
        int pipe_fds[2];

        if (i < cmd_count - 1) {
            if (pipe(pipe_fds) < 0) {
                perror("pipe");
                exit(1);
            }
        }

        int pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            if (pgid == 0) pgid = getpid();
            setpgid(0, pgid);

            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (i < cmd_count - 1) {
                dup2(pipe_fds[1], STDOUT_FILENO);
                close(pipe_fds[0]);
                close(pipe_fds[1]);
            }

            char** cmd = &command[cmd_start[i]];
            handle_redirection(cmd);
            execvp(cmd[0], cmd);
            perror("execvp");
            exit(1);
        }

        if (pgid == 0) pgid = pid;
        setpgid(pid, pgid);

        if (prev_fd != -1) {
            close(prev_fd);
        }
        if (i < cmd_count - 1) {
            close(pipe_fds[1]);
            prev_fd = pipe_fds[0];
        }
    }

    current_child = pgid;

    for (int i = 0; i < cmd_count; i++) {
        wait(NULL);
    }
    if (prev_fd != -1)
    {
        close(prev_fd);
    }
    current_child = -1;
}

int main() {
    char* buf;

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    init_shell();

    while (1) {
        buf = readline("\nYash> ");
        if (!buf) break;

        if(strlen(buf) > 0) {

            int i = 0;
            pid_t pid;
            char* command[MAXCOM]; 
            char fixed[MAXLET];
            int j = 0;

            for (int i = 0; buf[i] != '\0' && j < MAXLET - 4; i++) {
                if (buf[i] == '>' || buf[i] == '|') {
                    fixed[j++] = ' ';
                    fixed[j++] = buf[i];
                    fixed[j++] = ' ';
                } else {
                    fixed[j++] = buf[i];
                }

            }
            fixed[j] = '\0';
            
            char* token = strtok(fixed, " ");
            
            while (token != NULL && i < MAXCOM - 1) {
                command[i++] = token;
                token = strtok(NULL, " ");
            }
            command[i] = NULL;

            if (command[0] != NULL && strcmp(command[0], "exit") == 0) {
                exit(0);
                
            }
            else if (command[0] != NULL && strcmp(command[0], "cd") == 0) {
                if (command[1] == NULL) {
                    
                    chdir(getenv("HOME"));
                }else {
                    chdir(command[1]);
                }
                
            } 
            else if (letter_Occurance(command, "|") != -1) {
                handle_pipe(command);
            } else {

            pid = fork();

            switch (pid) {
            case -1:
                    perror("fork");
                    exit(EXIT_FAILURE);
            case 0:
                    struct sigaction sa;
                    sa.sa_handler = SIG_DFL;
                    sigemptyset(&sa.sa_mask);
                    sa.sa_flags = 0;
                    sigaction(SIGINT, &sa, NULL);
                    fflush(stdout);
                    
                    handle_redirection(command); 
                    execvp(command[0], command);
                    perror("execvp"); 
                    exit(EXIT_FAILURE);
            default:
                    current_child = pid;
                    waitpid(pid, NULL, 0);
                    current_child = -1;
                    break;
            }
        }
        }
        free(buf);

    }
    return 0;
}