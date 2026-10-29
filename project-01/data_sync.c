#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

// Returns 1 if the name exists, 0 if missing, or -1 on a read error.
int find_name(DIR *directory, const char *name)
{
    struct dirent *entry;

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

// Compares entry names only; it does not inspect contents or copy files.
int scan_directory(DIR *source, DIR *destination)
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
            printf("Name already present: %s\n", entry->d_name);
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

    int result = scan_directory(source, destination);

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
