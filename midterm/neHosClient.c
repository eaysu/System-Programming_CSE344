#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define SIZE 256
#define MID_SIZE 1024
#define BIG_SIZE 4096 

char FIFO_PATH[SIZE];
char FIFO_PATH1[SIZE];
char FIFO_PATH2[SIZE];

int connected_flag = 0;

int server_fd;

// Function to clean up FIFO files
void cleanup() {
    unlink(FIFO_PATH1);
    unlink(FIFO_PATH2);
}

// Function to print error message and exit
void error_exit(char *msg) {
    perror(msg);
    cleanup();
    exit(EXIT_FAILURE);
}

// Signal handler function
void handle_signal(int signal) {
    // Handle termination signal
    switch(signal) {
        case SIGINT:
            printf("\nReceived Ctrl+C signal. Exiting...\n");
            if (connected_flag) {
                if ((server_fd = open(FIFO_PATH, O_WRONLY)) == -1) {
                    error_exit("Error: Server fifo couldn't opened\n");
                }
                // Send a message to the server indicating the client is disconnecting
                char disconnect_msg[SIZE];
                sprintf(disconnect_msg, "Disconnect %d", getpid());
                if (write(server_fd, disconnect_msg, strlen(disconnect_msg) + 1) == -1) {
                    error_exit("Error: Data cannot be sent\n");
                }
            }
            cleanup();
            exit(EXIT_SUCCESS);    
            break;
        // Add handling for other signals if needed
    }
}


// Main function
int main(int argc, char *argv[]) {

    char connect[SIZE];
    char arg1[SIZE];
    char arg2[SIZE];
    char arg3[SIZE];
    char arg4[SIZE];
    char sent_answer[MID_SIZE];
    char requestAnswer[BIG_SIZE];

    // Set up signal handler
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // Parse command-line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Connect/tryConnect> <ServerPID>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *connect_option = argv[1];
    int server_pid = atoi(argv[2]);

    //Obtain server fifo name
    snprintf(FIFO_PATH, SIZE, "/tmp/FIFO_server_%d", server_pid);
    // Obtain client fifo names
    snprintf(FIFO_PATH1, SIZE, "/tmp/FIFO_client_read_%d", getpid());
    snprintf(FIFO_PATH2, SIZE, "/tmp/FIFO_client_write_%d", getpid()); 

    // Create named FIFO for communication with server
    if (mkfifo(FIFO_PATH1, 0666) == -1 && errno != EEXIST) {
        error_exit("Error: Client read fifo couldn't create\n");
        exit(EXIT_FAILURE);
    }

    if (mkfifo(FIFO_PATH2, 0666) == -1 && errno != EEXIST) {
        error_exit("Error: Client write fifo couldn't create\n");
        exit(EXIT_FAILURE);
    }
    // Connect to the server
    if (strcmp(connect_option, "Connect") == 0) {
        // Send connection request to server
        printf("> NehosClient connect %d...\n", server_pid);

        // Implement logic to send connection request to server and wait for response 
        if ((server_fd = open(FIFO_PATH, O_WRONLY)) == -1) {
            error_exit("Error: Server fifo couldn't opened\n");
        }

        sprintf(connect, "Connect %d", getpid());
        fflush(stdout);

        if (write(server_fd, connect, strlen(connect) + 1) == -1) {
            error_exit("Error: Data couldn't send\n");
        }
        memset(connect, 0, sizeof(connect));

        int client_fd_read;
        // Open client read fifo
        if ((client_fd_read = open(FIFO_PATH1, O_RDONLY)) == -1) {
            error_exit("Error: Client read fifo couldn't open\n");
        }

        printf("    >> Waiting for Que.. ");
        if (read(client_fd_read, connect, SIZE) == -1) {
            error_exit("Error: Client couldn't connect to server\n");
        } else if (strcmp(connect, "ok") == 0) {
            printf("Connection established:\n");
            connected_flag = 1;
        }

        close(client_fd_read);
        do {
            printf("    >> Enter comment : ");
            fflush(stdout);
            // Use read system call to read from standard input
            ssize_t bytesRead = read(0, connect, sizeof(connect));

            // Check if read was successful
            if (bytesRead < 0) {
                printf("Error: Input couldn't read\n");
                return 1;
            }
            // Null-terminate the string
            connect[bytesRead] = '\0';      
            int numArgs = sscanf(connect, "%s %s %s %s", arg1, arg2, arg3, arg4);
            
            int client_fd_write;
            // Open client write fifo
            if ((client_fd_write = open(FIFO_PATH2, O_WRONLY)) == -1) {
                error_exit("Error: Client write fifo couldn't open\n");
            }
            if (numArgs == 1) {
                if (write(client_fd_write, arg1, sizeof(arg1)) == -1) {
                    error_exit("Error: Client couldn't write the command to client write fifo\n");
                }      
            } else if (numArgs == 2) {
                sprintf(sent_answer, "%s %s", arg1, arg2);
                if (write(client_fd_write, sent_answer, sizeof(sent_answer)) == -1) {
                    error_exit("Error: Client couldn't write the command to client write fifo\n");
                }
            } else if (numArgs == 3) {
                sprintf(sent_answer, "%s %s %s", arg1, arg2, arg3);
                if (write(client_fd_write, sent_answer, sizeof(sent_answer)) == -1) {
                    error_exit("Error: Client couldn't write the command to client write fifo\n");
                }
            }  else if (numArgs == 4) {
                sprintf(sent_answer, "%s %s %s %s", arg1, arg2, arg3, arg4);
                if (write(client_fd_write, sent_answer, sizeof(sent_answer)) == -1) {
                    error_exit("Error: Client couldn't write the command to client write fifo\n");
                }
            } else {
                printf("Invalid input format\n");
            }     
            // Open client read fifo
            if ((client_fd_read = open(FIFO_PATH1, O_RDONLY)) == -1) {
                error_exit("Error: Client read fifo cannot be opened\n");
            }

            if (read(client_fd_read, requestAnswer, sizeof(requestAnswer)) == -1) {
                error_exit("Error: Client couldn't read the request answer from server\n");
            }
            if (strcmp(requestAnswer, "quit") == 0) {
                break;
            }
            printf("%s", requestAnswer);
            memset(requestAnswer, 0, BIG_SIZE);

        } while ((strcmp(arg1, "quit") != 0) && (strcmp(arg1, "killServer") != 0));
        cleanup();
        printf("\tSending write request to server log file\n\twaiting for logfile...\n\tlogfile write request granted\n\tbye..");

    } else if (strcmp(connect_option, "tryConnect") == 0) {
        // Send connection request to server
        printf("> NehosClient tryConnect %d...\n", server_pid);

        // Implement logic to send connection request to server and wait for response
        if ((server_fd = open(FIFO_PATH, O_WRONLY)) == -1) {
            error_exit("Error: Server fifo cannot be opened\n");
        }

        sprintf(connect, "tryConnect %d", getpid());
        fflush(stdout);

        if (write(server_fd, connect, strlen(connect) + 1) == -1) {
            error_exit("Error: Data cannot be sent\n");
        }
        memset(connect, 0, sizeof(connect));

        int client_fd_read;
        // Open client read fifo
        if ((client_fd_read = open(FIFO_PATH1, O_RDONLY)) == -1) {
            error_exit("Error: Client read fifo cannot be opened\n");
        }

        printf("    >> Waiting for Que.. ");
        if (read(client_fd_read, connect, SIZE) == -1) {
            error_exit("Error: Client couldn't connected to server\n");
        } else if (strcmp(connect, "ok") == 0) {
            printf("Connection established:\n");
            connected_flag = 1;
        } else {
            close(client_fd_read);
            error_exit("Error: Client que is full... terminating...\n");
        }
        close(client_fd_read);
        do {
            printf("    >> Enter comment : ");
            fflush(stdout);
            // Use read system call to read from standard input
            ssize_t bytesRead = read(0, connect, sizeof(connect));

            // Check if read was successful
            if (bytesRead < 0) {
                printf("Error: Input couldn't read\n");
                return 1;
            }
            // Null-terminate the string
            connect[bytesRead] = '\0';      
            int numArgs = sscanf(connect, "%s %s %s %s", arg1, arg2, arg3, arg4);
            
            int client_fd_write;
            // Open client write fifo
            if ((client_fd_write = open(FIFO_PATH2, O_WRONLY)) == -1) {
                error_exit("Error: Client write fifo couldn't open\n");
            }
            if (numArgs == 1) {
                if (write(client_fd_write, arg1, sizeof(arg1)) == -1) {
                    error_exit("Error: Client couldn't write the command to client write fifo\n");
                }      
            } else if (numArgs == 2) {
                sprintf(sent_answer, "%s %s", arg1, arg2);
                if (write(client_fd_write, sent_answer, sizeof(sent_answer)) == -1) {
                    error_exit("Error: Client couldn't write the command to client write fifo\n");
                }
            } else if (numArgs == 3) {
                sprintf(sent_answer, "%s %s %s", arg1, arg2, arg3);
                if (write(client_fd_write, sent_answer, sizeof(sent_answer)) == -1) {
                    error_exit("Error: Client couldn't write the command to client write fifo\n");
                }
            }  else if (numArgs == 4) {
                sprintf(sent_answer, "%s %s %s %s", arg1, arg2, arg3, arg4);
                if (write(client_fd_write, sent_answer, sizeof(sent_answer)) == -1) {
                    error_exit("Error: Client couldn't write the command to client write fifo\n");
                }
            } else {
                printf("Invalid input format\n");
            }       
            // Open client read fifo
            if ((client_fd_read = open(FIFO_PATH1, O_RDONLY)) == -1) {
                error_exit("Error: Client read fifo cannot be opened\n");
            }

            if (read(client_fd_read, requestAnswer, sizeof(requestAnswer)) == -1) {
                error_exit("Error: Client couldn't read the request answer from server\n");
            }
            if (strcmp(requestAnswer, "quit") == 0) {
                break;
            }
            printf("%s", requestAnswer);
            memset(requestAnswer, 0, BIG_SIZE);
        } while ((strcmp(arg1, "quit") != 0) && (strcmp(arg1, "killServer") != 0));
        cleanup();
        printf("\tSending write request to server log file\n\twaiting for logfile...\n\tlogfile write request granted\n\tbye..");
        
    } else {
        fprintf(stderr, "Invalid connection option: %s\n", connect_option);
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}