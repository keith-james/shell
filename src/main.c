#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

#include "jobs.h"
#include "signals.h"

#define MAX_ARGS 64

int main() {

  init_signals();
  pid_t shell_pid = getpid();
  setpgid(shell_pid, shell_pid);
  tcsetpgrp(STDIN_FILENO, shell_pid);

  job jobs[MAX_JOBS];
  int next_job_id = 1;

  init_jobs(jobs);

  char line[1000]; 
  char *argv[MAX_ARGS];
  
  while(1) {

    printf("\n> ");
    reset_sigint_flag();

    fflush(stdout);
    if(fgets(line, sizeof(line), stdin) == NULL) {
      if(was_sigint_received()) {
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

    int background = 0;

    if(i > 0 && strcmp(argv[i-1], "&") == 0) {
      background = 1;
      argv[i-1] = NULL;
    }

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
    }else if (strcmp(argv[0], "fg") == 0) {
      long val;
      if(argv[1] == NULL) {
        fprintf(stderr, "fg: missing arguments");
        continue;
      } else {
        char *endptr;
        errno = 0;
        val = strtol(argv[1], &endptr, 10);

        if(errno == ERANGE) {
          fprintf(stderr, "Overflow");
          continue;
        }
        if(endptr == argv[1]){
          fprintf(stderr, "Invalid argument");
          continue;
        }else if(*endptr != '\0') {
          fprintf(stderr, "Invalid argument");
          continue;
        }else if (val <= 0) {
          fprintf(stderr, "Invalid argument");
          continue;
        }
      }
      int i = (int) val;
      int index = find_job_by_id(jobs, i);
      if(index == -1) {
        fprintf(stderr, "Invalid job");
        continue;
      }
      pid_t pgid = jobs[index].pgid;
      tcsetpgrp(STDIN_FILENO, pgid);
      if (jobs[index].state == JOB_STOPPED) {
        kill(-pgid, SIGCONT);
        jobs[index].state = JOB_RUNNING;
      }
      int status;
      while(1) {
        pid_t w = waitpid(-pgid, &status, WUNTRACED);

        if(w == -1) {
          if(errno == EINTR) {
            continue;
          }else{
            perror("waitpid");
            break;
          }
        }
        if(WIFEXITED(status) || WIFSIGNALED(status)){
          remove_job(jobs, index);
          break;
        }else if(WIFSTOPPED(status)){
          jobs[index].state = JOB_STOPPED;
          printf("[%d]  Stopped  %s\n", jobs[index].jobid, jobs[index].command);
 
          break;
        }
      }
      tcsetpgrp(STDIN_FILENO, shell_pid);
      continue;
    }else if(strcmp(argv[0], "bg") == 0) {
      long val;
      if(argv[1] == NULL) {
        fprintf(stderr, "bg: missing arguments");
        continue;
      } else {
        char *endptr;
        errno = 0;
        val = strtol(argv[1], &endptr, 10);

        if(errno == ERANGE) {
          fprintf(stderr, "Overflow");
          continue;
        }
        if(endptr == argv[1]){
          fprintf(stderr, "Invalid argument");
          continue;
        }else if(*endptr != '\0') {
          fprintf(stderr, "Invalid argument");
          continue;
        }else if (val <= 0) {
          fprintf(stderr, "Invalid argument");
          continue;
        }
      }
      int i = (int) val;
      int index = find_job_by_id(jobs, i);
      if(index == -1) {
        fprintf(stderr, "Invalid job");
        continue;
      }
      pid_t pgid = jobs[index].pgid;
      if (jobs[index].state == JOB_STOPPED) {
        jobs[index].state = JOB_RUNNING;
        kill(-pgid, SIGCONT);
      }
      printf("[%d]  Running  %s\n", jobs[index].jobid, jobs[index].command);
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
      if(background == 1) {
        int slot = add_job(jobs, next_job_id++, pid, JOB_RUNNING, cmdline_copy);
        if(slot == -1) {
          fprintf(stderr, "job table full\n");
        }
        continue;
      }
      tcsetpgrp(STDIN_FILENO, pid);
      int status;

      while(1){
        pid_t w = waitpid(-pid, &status, WUNTRACED);

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
          int slot = add_job(jobs, next_job_id++, pid, JOB_STOPPED, cmdline_copy);
          if (slot == -1) {
            fprintf(stderr, "job table full\n");
          }
 
          break;
        }
      }

      tcsetpgrp(STDIN_FILENO, shell_pid);
    }

  }

  return 0;
}
