#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include "guestQueue.h"
#include "clientList.c"

#define SIZE 256
#define MID_SIZE 1024
#define BIG_SIZE 4096 

int client_finish_flag = 0;
int server_fd;
int log_fd;

char FIFO_PATH[SIZE];
char FIFO_PATH1[SIZE];
char FIFO_PATH2[SIZE];

int client_ID = 0;

char connected[SIZE];
char log_entry[MID_SIZE];


// Function to clean up FIFO files
void cleanup() {
    unlink(FIFO_PATH);
}

// Function to print error message and exit
void error_exit(char *msg) {
    perror(msg);
    cleanup();
    exit(EXIT_FAILURE);
}

void writeToLogFile(const char *log_entry, int log) {
    if (write(log, log_entry, strlen(log_entry)) == -1) {
        error_exit("Error: Log entry cannot be written\n");
    } 
}

// Function to handle disconnect message from client
void handle_disconnect(int client_pid) {
    printf(">> Client %d disconnected..\n", client_pid);
    fflush(stdout);
    fflush(stdout);
    client_finish_flag++;
}

// Signal handler function
void handle_signal(int signal) {
    // Handle termination signal
    switch(signal) {
        case SIGINT:
            printf("\nReceived Ctrl+C signal. Exiting...\n");
            cleanup();
            exit(EXIT_SUCCESS);
    }
}

char *helpRequest(const char *arg1, const char *arg2) {
    if(strcmp(arg2, "list") == 0){
        return "\nlist\n\nsends a request to display the list of files in Servers directory\n(also displays the list received from the Server)\n\n";
    } else if(strcmp(arg2, "readF") == 0){
        return "\nreadF <file> <line #>\n\nrequests to display the # line of the <file>, if no line number is given\nthe whole contents of the file is requested (and displayed on the client side) \n\n";
    } else if(strcmp(arg2, "writeT") == 0){
        return "\nwriteT <file> <line #> <string>\n\nrequest to write the  content of “string” to the  #th  line the <file>, if the line # is not given\nwrites to the end of file. If the file does not exists in Servers directory creates and edits the\nfile at the same time\n\n";
    } else if(strcmp(arg2, "upload") == 0){
        return "\nupload <file>\n\nuploads the file from the current working directory of client to the Servers directory\n(beware of the cases no file in clients current working directory and file with the same\nname on Servers side)\n\n";
    } else if(strcmp(arg2, "download") == 0){
        return "\ndownload <file>\n\nrequest to receive <file> from Servers directory to client side\n\n";
    } else if(strcmp(arg2, "archServer") == 0){
        return "\narchServer <fileName>.tar\n\nUsing fork, exec and tar utilities create a child process that will collect all the files currently\navailable on the the  Server side and store them in the <filename>.tar archive\n\n";
    } else if(strcmp(arg2, "killServer") == 0){
        return "\nkillServer\n\nSends a kill request to the Server\n\n";
    } else if(strcmp(arg2,"quit") == 0) {
        return "\nquit\n\nSend write request to Server side log file and quits\n\n";
    } else if(strcmp(arg1, "help") == 0){  
        return "\nAvailable comments are :\n\nhelp, list, readF, writeT, upload, download, archServer, quit, killServer\n\n";
    } else {
        return "\n\n\tInvalid command write \"help\" for available commands\n\n";
    }
}

char *listRequest(char *dirname){

    DIR *directory;
    struct dirent *file;

    char *list = malloc(SIZE * sizeof(char)); // Allocate memory dynamically
    if (list == NULL) {
        error_exit("Error: Memory allocation failed\n");
    }

    directory = opendir(dirname); 
    if (directory == NULL) {
        error_exit("Error: Directory couldn't found.");
    }

    while ((file = readdir(directory)) != NULL) {
        if(strcmp(file->d_name,".") !=0 && strcmp(file->d_name,"..") != 0){
            strcat(list,"\n");
            strcat(list,file->d_name);
        }
    }
    strcat(list,"\n\n");
    return list;
}

char *readFRequest(const char* filename, int line_number, char* dirname) {

    FILE* file_ptr;
    char* output = malloc(SIZE * sizeof(char)); // Allocate memory dynamically
    if (output == NULL) {
        error_exit("Error: Memory allocation failed\n");
    }

    char location[SIZE];
    snprintf(location, SIZE, "%s/%s", dirname, filename);

    file_ptr = fopen(location, "r");
    if (file_ptr == NULL) {
        error_exit("Error: File couldn't be opened\n");
    }

    if (line_number == 0) { // Read the whole file
        fseek(file_ptr, 0, SEEK_END); // Move to the end of the file
        long file_size = ftell(file_ptr); // Get the file size
        rewind(file_ptr); // Move the file pointer back to the beginning

        // Allocate memory to hold the entire file
        output = realloc(output, file_size + 1);
        if (output == NULL) {
            error_exit("Error: Memory allocation failed\n");
        }

        // Read the entire file into the output buffer
        fread(output, 1, file_size, file_ptr);
        output[file_size] = '\0'; // Null-terminate the string
    } else { // Read a specific line
        char line[BIG_SIZE];
        int current_line = 0;

        // Read lines until the specified line number
        while (fgets(line, BIG_SIZE, file_ptr) != NULL) {
            current_line++;
            if (current_line == line_number) {
                strcpy(output, line);
                break;
            }
        }

        if (current_line != line_number) { // Line number exceeds the total lines
            snprintf(output, BIG_SIZE, "Error: Line number %d exceeds the total number of lines in the file\n", line_number);
        }
    }

    fclose(file_ptr);
    return output;
}

char* writeTRequest(const char* filename, int line_number, const char* content, char* dirname) {
    FILE* file_ptr;
    char* output = malloc(SIZE * sizeof(char)); // Allocate memory dynamically
    if (output == NULL) {
        error_exit("Error: Memory allocation failed\n");
    }

    char location[SIZE];
    snprintf(location, SIZE, "%s/%s", dirname, filename);

    if (line_number == 0) { // Write to the end of the file
        file_ptr = fopen(location, "a");
        if (file_ptr == NULL) {
            error_exit("Error: File couldn't be opened\n");
        }
        fprintf(file_ptr, "%s\n", content); // Write content to the end of the file
        fclose(file_ptr);
        snprintf(output, SIZE, "Content \"%s\" appended to the file \"%s\"\n", content, filename);
    } else { // Write to a specific line
        file_ptr = fopen(location, "r+");
        if (file_ptr == NULL) {
            error_exit("Error: File couldn't be opened\n");
        }

        char temp[SIZE];
        int current_line = 0;

        // Write content to the specified line
        while (fgets(temp, SIZE, file_ptr) != NULL) {
            current_line++;
            if (current_line == line_number) {
                fseek(file_ptr, -strlen(temp), SEEK_CUR); // Move the file pointer back to the beginning of the line
                fprintf(file_ptr, "%s\n", content); // Overwrite the line with the new content
                break;
            }
        }

        if (current_line != line_number) { // Line number exceeds the total lines
            snprintf(output, SIZE, "Error: Line number %d exceeds the total number of lines in the file\n", line_number);
        } else {
            snprintf(output, SIZE, "Content \"%s\" written to the %dth line of the file \"%s\"\n", content, line_number, filename);
        }

        fclose(file_ptr);
    }

    return output;
}


char* downloadRequest(char *filename, char *dirname) {
    ssize_t bytes_read;
    int total_byte = 0;
    
    char *output = malloc(BIG_SIZE * sizeof(char)); // Allocate memory dynamically
    if (output == NULL) {
        error_exit("Error: Memory allocation failed\n");
    }

    char location[SIZE];

    snprintf(location, SIZE, "%s/%s", dirname, filename);
    int download_fd = open(location, O_RDONLY, 0666);

    if(download_fd == -1){
        error_exit("Error: File couldn't opened");
    }

    int downloaded_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(downloaded_fd == -1){
        error_exit("Error: File couldn't opened");
    }

    bytes_read = read(download_fd, output, BIG_SIZE);
    total_byte += bytes_read;

    ssize_t writed_bytes = write(downloaded_fd, output, bytes_read);
    if(writed_bytes == -1){
        error_exit("Error: Writing operation couldn't be done");
    }
    
    if(bytes_read == -1){
        error_exit("Error: File couldn't read");
    }
    if (close(downloaded_fd) == -1) {
        error_exit("Error: File couldn't closed");
    }
    if (close(download_fd) == -1) {
        error_exit("Error: File couldn't closed");
    }
    snprintf(output, SIZE, "file download request received. Beginning file downloading:\n%d bytes downloaded\n\n", total_byte);
    
    return output;
}

char *uploadRequest(char *filename, char *dirname) {
    ssize_t bytes_read;
    int total_byte = 0;

    char *output = malloc(BIG_SIZE * sizeof(char)); // Allocate memory dynamically
    if (output == NULL) {
        error_exit("Error: Memory allocation failed\n");
    }

    char location[SIZE];

    int upload_fd = open(filename, O_RDONLY, 0666);
    if(upload_fd == -1){
        error_exit("Error: File couldn't opened");
    }

    snprintf(location, SIZE, "%s/%s", dirname, filename);

    int uploaded_fd = open(location, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(uploaded_fd == -1){
        perror("Error happened while opening file");
        exit(EXIT_FAILURE);
    }

    bytes_read = read(upload_fd, output, BIG_SIZE);
    total_byte += bytes_read;

    ssize_t writed_bytes = write(uploaded_fd, output, bytes_read);
    if(writed_bytes == -1){
        perror("Error happened while writing file");
        exit(EXIT_FAILURE);
    }
    
    if(bytes_read == -1){
        perror("Error happened while reading file");
        exit(EXIT_FAILURE);
    }
    if (close(uploaded_fd) == -1) {
        perror("Error happened while closing file");
        exit(EXIT_FAILURE);
    }
    if (close(upload_fd) == -1) {
        perror("Error happened while closing file");
        exit(EXIT_FAILURE);
    }

    snprintf(output, SIZE, "file transfer request received. Beginning file transfer:\n%d bytes transferred\n\n", total_byte);
    
    return output;
}

void archServer(const char* filename, char* dirname) {
    pid_t pid;
    char tar_command[BIG_SIZE];

    snprintf(tar_command, BIG_SIZE, "tar -cf %s/%s %s/*", dirname, filename, dirname);

    pid = fork();
    if (pid == -1) {
        perror("Error: Fork failed\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        execl("/bin/sh", "sh", "-c", tar_command, (char*)NULL);
        perror("Error: Exec failed\n");
        exit(EXIT_FAILURE);
    } else { // Parent process
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("Error: Wait failed\n");
            exit(EXIT_FAILURE);
        }
        if (WIFEXITED(status)) {
            printf("Child process exited with status %d\n", WEXITSTATUS(status));
        }
    }
}

void doConnectedClientRequests(int client_pid, Node *head, char *dirname) {

    char argument[SIZE];
    char arg1[SIZE];
    char arg2[SIZE];
    char arg3[SIZE];
    char arg4[SIZE];
    char request[BIG_SIZE];

    client_ID++;   
     
    pid_t pid;
    if ((pid = fork()) == -1) {
        error_exit("Error: forking child process\n");
    } else if (pid == 0) { 
        snprintf(FIFO_PATH1, SIZE,"/tmp/FIFO_client_read_%d", client_pid);
        snprintf(FIFO_PATH2, SIZE,"/tmp/FIFO_client_write_%d", client_pid);

        int client_fd_read; 
        // Open client read fifo
        if ((client_fd_read = open(FIFO_PATH1, O_WRONLY)) == -1) {
            error_exit("Error: Client read fifo couldn't opened\n");
        }

        printf(">> Client PID %d connected as 'client0%d'\n", client_pid, client_ID);
        fflush(stdout);
        snprintf(log_entry, MID_SIZE, "Client PID %d connected as 'client0%d'\n", client_pid, client_ID);
        writeToLogFile(log_entry, log_fd);

        if (write(client_fd_read, "ok", 2) == -1) {
            error_exit("Error: Ok massage couldn't send\n");
        }

        close(client_fd_read);

        while (1){
            // Open client write fifo
            if(sizeOfList(head) == 0) {
                break;
            }
            int client_fd_write; 
            if ((client_fd_write = open(FIFO_PATH2, O_RDONLY)) == -1) {
                error_exit("Error: Client write fifo couldn't opened\n");
            }
            if (read(client_fd_write, &argument, sizeof(argument)) == -1) {
                error_exit("Error: Request couldn't obtain\n");
            } else {
                close(client_fd_write);

                sscanf(argument, "%s %s %s %s", arg1, arg2, arg3, arg4); 
                
                int client_fd_read;
                if ((client_fd_read = open(FIFO_PATH1, O_WRONLY)) == -1) {
                    error_exit("Error: Client read fifo couldn't opened\n");
                }

                if(strcmp(arg1, "help") == 0) {
                    strcpy(request,helpRequest(arg1,arg2));
                    write(client_fd_read, &request, strlen(request) + 1);
                    snprintf(log_entry, MID_SIZE, "Client PID %d requested '%s'\n", client_pid, arg1);
                    writeToLogFile(log_entry, log_fd);
                    memset(log_entry, 0, sizeof(log_entry));
                    memset(request, 0, sizeof(request));
                    close(client_fd_read);
                    continue;
                } else if(strcmp(arg1, "list") == 0) {
                    strcpy(request,listRequest(dirname));
                    write(client_fd_read, request, strlen(request) + 1);
                    snprintf(log_entry, MID_SIZE, "Client PID %d requested '%s'\n", client_pid, arg1);
                    writeToLogFile(log_entry, log_fd);
                    memset(log_entry, 0, sizeof(log_entry));
                    memset(request, 0, sizeof(request));
                    close(client_fd_read);
                    continue;                 
                } else if(strcmp(arg1, "readF") == 0) {
                    int line_number = atoi(arg3);
                    strcpy(request,readFRequest(arg2, line_number, dirname));
                    write(client_fd_read, request, strlen(request) + 1);
                    snprintf(log_entry, MID_SIZE, "Client PID %d requested '%s'\n", client_pid, arg1);
                    writeToLogFile(log_entry, log_fd);
                    memset(log_entry, 0, sizeof(log_entry));
                    memset(request, 0, sizeof(request));
                    close(client_fd_read);
                    continue;  
                } else if(strcmp(arg1, "writeT") == 0) {
                    int line_number = atoi(arg3);
                    strcpy(request,writeTRequest(arg2, line_number, arg4, dirname));
                    write(client_fd_read, request, strlen(request) + 1);
                    snprintf(log_entry, MID_SIZE, "Client PID %d requested '%s'\n", client_pid, arg1);
                    writeToLogFile(log_entry, log_fd);
                    memset(log_entry, 0, sizeof(log_entry));
                    memset(request, 0, sizeof(request));
                    close(client_fd_read);
                    continue;  
                } else if(strcmp(arg1, "upload") == 0) {
                    strcpy(request, uploadRequest(arg2, dirname));
                    write(client_fd_read, request, strlen(request) + 1);
                    snprintf(log_entry, MID_SIZE, "Client PID %d requested '%s'\n", client_pid, arg1);
                    writeToLogFile(log_entry, log_fd);
                    memset(log_entry, 0, sizeof(log_entry));
                    memset(request, 0, sizeof(request));
                    close(client_fd_read);
                    continue; 
                } else if(strcmp(arg1, "download") == 0) {
                    strcpy(request, downloadRequest(arg2, dirname));
                    write(client_fd_read, request, strlen(request) + 1);
                    snprintf(log_entry, MID_SIZE, "Client PID %d requested '%s'\n", client_pid, arg1);
                    writeToLogFile(log_entry, log_fd);
                    memset(log_entry, 0, sizeof(log_entry));
                    memset(request, 0, sizeof(request));
                    close(client_fd_read);
                    continue; 
                } else if(strcmp(arg1, "archServer") == 0) {
                    strcpy(request, "tar file created");
                    archServer(arg2, dirname);
                    write(client_fd_read, request, strlen(request) + 1);
                    snprintf(log_entry, MID_SIZE, "Client PID %d requested '%s'\n", client_pid, arg1);
                    writeToLogFile(log_entry, log_fd);
                    memset(log_entry, 0, sizeof(log_entry));
                    memset(request, 0, sizeof(request));
                    close(client_fd_read);
                    continue; 
                } else if(strcmp(arg1, "quit") == 0) {
                    snprintf(log_entry, MID_SIZE, "Client PID %d requested '%s'\n", client_pid, arg1);
                    writeToLogFile(log_entry, log_fd);
                    memset(log_entry, 0, sizeof(log_entry));
                    memset(arg1, 0, sizeof(arg1));
                    removePid(&head, client_pid);
                    close(client_fd_read);
                    continue;
                } else if(strcmp(arg1, "killServer") == 0) {
                    snprintf(log_entry, MID_SIZE, "Client PID %d requested '%s'\n", client_pid, arg1);
                    writeToLogFile(log_entry, log_fd);
                    memset(log_entry, 0, sizeof(log_entry));
                    memset(arg1, 0, sizeof(arg1));
                    close(client_fd_read);
                    printf(">> Kill signal from client %d.. terminating...\n>> bye\n", client_pid);
                    kill(getppid(), SIGINT);
                    exit(EXIT_SUCCESS);
                } else {
                    printf("Error: Invalid request. Type 'help' for display request types.\n");
                }
            }    
        }
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[]) {

    Queue* queue_guest = createQueue();
    Node* head = NULL;

    // Set up signal handler
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    // Parse command line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <dirname> <max. #ofClients>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *dirname = argv[1];
    int max_clients = atoi(argv[2]);
    
    // Create directory if it doesn't exist
    if (mkdir(dirname, 0777) == -1 && errno != EEXIST) {
        error_exit("Error: Directory couldn't created\n");
    }

    // Create log file for clients
    if ((log_fd = open("server.log", O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) { 
        error_exit("Error: Log file couldn't opened\n");
    }

    // Display server PID
    printf(">> Server Started PID %d...\n", getpid());
    printf(">> waiting for clients...\n");

    // Obtain server fifo name
    snprintf(FIFO_PATH, SIZE, "/tmp/FIFO_server_%d", getpid());

    // Create named FIFO for communication with clients
    if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST) {
        error_exit("Error: Server fifo couldnt't created\n");
    }

    // Wait for clients to connect
    // Accept client connections
    while (1) { 
        int client_pid;
        char connect_option[SIZE];
        
        if (client_finish_flag > 0 && !isEmpty(queue_guest)) {
            client_finish_flag--;
            client_pid = dequeue(queue_guest);
            doConnectedClientRequests(client_pid, head, dirname);
        }

        // Open named pipe for reading
        if ((server_fd = open(FIFO_PATH, O_RDONLY)) == -1) {
            error_exit("Error: Server fifo couldn't opened\n");
        }

        if (read(server_fd, &connected, sizeof(connected)) == -1) {
            error_exit("Error: Data couldn't achieved\n");
        } else {
            sscanf(connected,"%s %d", connect_option, &client_pid);

            if (strcmp(connect_option, "Disconnect") == 0) {
                handle_disconnect(client_pid);
                removePid(&head, client_pid);
                sleep(1);
                continue;
            }

            addPid(&head, client_pid);

            if (sizeOfList(head) > max_clients) {
                if (strcmp(connect_option, "Connect") == 0) {
                    enqueue(queue_guest, client_pid);
                }

                removePid(&head, client_pid);
                printf(">> Connection request PID %d…  Que FULL\n", client_pid);
                fflush(stdout);
                snprintf(log_entry, SIZE, "Connection request PID %d... Que FULL\n", client_pid);
                writeToLogFile(log_entry, log_fd);
                if (strcmp(connect_option, "tryConnect") == 0) {
                    snprintf(FIFO_PATH1, SIZE,"/tmp/FIFO_client_read_%d", client_pid);
                    // Open client read fifo
                    int client_fd_read; 
                    if ((client_fd_read = open(FIFO_PATH1, O_WRONLY)) == -1) {
                        error_exit("Error: Client read fifo couldn't opened\n");
                    }
                    if (write(client_fd_read, "no", 2) == -1) {
                            error_exit("Error: Ok massage cannot be sent\n");
                    }
                }    
            } else {
                doConnectedClientRequests(client_pid, head, dirname);
                continue;
            }    
        }    
    }

    // Close the file
    if (close(log_fd) == -1) {
        error_exit("Error: Log file cannot be closed\n");
    }

    // Close named pipe (FIFO)
    close(server_fd);
    cleanup();

    return 0;
}
