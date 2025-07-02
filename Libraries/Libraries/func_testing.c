#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>  // For opendir(), readdir(), closedir()
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>  // For getcwd()

#define MAX_PATH_LENGTH 4096

void getTextFilesFromDir(const char* testcase) {
    const char* directory = "./test_cases";
    // char *directory;  // Change this to your directory
    // snprintf(directory, sizeof(MAX_PATH_LENGTH), "./test_cases/%s", testcase);

    struct dirent *entry;

    DIR *dp = opendir(directory);
    if (dp == NULL) {
        perror("opendir");
    }

    char **file_paths = NULL;
    size_t file_count = 0;

    while ((entry = readdir(dp)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        // Allocate memory for the new path
        file_paths = realloc(file_paths, sizeof(char *) * (file_count + 1));
        if (!file_paths) {
            perror("realloc");
            closedir(dp);
        }

        // Construct full path
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

        // Save the path
        struct stat statbuf;
        
        if(stat(full_path, &statbuf) == -1) {
            perror("[stat]: Path is invalid.");
        }

        if (S_ISREG(statbuf.st_mode))
        {
            file_paths[file_count] = strdup(full_path);
            if (!file_paths[file_count]) {
                perror("strdup");
                closedir(dp);
            }

            file_count++;
        }
        
        
    }

    closedir(dp);

    // // Print the paths
    // for (size_t i = 0; i < file_count; i++) {
    //     printf("File %zu: %s\n", i + 1, file_paths[i]);
    //     free(file_paths[i]);  // Free each string
    // }
    // free(file_paths);  // Free the array itself

}

int main () {
    getFilesFromDir("10K");

    return 0;
}