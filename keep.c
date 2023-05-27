#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define INF_PM INT_MAX
#define PM_LEN 10

typedef enum {
  NO_ERROR, PM_UNMATCH,
  ERR_LEN
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
  0x0, "Unmatched number of parameter",
};

int min(int x, int y);
int max(int x, int y);
void terminate(ErrorType err);
int getParam(char *cmd);
void visParams(char **params);

int main(int argc, char **argv) {
  ErrorType err = NO_ERROR;

  if (argc == 1) terminate(PM_UNMATCH);

  char cmd[10], *params[PM_LEN];
  
  for (int i = 0; i < PM_LEN; i++) {
    params[i] = (char *) malloc(100);
    params[i][0] = '\0';
  }
  
  strcpy(cmd, argv[1]);
  int paramCnt = getParam(cmd);

  if (paramCnt == INF_PM) {
    if (argc < 3) terminate(PM_UNMATCH);
  }
  else if (argc != paramCnt + 2) terminate(PM_UNMATCH);

  if (paramCnt) {
    for (int i = 0; i < min(argc - 2, PM_LEN); i++)
      strcpy(params[i], argv[2 + i]);
  }

  visParams(params);

  for (int i = 0; i < PM_LEN; i++) free(params[i]);

  return err;
  
}

int min(int x, int y) { return x < y ? x : y; }
int max(int x, int y) { return x > y ? x : y; }

void terminate(ErrorType err) {
  fprintf(stderr, "[ERROR] %s\n", errMsg[err]);
  exit(err);
}

int getParam(char *cmd) {
  for (int i = 0; i < CMD_LEN; i++)
    if (!strcmp(cmds[i], cmd)) return pmNums[i];
  return -1;
}

void visParams(char **params) {
  for (int i = 0; i < PM_LEN; i++) printf("%s ", params[i]);
  printf("\n");
}