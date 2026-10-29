# Signal-Driven Server Controller

## Build and Signal Handling

Compile from `project-04` with `gcc server_controller.c -o server_controller`.
Run with `./server_controller`; the service prints its PID and begins in NORMAL
mode. `sigaction()` registers one handler for SIGUSR1, SIGUSR2, SIGINT, and
SIGTERM. The handler only assigns `volatile sig_atomic_t` flags. It does not
call `printf()` or perform cleanup, avoiding unsafe library operations inside
the asynchronous handler.

The main loop processes flags and prints messages. The four signals are blocked
while flags are inspected and reset. `pselect()` atomically unblocks them while
waiting and restores the blocked mask on return, avoiding a race between checking
requests and sleeping. An interrupted wait with `EINTR` is expected. A monotonic
clock controls five-second monitoring intervals, so status requests do not restart
the entire interval. Scheduling delays can affect the exact output time.

## Operational State and Termination

A state variable selects NORMAL or MAINTENANCE. SIGUSR2 requests a toggle;
the main loop changes state and reports it. SIGUSR1 requests a status report
containing the current state and PID. Monitoring continues in either mode.

SIGINT requests graceful shutdown: the loop reports shutdown and completion of
cleanup before returning. SIGTERM requests the emergency termination message
and exits without the graceful-shutdown message sequence. Both paths are handled
in the main loop and return normally; SIGTERM is not an uncatchable forced kill.
The program holds no open files or allocated buffers requiring explicit cleanup,
and normal return flushes standard output. Emergency termination takes priority
if both termination flags are set.

## Recorded Demonstration

The service output was captured using `./server_controller | tee LOG_FILE`.
Signals were sent with `kill -SIGUSR1 PID`, `kill -SIGUSR2 PID`,
`kill -SIGINT PID`, and `kill -SIGTERM PID`, substituting each run's actual PID.

| Evidence file | PID | Observed behavior |
| --- | --- | --- |
| `signal-demo.log` | 1497211 | NORMAL status, maintenance transition and status, continued monitoring, graceful SIGINT shutdown |
| `signal-term-demo.log` | 1547686 | Maintenance status and emergency SIGTERM termination |
| `signal-toggle-demo.log` | 1550553 | NORMAL to MAINTENANCE to NORMAL, another maintenance transition, status report, and SIGINT shutdown |

The final run explicitly records `Mode changed to NORMAL.` followed by
`Service running normally...`, confirming the reverse transition. The logs
demonstrate responses but contain no timestamps proving exact five-second
spacing. Standard Unix signals may coalesce, and these Boolean request flags
also merge repeated requests pending before processing; administrators should
wait for each response when testing individual toggles.
