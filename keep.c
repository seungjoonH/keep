#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/stat.h>

#define INF_PM INT_MAX
#define PM_LEN 10

typedef enum {
  NO_ERROR, PM_UNMATCH, 
  DIR_EXST, ERR_LEN
} ErrorType;

typedef enum {
  INIT, TRACK, UNTRACK, STORE, 
  UNSTORE, VERSIONS, CMD_LEN
} Command;

int pmNums[CMD_LEN] = { 0, INF_PM, INF_PM, 1, 1, 0 };
char *cmds[CMD_LEN] = {
  "init", "track", "untrack",
  "store", "restore", "versions"
};

char *errMsg[ERR_LEN] = {
  0x0, "unmatched number of parameter",
  "directory already exists",
};

char cmdStr[10], *params[PM_LEN];
Command cmd;

int min(int x, int y);
int max(int x, int y);
void terminate(ErrorType err);
int getPmNum(char *cmdStr);
void visParams(char **params);
void getCommand(int argc, char **argv);
void freeCommand();

void init();

int main(int argc, char **argv) {
  ErrorType err = NO_ERROR;

  if (argc == 1) terminate(PM_UNMATCH);
  getCommand(argc, argv);

  switch (cmd) {
    case INIT: init(); break;
    default: break;
  }

  freeCommand();

  return err;
}

int min(int x, int y) { return x < y ? x : y; }
int max(int x, int y) { return x > y ? x : y; }

void terminate(ErrorType err) {
  fprintf(stderr, "[ERROR] %s\n", errMsg[err]);
  exit(err);
}

int getPmNum(char *cmdStr) {
  for (int i = 0; i < CMD_LEN; i++)
    if (!strcmp(cmds[i], cmdStr)) { cmd = i; return pmNums[i]; }
  return -1;
}

void visParams(char **params) {
  printf("Param:\n");
  for (int i = 0; i < PM_LEN; i++) printf("%s ", params[i]);
  printf("\n");
}

void getCommand(int argc, char **argv) {
  for (int i = 0; i < PM_LEN; i++) {
    params[i] = (char *) malloc(100);
    params[i][0] = '\0';
  }
  
  strcpy(cmdStr, argv[1]);

  int paramCnt = getPmNum(cmdStr);

  if (paramCnt == INF_PM) {
    if (argc < 3) terminate(PM_UNMATCH);
  }
  else if (argc != paramCnt + 2) terminate(PM_UNMATCH);

  if (paramCnt) {
    for (int i = 0; i < min(argc - 2, PM_LEN); i++)
      strcpy(params[i], argv[2 + i]);
  }
}

void freeCommand() {
  for (int i = 0; i < PM_LEN; i++) free(params[i]);
}

void init() {
  struct stat st;

  bool alreadExist = !stat(".keep", &st) && S_ISDIR(st.st_mode);

  if (alreadExist) terminate(DIR_EXST);
  else {
    int result = mkdir(".keep", 0700);

    FILE *trackingFiles = fopen(".keep/tracking-files", "w");
    FILE *latestVersion = fopen(".keep/latest-version", "w");
    fprintf(latestVersion, "0");

    fclose(trackingFiles);
    fclose(latestVersion);
  }
}