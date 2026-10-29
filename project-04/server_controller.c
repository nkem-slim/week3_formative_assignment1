#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t status_requested = 0;
static volatile sig_atomic_t toggle_requested = 0;
static volatile sig_atomic_t shutdown_requested = 0;
static volatile sig_atomic_t emergency_requested = 0;

// Only assign flags here; output and state changes belong in the main loop.
void handle_signal(int signal_number)
{
    if (signal_number == SIGUSR1)
        status_requested = 1;
    else if (signal_number == SIGUSR2)
        toggle_requested = 1;
    else if (signal_number == SIGINT)
        shutdown_requested = 1;
    else if (signal_number == SIGTERM)
        emergency_requested = 1;
}

int register_handlers(const sigset_t *signals)
{
    struct sigaction action = {0};
    action.sa_handler = handle_signal;
    action.sa_mask = *signals;

    int signal_numbers[] = {SIGUSR1, SIGUSR2, SIGINT, SIGTERM};
    for (int i = 0; i < 4; i++)
    {
        if (sigaction(signal_numbers[i], &action, NULL) == -1)
        {
            perror("Could not register signal handler");
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    sigset_t signals;
    sigset_t previous_mask;
    sigemptyset(&signals);
    sigaddset(&signals, SIGUSR1);
    sigaddset(&signals, SIGUSR2);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGTERM);

    // Block signals while reading/resetting flags to avoid losing a request.
    if (sigprocmask(SIG_BLOCK, &signals, &previous_mask) == -1)
    {
        perror("Could not block signals");
        return 1;
    }
    if (register_handlers(&signals) != 0)
        return 1;

    sigset_t wait_mask = previous_mask;
    sigdelset(&wait_mask, SIGUSR1);
    sigdelset(&wait_mask, SIGUSR2);
    sigdelset(&wait_mask, SIGINT);
    sigdelset(&wait_mask, SIGTERM);

    int maintenance = 0;
    struct timespec next_report;
    if (clock_gettime(CLOCK_MONOTONIC, &next_report) == -1)
    {
        perror("Could not read clock");
        return 1;
    }

    printf("Server Controller PID: %ld\n", (long)getpid());
    printf("Server Controller Service running normally...\n");
    fflush(stdout);
    next_report.tv_sec += 5;

    while (1)
    {
        if (emergency_requested)
        {
            printf("Server Controller Emergency termination requested (SIGTERM).\n");
            break;
        }
        if (shutdown_requested)
        {
            printf("Server Controller Graceful shutdown requested (SIGINT).\n");
            printf("Server Controller Monitoring stopped; cleanup complete.\n");
            break;
        }
        if (toggle_requested)
        {
            toggle_requested = 0;
            maintenance = !maintenance;
            printf("Server Controller Mode changed to %s.\n", maintenance ? "MAINTENANCE" : "NORMAL");
        }
        if (status_requested)
        {
            status_requested = 0;
            printf("Server Controller Status: %s | PID: %ld\n", maintenance ? "MAINTENANCE" : "NORMAL", (long)getpid());
        }

        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
        {
            perror("Could not read clock");
            return 1;
        }
        if (now.tv_sec > next_report.tv_sec ||
            (now.tv_sec == next_report.tv_sec && now.tv_nsec >= next_report.tv_nsec))
        {
            printf("Server Controller %s\n", maintenance ? "Service in maintenance mode..." : "Service running normally...");
            next_report = now;
            next_report.tv_sec += 5;
        }
        fflush(stdout);

        struct timespec remaining;
        remaining.tv_sec = next_report.tv_sec - now.tv_sec;
        remaining.tv_nsec = next_report.tv_nsec - now.tv_nsec;
        if (remaining.tv_nsec < 0)
        {
            remaining.tv_sec--;
            remaining.tv_nsec += 1000000000L;
        }

        // Atomically unblock signals while waiting, then restore the block.
        if (pselect(0, NULL, NULL, NULL, &remaining, &wait_mask) == -1 && errno != EINTR)
        {
            perror("Could not wait for monitoring interval");
            return 1;
        }
    }

    // No files or allocated buffers are held; returning also flushes output.
    return 0;
}
