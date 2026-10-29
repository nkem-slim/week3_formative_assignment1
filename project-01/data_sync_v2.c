#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// Returns 1 if the name exists, 0 if missing, or -1 on a read error.
int find_name(DIR *directory, const char *name)
{
    struct dirent *entry; // declares a pointer to a 'dirent' (directory entry) structure, which represents a single file or directory inside an open directory stream. 

    rewinddir(directory);
    errno = 0;
    while ((entry = readdir(directory)) != NULL)
    {
        if (strcmp(entry->d_name, name) == 0)
            return 1;
    }

    if (errno != 0)
        return -1;

    return 0;
}

// Builds a path and rejects names that exceed the buffer size.
int build_path(char *path, size_t size, const char *directory, const char *name)
{
    int length = snprintf(path, size, "%s/%s", directory, name);

    if (length < 0 || (size_t)length >= size)
    {
        fprintf(stderr, "File path is too long or could not be formatted\n");
        return 1;
    }

    return 0;
}

// Compares regular-file metadata; equal metadata does not prove equal contents.
int compare_metadata(const char *source_path, const char *destination_path, const char *name)
{
    struct stat source_info;
    struct stat destination_info;

    if (stat(source_path, &source_info) == -1)
    {
        perror("Could not inspect source file");
        return 1;
    }
    if (stat(destination_path, &destination_info) == -1)
    {
        perror("Could not inspect destination file");
        return 1;
    }

    if (!S_ISREG(source_info.st_mode) || !S_ISREG(destination_info.st_mode))
        printf("Skipping metadata comparison for non-regular entry: %s\n", name);
    else if (source_info.st_size != destination_info.st_size || source_info.st_mtime != destination_info.st_mtime)
        printf("Requires synchronization (metadata differs): %s\n", name);
    else
        printf("Size and modification time match: %s\n", name);

    return 0;
}

// Scans one directory level without copying or modifying files.
int scan_directory(DIR *source, DIR *destination, const char *source_directory, const char *destination_directory)
{
    struct dirent *entry;

    while (1)
    {
        errno = 0;
        entry = readdir(source);
        if (entry == NULL)
        {
            if (errno != 0)
            {
                perror("Could not read source directory");
                return 1;
            }
            break;
        }

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        int found = find_name(destination, entry->d_name);
        if (found == -1)
        {
            perror("Could not read destination directory");
            return 1;
        }

        if (found == 0)
            printf("Requires synchronization: %s\n", entry->d_name);
        else
        {
            char source_path[4096];
            char destination_path[4096];

            if (build_path(source_path, sizeof(source_path), source_directory, entry->d_name) != 0 || build_path(destination_path, sizeof(destination_path), destination_directory, entry->d_name) != 0)
                return 1;

            if (compare_metadata(source_path, destination_path, entry->d_name) != 0)
                return 1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s SOURCE_DIRECTORY DESTINATION_DIRECTORY\n", argv[0]);
        return 1;
    }

    DIR *source = opendir(argv[1]);
    if (source == NULL)
    {
        perror("Could not open source directory");
        return 1;
    }

    DIR *destination = opendir(argv[2]);
    if (destination == NULL)
    {
        perror("Could not open destination directory");
        closedir(source);
        return 1;
    }

    int result = scan_directory(source, destination, argv[1], argv[2]);

    if (closedir(source) == -1)
    {
        perror("Could not close source directory");
        result = 1;
    }
    if (closedir(destination) == -1)
    {
        perror("Could not close destination directory");
        result = 1;
    }

    return result;
}
