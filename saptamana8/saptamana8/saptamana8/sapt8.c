#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_FILENAME_LENGTH 256
#define MAX_CHILD_PROCESS_COUNT 10

// Function to get current timestamp as a string
char* get_timestamp() {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[80];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    return strdup(timestamp);
}

// Function to check if a file exists
int file_exists(const char* path) {
    FILE* file = fopen(path, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Function to recursively traverse directory and record file information
void record_file_info(const char* dirpath, FILE* snapshot_file) {
    DIR* dir = opendir(dirpath);
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    struct dirent* entry;
    struct stat statbuf;
    char filepath[MAX_FILENAME_LENGTH];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; // Skip "." and ".." entries
        }

        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);

        if (stat(filepath, &statbuf) == -1) {
            perror("Error getting file stats");
            continue;
        }

        fprintf(snapshot_file, "%s %ld\n", entry->d_name, statbuf.st_mtime);

        if (S_ISDIR(statbuf.st_mode)) {
            record_file_info(filepath, snapshot_file); // Recursively explore subdirectories
        }
    }

    closedir(dir);
}

// Function to create and manage child processes for creating snapshots
void create_and_manage_child_processes(const char* output_dir, const char* directory_paths[], int num_directories) {
    int child_process_pids[MAX_CHILD_PROCESS_COUNT]; // Array to store child process IDs
    int num_active_children = 0; // Number of active child processes

    // Loop through directory paths and create child processes for each
    for (int i = 0; i < num_directories; i++) {
        const char* dirpath = directory_paths[i];

        // Check if maximum child process limit is reached
        if (num_active_children >= MAX_CHILD_PROCESS_COUNT) {
            // Wait for a child process to finish before creating a new one
            int child_pid = wait(NULL, NULL, WNOHANG);
            if (child_pid > 0) {
                // Child process exited, decrement active child count
                num_active_children--;
            }
        }

        // Create a new child process
        pid_t child_pid = fork();
        if (child_pid == 0) { // Child process
            // Construct snapshot filename based on output directory and directory name
            char snapshot_path[MAX_FILENAME_LENGTH];
            snprintf(snapshot_path, sizeof(snapshot_path), "%s/%s_snapshot.txt", output_dir, strrchr(dirpath, '/') + 1);

            // Create snapshot file and record file information
            FILE* snapshot_file = fopen(snapshot_path, "w+");
            if (snapshot_file) {
                record_file_info(dirpath, snapshot_file);
                fclose(snapshot_file);
                printf("Snapshot updated for directory: %s\n", dirpath);
            }
            else {
                perror("Error opening snapshot file");
            }

            // Exit the child process
            exit(0);
        }
        else if (child_pid > 0) { // Parent process
            // Store child process ID for tracking
            child_process_pids[num_active_children] = child_pid;
            num_active_children++;
            printf("Child process created (PID: %d) for directory: %s\n", child_pid, dirpath);
        }
        else {
            // Fork failed, handle error
            perror("Error creating child process");
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < num_active_children; i++) {
        waitpid(child_process_pids[i], NULL, 0);
    }
}

int main(int argc, char* argv[]) {
    const int MAX_DIR_COUNT = 10; // Maximum number of directory paths allowed
    char* directory_paths[MAX_DIR_COUNT];

    // Process command line arguments and capture directory paths
    int num_directories = process_arguments(argc, argv, directory_paths, MAX_DIR_COUNT);
    if (num_directories <= 0) {
        return 1;
    }

    const char* output_dir = argv[num_directories + 1];

    // Create and manage child processes for creating snapshots
    create_and_manage_child_processes(output_dir, directory_paths, num_directories);

    printf("Snapshots created for all specified directories.\n");

    return 0;
}
