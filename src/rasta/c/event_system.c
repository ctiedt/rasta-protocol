#include"event_system.h"
#include"rasta_new.h"
#include<time.h>
#include<sys/select.h>

uint64_t get_nanotime() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000000000 + t.tv_nsec;
}

/**
 * sleeps but keeps track of the fd events
 * @param time_to_wait the time to sleep in nanoseconds
 * @param fd_events the fd event array
 * @param len the length of the fd event array
 * @return the amount of fd events that got called or -1 to terminate the event loop
 */
int event_system_sleep(uint64_t time_to_wait, fd_event fd_events[], int len) {
    struct timeval tv;
    tv.tv_sec = time_to_wait / 1000000000;
    tv.tv_usec = (time_to_wait / 1000) % 1000000;
    int nfds = 0;
    // find highest fd
    for (int i = 0; i < len; i++) nfds = nfds < fd_events[i].fd ? fd_events[i].fd : nfds;
    // set the fd to watch
    fd_set on_readable;
    FD_ZERO(&on_readable);
    for (int i = 0; i < len; i++) FD_SET(fd_events[i].fd, &on_readable);
    // wait
    int result = select(nfds + 1, &on_readable, NULL, NULL, &tv);
    if (result == -1) {
        // syscall error or error on select()
        return -1;
    }
    for (int i = 0; i < len; i++) {
        if (FD_ISSET(fd_events[i].fd, &on_readable)) {
            if (fd_events[i].callback()) return -1;
        }
    }
    return result;
}

/**
 * starts an event loop with the given events
 * the events may not be removed while the loop is running but can be modified
 * @param timed_events an array with the looping events to handle
 * @param timed_events_len the length of the timed_event array
 * @param fd_events an array with the events, that get called whenever the given fd gets readable
 * @param fd_events_len the length of the fd event array
 */
void start_event_loop(timed_event timed_events[], int timed_events_len, fd_event fd_events[], int fd_events_len) {
    uint64_t cur_time = get_nanotime();
    uint64_t last_call[timed_events_len];
    for (int i = 0; i < timed_events_len; i++) {
        last_call[i] = cur_time;
    }
    while (1) {
        cur_time = get_nanotime();
        uint64_t time_to_wait = UINT64_MAX;
        int next_event;
        // find next timed event
        for (int i = 0; i < timed_events_len; i++) {
            uint64_t continue_at = last_call[i] + timed_events[i].interval;
            if (continue_at <= cur_time) {
                time_to_wait = 0;
                next_event = i;
                break;
            }
            else {
                uint64_t new_time_to_wait = continue_at - cur_time;
                if (new_time_to_wait < time_to_wait) {
                    next_event = i;
                    time_to_wait = new_time_to_wait;
                }
            }
        }
        if (time_to_wait != 0) {
            int result = event_system_sleep(time_to_wait, fd_events, fd_events_len);
            if (result == -1) {
                break;
            }
            else if (result > 0) {
                // the sleep didn't time out, but a fd event occured
                // recalculate next timed event in case it got resceduled
                continue;
            }
        }
        timed_events[next_event].callback(cur_time - last_call[next_event]);
        last_call[next_event] = cur_time + time_to_wait;
    }
}