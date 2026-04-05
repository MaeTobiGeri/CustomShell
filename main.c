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
        kill(current_child, SIGINT);
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
    printf("\n\n\n\t****Custom Shell****");
    printf("\n\n\t-USE AT YOUR OWN RISK-");
    printf("\n\n\n\n*******************"
        "***********************");
    printf("\n");
    sleep(1);
    clear();
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
        buf = readline("\nmsh> ");
        if (!buf) break;

        if(strlen(buf) > 0) {

            int i = 0;
            pid_t pid;
            char* command[MAXCOM]; 
            char fixed[MAXLET];
            int j = 0;

            for (int i = 0; buf[i] != '\0' && j < MAXLET - 1; i++) {
                if (buf[i] == '>') {
                    fixed[j++] = ' ';
                    fixed[j++] = '>';
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
                    
                    

                    int redirect_index = -1;

                    for (int j = 0; command[j] != NULL; j++) {
                        if (strcmp(command[j], ">") == 0) {
                            redirect_index = j;
                            perror("r1");
                            exit(1);
                        }

                        dup2(workingFile, STDOUT_FILENO);
                        close(workingFile);
                        command[redirect_index] = NULL;
                    }

                    
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