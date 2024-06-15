#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 100

// Function to write log entry to the log file
void writeLogEntry(const char *task) {
    pid_t pid = fork(); // Creating child process
    if (pid < 0) {
        printf("Error: Fork failed.\n");
    } else if (pid == 0) {
        int logFile = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        if (logFile == -1) {
            printf("Error opening log file.\n");
            return;
        }

        // Get current time
        time_t currentTime;
        struct tm *localTime;
        time(&currentTime);
        localTime = localtime(&currentTime);

        // Format log entry with current time and task
        char logEntry[MAX_LINE_LENGTH];
        sprintf(logEntry, "[%02d/%02d/%04d %02d:%02d:%02d] Task completed: %s\n",
                localTime->tm_mday, localTime->tm_mon + 1, localTime->tm_year + 1900,
                localTime->tm_hour, localTime->tm_min, localTime->tm_sec, task);

        // Write log entry to the log file
        write(logFile, logEntry, strlen(logEntry));

        // Close the log file
        close(logFile);
        exit(EXIT_SUCCESS);
    } else {
        int status; // Parent process
        waitpid(pid, &status, 0); // Wait for child process to finish
        if (!WIFEXITED(status)) {
            // Child process terminated abnormally
            printf("Error: Child process terminated abnormally.\n");
        }
    }  
}

// Function to display usage instructions
void displayUsage() {
    pid_t pid = fork(); 
    if (pid < 0) {
        printf("Error: Fork failed.\n");
    } else if (pid == 0) {
        printf("Usage: gtuStudentGrades [command] [arguments]\n");
        printf("Available Commands:\n");
        printf("  gtuStudentGrades \"grades.txt\": Create grades.txt file.\n");
        printf("  addStudentGrade \"Name Surname\" \"Grade\" \"grades.txt\": Add a new student's grade.\n");
        printf("  searchStudent \"Name Surname\" \"grades.txt\": Search for a student's grade.\n");
        printf("  sortAll \"grades.txt\": Sort student grades by name.\n");
        printf("  showAll \"grades.txt\": Display all student grades.\n");
        printf("  listGrades \"grades.txt\": Display the first five student grades.\n");
        printf("  listSome \"numEntries\" \"pageNumber\" \"grades.txt\": Display student grades based on page number and number of entries per page.\n");
        printf("  gtuStudentGrades: Display usage instructions.\n");
        printf("  exit: Terminates program.\n");
        exit(EXIT_SUCCESS);
    } else {
        int status; // Parent process
        waitpid(pid, &status, 0); // Wait for child process to finish
        if (!WIFEXITED(status)) {
            printf("Error: Child process terminated abnormally.\n");
        }
    }     
}

// Function to create grades file if it doesn't exist
void createGradesFile() {
    pid_t pid = fork(); // Creating child process
    if (pid < 0) {
        printf("Error: Fork failed.\n");
    } else if (pid == 0) {
        int file = open("grades.txt", O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if (file != -1) {
            printf("\"grades.txt\" created successfully.\n");
            close(file);
        } else {
            printf("\"grades.txt\" already created.\n");
        }
        exit(EXIT_SUCCESS);
    } else {
        int status; // Parent process
        waitpid(pid, &status, 0); // Wait for child process to finish
        if (!WIFEXITED(status)) {
            printf("Error: Child process terminated abnormally.\n");
        }
    }
}

// Function to add student grade
void addStudentGrade(char *name, char *grade, char *filename) {
    pid_t pid = fork(); // Creating child process
    if (pid < 0) {
        printf("Error: Fork failed.\n");
    } else if (pid == 0) {
        int file = open(filename , O_WRONLY | O_APPEND);
        if (file != -1) {
            char studentInfo[MAX_LINE_LENGTH];
            sprintf(studentInfo, "%s, %s\n", name, grade);
            write(file, studentInfo, strlen(studentInfo));
            close(file);
            exit(EXIT_SUCCESS);
        } else {
            printf("Error opening file for appending.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        int status; // Parent process
        waitpid(pid, &status, 0); // Wait for child process to finish
        if (WIFEXITED(status)) {
            printf("Student grade added successfully.\n");
        } else {
            printf("Error: Child process terminated abnormally.\n");
        }
    }
}

// Function to read from file descriptor until newline character or EOF is encountered
ssize_t readline(int fd, char *buffer, size_t maxlen) {
    ssize_t bytesRead = 0;
    ssize_t totalBytesRead = 0;
    char ch;

    while ((bytesRead = read(fd, &ch, 1)) > 0 && (ssize_t)totalBytesRead < (ssize_t)maxlen - 1) {
        if (ch == '\n') {
            buffer[totalBytesRead] = '\0';
            return totalBytesRead + 1;
        }
        buffer[totalBytesRead++] = ch;
    }

    if (totalBytesRead > 0) {
        buffer[totalBytesRead] = '\0';
        return totalBytesRead;
    }

    return bytesRead;
}

// Function to search for student grade
void searchStudent(char *name, char *filename) {
    pid_t pid = fork(); // Creating child process
    if (pid < 0) {
        printf("Error: Fork failed.\n");
    } else if (pid == 0) {
        int file = open(filename, O_RDONLY);
        if (file != -1) {
            int found = 0;
            char line[MAX_LINE_LENGTH];
            char studentName[100];
            char studentGrade[100];
            ssize_t bytes_read;
            while ((bytes_read = readline(file, line, MAX_LINE_LENGTH)) > 0) {
                sscanf(line, "%[^,], %[^\n]", studentName, studentGrade);
                if (strcasecmp(studentName, name) == 0) {
                    printf("Student %s found. Grade: %s\n", name, studentGrade);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                printf("Student %s not found.\n", name);
            }
            close(file);
            exit(EXIT_SUCCESS);
        } else {
            printf("Error opening file for reading.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        int status; // Parent process
        waitpid(pid, &status, 0); // Wait for child process to finish
        if (WIFEXITED(status)) {
            printf("Student search done successfully.\n");
        } else {
            printf("Error: Child process terminated abnormally.\n");
        }
    }
}

typedef struct {
    char name[50];
    char grade[3]; 
} Student;

// Compare function for qsort - by name ascending
int compare_students_asc(const void *a, const void *b) {
    return strcmp(((const Student *)a)->name, ((const Student *)b)->name);
}

// Compare function for qsort - by name descending
int compare_students_desc(const void *a, const void *b) {
    return strcmp(((const Student *)b)->name, ((const Student *)a)->name);
}

// Compare function for qsort - by grade ascending
int compare_grades_asc(const void *a, const void *b) {
    return strcmp(((const Student *)a)->grade, ((const Student *)b)->grade);
}

// Compare function for qsort - by grade descending
int compare_grades_desc(const void *a, const void *b) {
    return strcmp(((const Student *)b)->grade, ((const Student *)a)->grade);
}

void sortAll(char *filename) {
    pid_t pid = fork(); // Creating child process
    if (pid < 0) {
        printf("Error: Fork failed.\n");
    } else if (pid == 0) {

        int fd_read = open(filename, O_RDONLY);
        if (fd_read == -1) {
            printf("Error opening file for reading");
            return;
        }

        // Count the number of lines in the file to determine the number of students
        int num_students = 0;
        char buffer[MAX_LINE_LENGTH];
        while (readline(fd_read, buffer, MAX_LINE_LENGTH) > 0) {
            num_students++;
        }
        close(fd_read);

        // Allocate memory for the students array based on the number of lines
        Student *students = malloc(num_students * sizeof(Student));
        if (students == NULL) {
            printf("Memory allocation failed.\n");
            return;
        }

        // Reopen the file to read student data
        fd_read = open(filename, O_RDONLY);
        if (fd_read == -1) {
            printf("Error opening file for reading");
            free(students);
            return;
        }

        // Read data from file
        int i = 0;
        while (i < num_students && readline(fd_read, buffer, MAX_LINE_LENGTH) > 0) {
            char *token = strtok(buffer, ",");
            if (token != NULL) {
                strcpy(students[i].name, token);
                token = strtok(NULL, ",");
                if (token != NULL) {
                    // Trim leading and trailing spaces
                    while (*token == ' ')
                        token++;
                    strcpy(students[i].grade, token);
                } else {
                    strcpy(students[i].grade, ""); // Empty grade if not provided
                }
                i++;
            }
        }
        close(fd_read);

        // Prompt for sorting criteria
        int sortType, sortOrder;
        printf("How would you like to sort students? By name(1) or by grade(2): \n");
        if (scanf("%d", &sortType) != 1) {
            printf("Invalid input for sorting type.\n");
            free(students);
            return;
        }
        printf("Ascending(1) or descending(2) order?: \n");
        if (scanf("%d", &sortOrder) != 1) {
            printf("Invalid input for sorting order.\n");
            free(students);
            return;
        }

        // Sort students according to desired conditions
        if (sortType == 1) {
            if (sortOrder == 1) {
                qsort(students, num_students, sizeof(Student), compare_students_asc);
            } else if (sortOrder == 2) {
                qsort(students, num_students, sizeof(Student), compare_students_desc);
            }    
        } else if (sortType == 2) {
            if (sortOrder == 1) {
                qsort(students, num_students, sizeof(Student), compare_grades_asc);
            } else if (sortOrder == 2) {
                qsort(students, num_students, sizeof(Student), compare_grades_desc);
            }    
        } else {
            printf("Invalid sorting option.\n");
            free(students);
            return;
        }

        // Write sorted data back to file
        int fd_write = open(filename, O_WRONLY | O_TRUNC);
        if (fd_write == -1) {
            printf("Error opening file for writing");
            free(students);
            return;
        }

        for (int i = 0; i < num_students; i++) {
            sprintf(buffer, "%s, %s\n", students[i].name, students[i].grade);
            ssize_t written_bytes = write(fd_write, buffer, strlen(buffer));
            if (written_bytes == -1) {
                printf("Error writing to file");
                free(students);
                close(fd_write);
                return;
            }
        }
        close(fd_write);
        // Print sorted entries (Optional for large datasets)
        printf("Sorted entries:\n");
        for (int i = 0; i < num_students; i++) {
            printf("%s, %s\n", students[i].name, students[i].grade);
        }
        free(students);
        exit(EXIT_SUCCESS);
    } else {
        int status; // Parent process
        waitpid(pid, &status, 0); // Wait for child process to finish
        if (WIFEXITED(status)) {
            printf("Student / grade sort done successfully.\n");
        } else {
            printf("Error: Child process terminated abnormally.\n");
        }
    }    
}

// Function to display all student grades
void showAll(char *filename) {
    pid_t pid = fork(); // Creating child process
    if (pid < 0) {
        printf("Error: Fork failed.\n");
    } else if (pid == 0) {
        // Child process
        // Open the file for reading
        int fd_open = open(filename, O_RDONLY);
        if (fd_open == -1) {
            printf("Error opening file for reading.\n");
            exit(EXIT_FAILURE);
        }

        // Read and print each line from the file
        char line[MAX_LINE_LENGTH]; // Adjust the buffer size as needed
        ssize_t bytes_read;
        while ((bytes_read = readline(fd_open, line, sizeof(line))) > 0) {
            printf("%s\n", line);
        }

        // Close the file
        close(fd_open);

        // Exit child process with success status
        exit(EXIT_SUCCESS);
    } else {
        int status; // Parent process
        waitpid(pid, &status, 0); // Wait for child process to finish
        if (WIFEXITED(status)) {
            printf("All student of the txt listed successfully.\n");
        } else {
            printf("Error: Child process terminated abnormally.\n");
        }
    }
}

// Function to display the first five student grades
void listGrades(char *filename) {
    pid_t pid = fork(); // Creating child process

    if (pid < 0) {
        // Fork failed
        printf("Error: Fork failed.\n");
    } else if (pid == 0) {
        // Child process
        // Open the file for reading
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            printf("Error opening file for reading.\n");
            exit(EXIT_FAILURE);
        }

        // Read and print the first 5 lines from the file
        char line[MAX_LINE_LENGTH]; // Adjust the buffer size as needed
        ssize_t bytes_read;
        for (int i = 0; i < 5 && (bytes_read = readline(fd, line, sizeof(line))) > 0; i++) {
            printf("%s\n", line);
        }

        // Close the file
        close(fd);

        // Exit child process with success status
        exit(EXIT_SUCCESS);
    } else {
        int status; // Parent process
        waitpid(pid, &status, 0); // Wait for child process to finish
        if (WIFEXITED(status)) {
            printf("Grade of first 5 student listed successfully.\n");
        } else {
            printf("Error: Child process terminated abnormally.\n");
        }
    }
}

// Function to display student grades based on page number and number of entries per page
void listSome(int numOfEntries, int pageNumber, char *filename) {
    pid_t pid = fork(); // Creating child process

    if (pid < 0) {
        // Fork failed
        printf("Error: Fork failed.\n");
    } else if (pid == 0) {
        // Child process
        // Calculate the starting line number based on page number and number of entries per page
        int startLine = (pageNumber - 1) * numOfEntries + 1;

        // Open the file for reading
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            printf("Error opening file for reading.\n");
            exit(EXIT_FAILURE);
        }

        // Skip lines until reaching the starting line
        for (int i = 1; i < startLine; i++) {
            char line[MAX_LINE_LENGTH]; // Adjust the buffer size as needed
            if (readline(fd, line, sizeof(line)) == -1) {
                // If the file ends before reaching the starting line, exit
                close(fd);
                exit(EXIT_SUCCESS);
            }
        }

        // Read and print the specified number of lines
        char line[MAX_LINE_LENGTH]; // Adjust the buffer size as needed
        ssize_t bytes_read;
        for (int i = 0; i < numOfEntries && (bytes_read = readline(fd, line, sizeof(line))) > 0; i++) {
            printf("%s\n", line);
        }

        // Close the file
        close(fd);

        // Exit child process with success status
        exit(EXIT_SUCCESS);
    } else {
        int status; // Parent process
        waitpid(pid, &status, 0); // Wait for child process to finish
        if (WIFEXITED(status)) {
            printf("Desired number and page displayed successfully.\n");
        } else {
            printf("Error: Child process terminated abnormally.\n");
        }
    }
}

int main() {
    char input[250]; // Assuming a maximum input length of 255 characters
    char command[20], arg1[50], arg2[50], arg3[50]; // args initializations
    printf("Type exit to terminate program.\n");

    while (1) {
        printf("Enter command: ");
        fflush(stdout);

        // Use read system call to read from standard input
        ssize_t bytesRead = read(0, input, sizeof(input));

        // Check if read was successful
        if (bytesRead < 0) {
            printf("Error reading input");
            return 1;
        }

        // Null-terminate the string
        input[bytesRead] = '\0';      
        sscanf(input, "%s \"%[^\"]\" \"%[^\"]\" \"%[^\"]\"", command, arg1, arg2, arg3);

        // Handle different commands
        if (strcmp(command, "exit") == 0) {
            // Exit the loop and terminate the program
            break;
        } else if ((strcmp(command, "gtuStudentGrades") == 0) && (strcmp(arg1, "grades.txt") == 0)){
            // Handle grades.txt command
            createGradesFile();
        } else if (strcmp(command, "addStudentGrade") == 0) {
            // Handle addStudentGrade command
            char *name = arg1;
            char *grade = arg2;
            addStudentGrade(name, grade, arg3);
        } else if (strcmp(command, "searchStudent") == 0) {
            // Handle searchStudent command
            char *name = arg1;
            searchStudent(name, arg2);
        } else if (strcmp(command, "sortAll") == 0) {
            // Handle sortAll command
            sortAll(arg1);
        } else if (strcmp(command, "showAll") == 0) {
            // Handle showAll command
            showAll(arg1);
        } else if (strcmp(command, "listGrades") == 0) {
            // Handle listGrades command
            listGrades(arg1);
        } else if (strcmp(command, "listSome") == 0) {
            // Handle listSome command
            int numEntries = atoi(arg1);
            int pageNumber = atoi(arg2);
            if (numEntries < 1 || pageNumber < 1) {
                printf("Entry number and page number must be positive.\n");
                continue;
            }
            listSome(numEntries, pageNumber, arg3);
        } else if (strcmp(command, "gtuStudentGrades") == 0) {
            // Display usage instructions
            displayUsage();
        } else {
            printf("Unknown command. Type 'gtuStudentGrades' without arguments for usage instructions.\n");
        } 

        writeLogEntry(command);
    }

    printf("Program terminated.\n");
    return 0;
}