#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define MAX_ARGS 64
#define MAX_JOBS 64
#define CMD_LEN  1000

volatile sig_atomic_t sigint_received = 0;

void handle_sigint(int sig) {
  sigint_received = 1;
  write(STDOUT_FILENO, "\n", 1);
}

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


const char *job_state_str(job_state state) {
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

int main() {

  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTSTP, SIG_IGN); 

  pid_t shell_pid = getpid();
  setpgid(shell_pid, shell_pid);
  tcsetpgrp(STDIN_FILENO, shell_pid);

  job jobs[MAX_JOBS];
  int next_job_id = 1;

  memset(jobs, 0, sizeof(jobs));

  char line[1000]; 
  char *argv[MAX_ARGS];

  struct sigaction sa;
  sa.sa_handler = handle_sigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);


  while(1) {

    printf("\n> ");
    sigint_received = 0;

    fflush(stdout);
    if(fgets(line, sizeof(line), stdin) == NULL) {
      if(sigint_received) {
        continue;
      }
      break;
    }

    line[strcspn(line, "\n")] = '\0';

    if(line[0] == '\0') {
      continue;
    }

    char cmdline_copy[CMD_LEN];
    strncpy(cmdline_copy, line, CMD_LEN);
    cmdline_copy[CMD_LEN - 1] = '\0';

     
    char *token;
    token = strtok(line, " ");
    int i = 0;
    while(token != NULL && i < MAX_ARGS - 1) {
      argv[i++] = token;
      token = strtok(NULL, " ");

    }
    argv[i] = NULL;

    if(strcmp(argv[0], "exit") == 0) {
      break;
    }else if(strcmp(argv[0], "cd") == 0) {
      if(argv[1] == NULL) {
        fprintf(stderr, "cd: missing arguments");
      } else {
        if (chdir(argv[1]) != 0) {
          perror("cd");
        }
      }
      continue;
    }else if (strcmp(argv[0], "jobs") == 0) {
      print_jobs(jobs);
      continue;
    }
    
    pid_t pid = fork();

    if(pid < 0) {
      perror("fork");
      continue;
    }else if(pid == 0) { 
      setpgid(0, 0);
      signal(SIGINT, SIG_DFL);
      signal(SIGTSTP, SIG_DFL);
      signal(SIGTTIN, SIG_DFL);
      signal(SIGTTOU, SIG_DFL);

      execvp(argv[0], argv);
      perror("execvp");
      return 1;
    }else {
      setpgid(pid, pid);
      tcsetpgrp(STDIN_FILENO, pid);
      int status;

      while(1){
        pid_t w = waitpid(pid, &status, WUNTRACED);

        if(w == -1) {
          if(errno == EINTR) {
            continue;
          }else{
            perror("waitpid");
            break;
          }
        }
        if(WIFEXITED(status) || WIFSIGNALED(status)){
          break;
        }else if(WIFSTOPPED(status)){
          int slot = find_free_job_slot(jobs);

          if (slot == -1) {
            fprintf(stderr, "job table full\n");
          } else {
            jobs[slot].active  = 1;
            jobs[slot].jobid   = next_job_id++;
            jobs[slot].pgid    = pid;
            jobs[slot].state   = JOB_STOPPED;
            strncpy(jobs[slot].command, cmdline_copy, CMD_LEN);
            jobs[slot].command[CMD_LEN - 1] = '\0';
          
            printf("[%d]  Stopped  %s\n", jobs[slot].jobid, jobs[slot].command);
          }
 
          break;
        }
      }

      tcsetpgrp(STDIN_FILENO, shell_pid);
    }

  }

  return 0;
}
