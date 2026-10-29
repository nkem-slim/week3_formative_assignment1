#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Handles interrupted writes and writes that transfer only part of a buffer.
int write_all(int descriptor, const char *buffer, size_t length)
{
    size_t written = 0;

    while (written < length)
    {
        ssize_t count = write(descriptor, buffer + written, length - written);
        if (count == -1)
        {
            if (errno == EINTR)
                continue;
            return 1;
        }
        if (count == 0)
        {
            errno = EIO;
            return 1;
        }
        written += (size_t)count;
    }

    return 0;
}

int log_entry(int log_descriptor, int number, const char *message)
{
    char line[256];
    int length = snprintf(line, sizeof(line), "%d. %s\n", number, message);

    if (length < 0 || (size_t)length >= sizeof(line))
    {
        fprintf(stderr, "Could not format log entry\n");
        return 1;
    }
    if (write_all(log_descriptor, line, (size_t)length) != 0)
    {
        perror("Could not write log entry");
        return 1;
    }

    return 0;
}

// Opens without truncating first, so copying a file onto itself can be rejected.
int copy_file(const char *source_path)
{
    int source = open(source_path, O_RDONLY);
    if (source == -1)
    {
        perror("Could not open source file");
        return 1;
    }

    int destination = -1;
    int result = 1;
    struct stat source_info;
    struct stat destination_info;

    printf("Source file opened\n");
    if (fstat(source, &source_info) == -1)
    {
        perror("Could not inspect source file");
        goto cleanup;
    }
    if (!S_ISREG(source_info.st_mode))
    {
        fprintf(stderr, "Source must be a regular file\n");
        goto cleanup;
    }

    destination = open("backup/source.txt", O_WRONLY | O_CREAT | O_NONBLOCK, 0644);
    if (destination == -1)
    {
        perror("Could not open destination file");
        goto cleanup;
    }
    if (fstat(destination, &destination_info) == -1)
    {
        perror("Could not inspect destination file");
        goto cleanup;
    }
    if (!S_ISREG(destination_info.st_mode) ||
        (source_info.st_dev == destination_info.st_dev &&
         source_info.st_ino == destination_info.st_ino))
    {
        fprintf(stderr, "Destination must be a separate regular file\n");
        goto cleanup;
    }
    if (ftruncate(destination, 0) == -1)
    {
        perror("Could not clear destination file");
        goto cleanup;
    }
    printf("Destination file ready\n");

    char buffer[4096];
    while (1)
    {
        ssize_t count = read(source, buffer, sizeof(buffer));
        if (count == -1)
        {
            if (errno == EINTR)
                continue;
            perror("Could not read source file");
            goto cleanup;
        }
        if (count == 0)
            break;
        if (write_all(destination, buffer, (size_t)count) != 0)
        {
            perror("Could not write destination file");
            goto cleanup;
        }
    }
    result = 0;

cleanup:
    if (close(source) == -1)
    {
        perror("Could not close source file");
        result = 1;
    }
    if (destination != -1 && close(destination) == -1)
    {
        perror("Could not close destination file");
        result = 1;
    }
    return result;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s SOURCE_FILE\n", argv[0]);
        return 1;
    }

    if (mkdir("backup", 0755) == -1)
    {
        if (errno != EEXIST)
        {
            perror("Could not create backup directory");
            return 1;
        }
        struct stat directory_info;
        if (stat("backup", &directory_info) == -1)
        {
            perror("Could not inspect backup directory");
            return 1;
        }
        if (!S_ISDIR(directory_info.st_mode))
        {
            fprintf(stderr, "Backup path is not a directory\n");
            return 1;
        }
        printf("Backup directory already exists\n");
    }
    else
        printf("Backup directory created\n");

    int log_descriptor = open("backup.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (log_descriptor == -1)
    {
        perror("Could not create log file");
        return 1;
    }
    printf("Log file created\n");
    if (log_entry(log_descriptor, 1, "Backup directory ready") != 0)
    {
        close(log_descriptor);
        return 1;
    }

    // Flush output before fork so the child does not repeat buffered messages.
    fflush(stdout);
    pid_t child = fork();
    if (child == -1)
    {
        perror("Could not create child process");
        close(log_descriptor);
        return 1;
    }
    if (child == 0)
    {
        int result = copy_file(argv[1]);
        if (log_entry(log_descriptor, 2, result == 0 ? "Child copy completed" : "Child copy failed") != 0)
            result = 1;
        if (close(log_descriptor) == -1)
        {
            perror("Could not close child log descriptor");
            result = 1;
        }
        exit(result);
    }

    int status;
    pid_t waited;
    do
    {
        waited = waitpid(child, &status, 0);
    } while (waited == -1 && errno == EINTR);

    int result = 1;
    if (waited == -1)
        perror("Could not wait for child process");
    else if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        result = 0;

    if (log_entry(log_descriptor, 3, result == 0 ? "Parent confirmed success" : "Parent detected failure") != 0)
        result = 1;
    if (close(log_descriptor) == -1)
    {
        perror("Could not close parent log descriptor");
        result = 1;
    }

    if (result == 0)
        printf("Backup completed successfully\n");
    else
        fprintf(stderr, "Backup failed\n");

    return result;
}
