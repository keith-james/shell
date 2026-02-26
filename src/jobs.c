#include <stdio.h>
#include <string.h>
#include "jobs.h"

void init_jobs(job jobs[]) {
    memset(jobs, 0, sizeof(job) * MAX_JOBS);
}

int find_free_job_slot(job jobs[]) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active)
            return i;
    }
    return -1;
}

int find_job_by_id(job jobs[], int jobid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active && jobs[i].jobid == jobid)
            return i;
    }
    return -1;
}

static const char *job_state_str(job_state state) {
    switch (state) {
        case JOB_RUNNING: return "Running";
        case JOB_STOPPED: return "Stopped";
        default: return "Unknown";
    }
}

void print_jobs(job jobs[]) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            printf("[%d]  %s  %s\n",
                   jobs[i].jobid,
                   job_state_str(jobs[i].state),
                   jobs[i].command);
        }
    }
}

void remove_job(job jobs[], int index) {
    jobs[index].active = 0;
}

int add_job(job jobs[], int jobid, pid_t pgid, job_state state, const char *cmd) {
    int slot = find_free_job_slot(jobs);
    if (slot == -1)
        return -1;

    jobs[slot].active = 1;
    jobs[slot].jobid = jobid;
    jobs[slot].pgid = pgid;
    jobs[slot].state = state;
    strncpy(jobs[slot].command, cmd, CMD_LEN);
    jobs[slot].command[CMD_LEN - 1] = '\0';

    return slot;
}
