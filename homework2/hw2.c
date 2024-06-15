#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#define FIFO1 "/tmp/fifo1"
#define FIFO2 "/tmp/fifo2"
#define NUM_CHILDREN 2

int counter = 0;

// Function to clean up FIFO files
void cleanup() {
    unlink(FIFO1);
    unlink(FIFO2);
}

// Function to print error message and exit
void error_exit(char *msg) {
    perror(msg);
    cleanup();
    exit(EXIT_FAILURE);
}

// Function to send random numbers array to a FIFO
void send_random_numbers(int fd, int *array, int num) {
    if (write(fd, array, num * sizeof(int)) == -1) {
        error_exit("Error writing random numbers to FIFO");
    } else {
        printf("Random numbers is sent to %d\n", fd);
    }   
}

// Function to send a multiply command to a FIFO2
void send_command(int fd, const char *command) {
    if (write(fd, command, strlen(command) + 1) == -1) {
        error_exit("Error writing command to FIFO");
    } else {
        printf("Multiply command is sent to %d\n", fd);
    }   
}

// Child process 1 function
void child_process_1() {
    printf("Child process 1 started\n");
    // Sleeps 10 seconds as expected
    sleep(10); 

    // Open FIFO1 for reading
    int fd1;
    if ((fd1 = open(FIFO1, O_RDONLY)) == -1)
        error_exit("Child Process 1: Error opening FIFO1");

    int sum = 0;
    int num;

    // Read random numbers from FIFO1 and calculate sum
    while (read(fd1, &num, sizeof(int)) > 0) {
        sum += num;
    }
    close(fd1);

    // Open FIFO2 for writing
    int fd2;
    if ((fd2 = open(FIFO2, O_WRONLY)) == -1) {
        error_exit("Child Process 1: Error opening FIFO2");
    }    

    // Write sum to FIFO2
    if (write(fd2, &sum, sizeof(int)) == -1) {
        error_exit("Child Process 1: Error writing to FIFO2");
    } else {
        printf("Summation result is written into FIFO2\n");
    }   

    close(fd2);
    printf("Child process 1 completed\n");
    exit(EXIT_SUCCESS);
}

// Child process 2 function
void child_process_2(int size) {
    printf("Child process 2 started\n");
    // Sleeps 10 seconds as expected
    sleep(10); 

    // Open FIFO2 for reading
    int fd2;
    if ((fd2 = open(FIFO2, O_RDONLY)) == -1) {
        error_exit("Child Process 2: Error opening FIFO2");
    }    

    // Wait for 2 seconds to ensure data availability in FIFO2
    sleep(2);
    char command[9];

    // Read command from FIFO2
    if (read(fd2, command, sizeof(command)) == -1) {
        error_exit("Child Process 2: Error reading from FIFO2");
    }    

    command[8] = '\0';
    printf("Command: %s\n", command);

    // Check command validity
    if (strcmp(command, "multiply") != 0) {
        printf("Child process 2: Invalid command received\n");
        exit(EXIT_FAILURE);
    }

    int result = 1;
    int count = 0;
    int num, sum = 0;

    // Read random numbers from FIFO2 and perform multiplication
    while (read(fd2, &num, sizeof(int)) > 0) {
        if (count == size) {
            sum = num;
            break;
        }
        result *= num;
        count += 1;
    } 

    close(fd2);

    // Printing summation and multiplication results
    printf("Summation result: %d\n", sum);
    printf("Multiplication result: %d\n", result);
    printf("Child process 2 completed\n");
    exit(EXIT_SUCCESS);
}

// Signal handler function for SIGCHLD
void sigchld_handler() {
    pid_t pid;
    int status;
    // Reap terminated child processes
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
        }
        counter++;
    }
    // Check for errors in waitpid
    if (pid == -1 && errno != ECHILD) {
        perror("Error in waitpid");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {

    // Take random number array size as argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <integer>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Size should be greater than 0
    if (atoi(argv[1]) <= 0) {
        printf("size of the array should be greater than 0\n");
        exit(EXIT_FAILURE);
    }

    // Initializations
    int size = atoi(argv[1]);
    int array[size];
    int fd1, fd2;
    srand(time(NULL));

    // Set up SIGCHLD signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &sigchld_handler;
    sigaction(SIGCHLD, &sa, NULL);

    // Creating 2 FIFOs
    if (mkfifo(FIFO1, 0666) == -1 || mkfifo(FIFO2, 0666) == -1) {
        error_exit("Error creating FIFOs\n");
    } else {
        printf("FIFOs created successfully\n");
    }

    // Creating random number array with given size
    printf("Numbers: ");
    for (int i = 0; i < size; i++) {
        // I take mod 10 for avoiding extraordinary operation results
        array[i] = rand() % 10; 
        printf("%d ", array[i]);
    }  
    printf("\n"); 

    // Creating 2 child processes to execute specific operations
    pid_t pid;
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if ((pid = fork()) == -1)
            error_exit("Error forking child process\n");
        else if (pid == 0) {
            if (i == 0) {
                printf("Child process 1 created successfully\n");
                child_process_1();
            } else {
                printf("Child process 2 created successfully\n");
                child_process_2(size);
            }    
            // Exit the loop after forking a child process
            break;
        }
    }

    // Creating 2 FIFOs which contains random numbers 
    if ((fd1 = open(FIFO1, O_WRONLY)) == -1 || (fd2 = open(FIFO2, O_WRONLY)) == -1) {
        perror("Error opening FIFOs for writing");
        exit(EXIT_FAILURE);
    } else {
        printf("FIFOs opened successfully\n");
    }

    send_random_numbers(fd1, array, size);
    send_command(fd2, "multiply");
    send_random_numbers(fd2, array, size);    

    // After writing operation FIFOs are closed
    close(fd1);
    close(fd2);

    // Proceeds for every two seconds until both child processes are finished
    while (counter < NUM_CHILDREN) {
        printf("Proceeding...\n");
        sleep(2);
    }

    // Unlink FIFOs, prints positive message and terminates program
    cleanup();
    printf("Parent process completed successfully\n");
    return 0;
}
