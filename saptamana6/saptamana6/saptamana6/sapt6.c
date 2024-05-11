#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }

    const char* dirpath = argv[1];

    char snapshot_path[MAX_FILENAME_LENGTH];
    snprintf(snapshot_path, sizeof(snapshot_path), "%s/snapshot.txt", dirpath);

    // Check if snapshot file exists
    if (!file_exists(snapshot_path)) {
        printf("Creating snapshot for the first time...\n");
    }

    FILE* snapshot_file = fopen(snapshot_path, "w+");
    if (!snapshot_file) {
        perror("Error opening snapshot file");
        return 1;
    }

    // Record file information for the current directory structure
    record_file_info(dirpath, snapshot_file);

    fclose(snapshot_file);

    printf("Snapshot updated for directory: %s\n", dirpath);

    // Optional: Implement logic to compare with previous snapshot and log changes

    return 0;
}
