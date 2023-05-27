#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define INF_PM INT_MAX

typedef enum {
  NO_ERROR, PM_UNMATCH,
  ERR_LEN
} ErrorType;

typedef enum {
  INIT, TRACK, UNTRACK, STORE, 
  UNSTORE, VERSIONS, CMD_LEN
} Command;

int params[CMD_LEN] = { 0, INF_PM, INF_PM, 1, 1, 0 };
char *cmds[CMD_LEN] = {
  "init", "track", "untrack",
  "store", "restore", "versions"
};

char *errMsg[ERR_LEN] = {
  0x0, "Unmatched number of parameter",
};

void terminate(ErrorType err);
int getParam(char *cmd);

int main(int argc, char **argv) {
  ErrorType err = NO_ERROR;

  if (argc == 1) terminate(PM_UNMATCH);

  char cmd[10], *param = (char *) malloc(100);
  *param = '\0';  
  
  strcpy(cmd, argv[1]);
  int paramCnt = getParam(cmd);

  if (paramCnt == INF_PM) {
    if (argc < 3) terminate(PM_UNMATCH);
  }
  else {
    if (argc != paramCnt + 2) terminate(PM_UNMATCH);
  }

  if (paramCnt) strcpy(param, argv[2]);
  printf("%s\n", param);

  free(param);

  return err;
  
}

void terminate(ErrorType err) {
  fprintf(stderr, "[ERROR] %s\n", errMsg[err]);
  exit(err);
}

int getParam(char *cmd) {
  for (int i = 0; i < CMD_LEN; i++)
    if (!strcmp(cmds[i], cmd)) return params[i];
  return -1;
}