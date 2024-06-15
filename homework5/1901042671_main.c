#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#define PATH_MAX 4096

typedef struct {
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];
} file_task_t;

typedef struct {
    file_task_t *tasks;
    int size;
    int count;
    int head;
    int tail;
    int done;
    pthread_mutex_t mutex;
    pthread_cond_t cond_full;
    pthread_cond_t cond_empty;
} task_buffer_t;

task_buffer_t buffer;
int num_workers;
pthread_t *workers;
pthread_t manager_thread;
int buffer_size;
int num_regular_files = 0;
int num_fifo_files = 0;
int num_directories = 0;
size_t total_bytes_copied = 0;
pthread_mutex_t stats_mutex;
pthread_barrier_t barrier;

void cleanup() {
    // Free dynamically allocated memory here
    free(workers);
    free(buffer.tasks);

    pthread_mutex_destroy(&buffer.mutex);
    pthread_cond_destroy(&buffer.cond_full);
    pthread_cond_destroy(&buffer.cond_empty);
    pthread_mutex_destroy(&stats_mutex);
    pthread_barrier_destroy(&barrier);
}

void sigint_handler(int sig) {
    printf("\nReceived SIGINT signal. Exiting...\n");

    // Perform cleanup actions here if needed
    cleanup();

    exit(0);
}

void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void *worker_thread_func(void *arg) {
    while (1) {
        pthread_mutex_lock(&buffer.mutex);

        while (buffer.count == 0 && !buffer.done) {
            pthread_cond_wait(&buffer.cond_empty, &buffer.mutex);
        }

        if (buffer.count == 0 && buffer.done) {
            pthread_mutex_unlock(&buffer.mutex);
            break;
        }

        file_task_t task = buffer.tasks[buffer.head];
        buffer.head = (buffer.head + 1) % buffer.size;
        buffer.count--;

        pthread_cond_signal(&buffer.cond_full);
        pthread_mutex_unlock(&buffer.mutex);

        int src_fd = open(task.src_path, O_RDONLY);
        if (src_fd < 0) {
            fprintf(stderr, "Failed to open source file %s: %s\n", task.src_path, strerror(errno));
            continue;
        }

        int dest_fd = open(task.dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dest_fd < 0) {
            fprintf(stderr, "Failed to open destination file %s: %s\n", task.dest_path, strerror(errno));
            close(src_fd);
            continue;
        }

        char buffer[8192];
        ssize_t bytes_read, bytes_written;
        while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
            bytes_written = write(dest_fd, buffer, bytes_read);
            if (bytes_written != bytes_read) {
                fprintf(stderr, "Failed to write to file %s: %s\n", task.dest_path, strerror(errno));
                break;
            }
            pthread_mutex_lock(&stats_mutex);
            total_bytes_copied += bytes_written;
            pthread_mutex_unlock(&stats_mutex);
        }

        close(src_fd);
        close(dest_fd);

        printf("Copied %s to %s\n", task.src_path, task.dest_path);

        pthread_mutex_lock(&stats_mutex);
        num_regular_files++;
        pthread_mutex_unlock(&stats_mutex);
    }

    pthread_barrier_wait(&barrier);
    return NULL;
}

void add_task(const char *src_path, const char *dest_path) {
    pthread_mutex_lock(&buffer.mutex);

    while (buffer.count == buffer.size) {
        pthread_cond_wait(&buffer.cond_full, &buffer.mutex);
    }

    strncpy(buffer.tasks[buffer.tail].src_path, src_path, PATH_MAX - 1);
    buffer.tasks[buffer.tail].src_path[PATH_MAX - 1] = '\0';
    strncpy(buffer.tasks[buffer.tail].dest_path, dest_path, PATH_MAX - 1);
    buffer.tasks[buffer.tail].dest_path[PATH_MAX - 1] = '\0';
    buffer.tail = (buffer.tail + 1) % buffer.size;
    buffer.count++;

    pthread_cond_signal(&buffer.cond_empty);
    pthread_mutex_unlock(&buffer.mutex);
}

void delete_directory_contents(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Failed to open directory %s: %s\n", path, strerror(errno));
        return;
    }

    struct dirent *entry;
    char file_path[PATH_MAX];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(file_path, PATH_MAX, "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(file_path, &st) == -1) {
            fprintf(stderr, "Failed to stat %s: %s\n", file_path, strerror(errno));
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            delete_directory_contents(file_path);
            if (rmdir(file_path) == -1) {
                fprintf(stderr, "Failed to remove directory %s: %s\n", file_path, strerror(errno));
            }
        } else {
            if (unlink(file_path) == -1) {
                fprintf(stderr, "Failed to remove file %s: %s\n", file_path, strerror(errno));
            }
        }
    }

    closedir(dir);
}

void copy_directory(const char *src_dir, const char *dest_dir) {
    DIR *dir = opendir(src_dir);
    if (!dir) {
        fprintf(stderr, "Failed to open directory %s: %s\n", src_dir, strerror(errno));
        return;
    }

    struct stat st;
    struct dirent *entry;
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(src_path, PATH_MAX, "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, PATH_MAX, "%s/%s", dest_dir, entry->d_name);

        if (stat(src_path, &st) == -1) {
            fprintf(stderr, "Failed to stat %s: %s\n", src_path, strerror(errno));
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            mkdir(dest_path, 0755);
            pthread_mutex_lock(&stats_mutex);
            num_directories++;
            pthread_mutex_unlock(&stats_mutex);
            copy_directory(src_path, dest_path);
        } else if (S_ISFIFO(st.st_mode)) {
            pthread_mutex_lock(&stats_mutex);
            num_fifo_files++;
            pthread_mutex_unlock(&stats_mutex);
        } else {
            add_task(src_path, dest_path);
        }
    }

    closedir(dir);
}

void *manager_thread_func(void *arg) {
    char **directories = (char **)arg;
    char *src_dir = directories[0];
    char *dest_dir = directories[1];

    delete_directory_contents(dest_dir);

    copy_directory(src_dir, dest_dir);

    pthread_mutex_lock(&buffer.mutex);
    buffer.done = 1;
    pthread_cond_broadcast(&buffer.cond_empty);
    pthread_mutex_unlock(&buffer.mutex);

    return NULL;
}

void usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s <buffer_size> <num_workers> <src_dir> <dest_dir>\n", prog_name);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        usage(argv[0]);
    }

    buffer_size = atoi(argv[1]);
    num_workers = atoi(argv[2]);
    char *src_dir = argv[3];
    char *dest_dir = argv[4];

    if (buffer_size <= 0 || num_workers <= 0) {
        usage(argv[0]);
    }

    buffer.tasks = malloc(buffer_size * sizeof(file_task_t));
    if (!buffer.tasks) {
        handle_error("Failed to allocate buffer");
    }
    buffer.size = buffer_size;
    buffer.count = 0;
    buffer.head = 0;
    buffer.tail = 0;
    buffer.done = 0;
    pthread_mutex_init(&buffer.mutex, NULL);
    pthread_cond_init(&buffer.cond_full, NULL);
    pthread_cond_init(&buffer.cond_empty, NULL);
    pthread_barrier_init(&barrier, NULL, num_workers);

    workers = malloc(num_workers * sizeof(pthread_t));
    if (!workers) {
        handle_error("Failed to allocate worker threads");
    }

    pthread_mutex_init(&stats_mutex, NULL);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    char *directories[] = {src_dir, dest_dir};
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (pthread_create(&manager_thread, NULL, manager_thread_func, directories) != 0) {
        handle_error("Failed to create manager thread");
    }

    for (int i = 0; i < num_workers; i++) {
        if (pthread_create(&workers[i], NULL, worker_thread_func, NULL) != 0) {
            handle_error("Failed to create worker thread");
        }
    }

    pthread_join(manager_thread, NULL);

    // Clean up
    for (int i = 0; i < num_workers; i++) {
        pthread_join(workers[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000;
    }
    double elapsed_time = seconds + nanoseconds * 1e-9;

    printf("\n---------------STATISTICS--------------------\n");
    printf("Consumers: %d - Buffer Size: %d\n", num_workers, buffer_size);
    printf("Number of Regular Files: %d\n", num_regular_files);
    printf("Number of FIFO Files: %d\n", num_fifo_files);
    printf("Number of Directories: %d\n", num_directories);
    printf("TOTAL BYTES COPIED: %zu\n", total_bytes_copied);
    printf("TOTAL TIME: %.6f seconds\n", elapsed_time);

    atexit(cleanup);

    return 0;
}

