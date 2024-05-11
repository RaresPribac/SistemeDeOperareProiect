#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

#define MAX_FILENAME_LENGTH 256

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

// Function to process command line arguments and capture directory paths
int process_arguments(int argc, char* argv[], char* directory_paths[], int max_directories) {
    if (argc < 3 || argc > max_directories + 2) {
        fprintf(stderr, "Usage: %s <directory_path1> [directory_path2] ... [directory_path%d] -o <output_directory>\n", argv[0], max_directories);
        return 1;
    }

    int directory_count = 0;
    int output_dir_index = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            // Output directory specified
            if (i + 1 >= argc || output_dir_index != -1) {
                fprintf(stderr, "Error: Invalid usage of -o option\n");
                return 1;
            }
            output_dir_index = i + 1;
        }
        else {
            if (directory_count == max_directories) {
                fprintf(stderr, "Error: Maximum number of directories (%d) reached\n", max_directories);
                return 1;
            }
            directory_paths[directory_count++] = argv[i];
        }
    }

    if (output_dir_index == -1) {
        fprintf(stderr, "Error: Output directory not specified using -o option\n");
        return 1;
    }

    return directory_count;
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

    // Loop through each directory path provided
    for (int i = 0; i < num_directories; i++) {
        const char* dirpath = directory_paths[i];

        // Construct snapshot filename based on output directory and directory name
        char snapshot_path[MAX_FILENAME_LENGTH];
        snprintf(snapshot_path, sizeof(snapshot_path), "%s/%s_snapshot.txt", output_dir, strrchr(dirpath, '/') + 1); // strrchr finds the last occurrence of '/'

        // Check if snapshot file exists
        if (!file_exists(snapshot_path)) {
            printf("Creating snapshot for directory: %s\n", dirpath);
        }

        FILE* snapshot_file = fopen(snapshot_path, "w+");
        if (!snapshot_file) {
            perror("Error opening snapshot file");
            continue; // Skip to the next directory on error
        }

        // Record file information for the current directory structure
        record_file_info(dirpath, snapshot_file);

        fclose(snapshot_file);

        printf("Snapshot updated for directory: %s\n", dirpath);
    }

    printf("Snapshots created for all specified directories.\n");

    return 0;
}

