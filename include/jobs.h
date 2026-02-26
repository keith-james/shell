#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

#define MAX_JOBS 64
#define CMD_LEN 1000

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED
} job_state;

typedef struct {
    int active;
    int jobid;
    pid_t pgid;
    job_state state;
    char command[CMD_LEN];
} job;

void init_jobs(job jobs[]);
int find_free_job_slot(job jobs[]);
int find_job_by_id(job jobs[], int jobid);
void print_jobs(job jobs[]);
void remove_job(job jobs[], int index);
int add_job(job jobs[], int jobid, pid_t pgid, job_state state, const char *cmd);

#endif
