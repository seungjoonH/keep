#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>

#define INF_PM INT_MAX
#define PM_LEN 10

typedef enum {
  NO_ERROR, PM_UNMATCH, 
  KP_EXST, STAT_FAIL, FILE_OPN,
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
  0x0, "unmatched number of parameter",
  "already managed by keep",
  "failed to access stat",
  "file open error",
};

char **ignore;
int ignoreLen;

char cmdStr[10], *params[PM_LEN];
Command cmd;

typedef struct {
  char *filename;
  time_t modtime;
} FileData;

FileData *fileList;
int listLen = 0;

FileData *tracking;
int trackLen = 0;

int min(int x, int y);
int max(int x, int y);
void terminate(ErrorType err);
int getPmNum(char *cmdStr);
void visParams(char **params);
void getCommand(int argc, char **argv);
void freeCommand();

void init();

void listFiles(char *filepath);
void listDirs(char *dirpath);
void loadTrackingfiles();
void saveTrackingfiles();
void track();
void untrack();

void getIgnores();
bool beIgnored(char *path);

int main(int argc, char **argv) {
  ErrorType err = NO_ERROR;

  getIgnores();
  if (argc == 1) terminate(PM_UNMATCH);
  getCommand(argc, argv);

  switch (cmd) {
    case INIT: init(); break;
    case TRACK: track(); break;
    case UNTRACK: untrack(); break;
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
    for (int i = 0; i < min(argc - 2, PM_LEN); i++) {
      if (argv[2 + i][strlen(argv[2 + i]) - 1] == '/') {
        argv[2 + i][strlen(argv[2 + i]) - 1] = '\0';
        strcpy(params[i], argv[2 + i]);
      }
      if (strncmp(argv[2 + i], "./", 2))
        strcpy(params[i], argv[2 + i]);
      else strcpy(params[i], argv[2 + i] + 2);
    }
  }
}

void freeCommand() {
  for (int i = 0; i < PM_LEN; i++) free(params[i]);
}

void init() {
  struct stat st;

  bool alreadExist = !stat(".keep", &st) && S_ISDIR(st.st_mode);

  if (alreadExist) terminate(KP_EXST);
  else {
    int result = mkdir(".keep", 0700);

    FILE *lvfp = fopen(".keep/latest-version", "w");
    FILE *tffp = fopen(".keep/tracking-files", "w");
    fprintf(lvfp, "0");

    fclose(tffp);
    fclose(lvfp);
  }
}

void loadTrackingFiles() {
  FILE *tffp = fopen(".keep/tracking-files", "r");
  if (!tffp) return;

  char line[1000];
  FileData *arr = NULL;
  int count = 0;

  while (fgets(line, 1000, tffp)) {
    if (line[strlen(line) - 1] == '\n')
      line[strlen(line) - 1] = '\0';

    char *name = strtok(line, " ");
    char *time = strtok(NULL, " ");

    FileData *temp = realloc(arr, (count + 1) * sizeof(FileData));

    arr = temp;
    arr[count].filename = (char *) malloc(strlen(name) * sizeof(char));
    strcpy(arr[count].filename, name);
    arr[count++].modtime = (time_t) atoi(time);
  }

  tracking = arr;
  trackLen = count;

  fclose(tffp);
}

void saveTrackingFiles() {
  FILE *tffp = fopen(".keep/tracking-files", "w");
  if (!tffp) return;

  for (int i = 0; i < trackLen; i++)
    fprintf(tffp, "%s %lu\n", tracking[i].filename, tracking[i].modtime);

  fclose(tffp);
}

void listFiles(char *filepath) {
  struct stat st;
  stat(filepath, &st);

  if (!beIgnored(filepath)) {
    char *newStr = (char *) malloc(strlen(filepath) + 1);
    strncpy(newStr, filepath, strlen(filepath) + 1);
    FileData *temp = realloc(fileList, (listLen + 1) * sizeof(FileData));

    fileList = temp;
    fileList[listLen].filename = newStr + 2 * !strncmp(newStr, "./", 2);
    fileList[listLen++].modtime = st.st_mtime;
  }
}

void listDirs(char *dirpath) {
  DIR * dir = opendir(dirpath);

	if (dir == 0x0) return;

	for (struct dirent *i = readdir(dir); i; i = readdir(dir)) {
		if (i->d_type != DT_DIR && i->d_type != DT_REG) continue;

		char *filepath = (char *) malloc(strlen(dirpath) + 1 + strlen(i->d_name) + 1);
		strcpy(filepath, dirpath);
		strcpy(filepath + strlen(dirpath), "/");
		strcpy(filepath + strlen(dirpath) + 1, i->d_name);

    bool pass = false;

    switch (i->d_type) {
      case DT_DIR:
        pass |= !strcmp(i->d_name, ".");
        pass |= !strcmp(i->d_name, "..");
        pass |= beIgnored(filepath);
        if (!pass) listDirs(filepath);
        break;
        
      case DT_REG: listFiles(filepath); break;
      default: break;
    }

		free(filepath);
	}

	closedir(dir);
}

void track() {
  for (int i = 0; i < PM_LEN; i++) {
    if (!strcmp(params[i], "")) break;
    struct stat st;
    if (stat(params[i], &st)) terminate(STAT_FAIL);
    if (S_ISREG(st.st_mode)) listFiles(params[i]);
    else if (S_ISDIR(st.st_mode)) listDirs(params[i]);
  }

  loadTrackingFiles();

  for (int i = 0; i < listLen; i++) {
    bool isExist = false;
    for (int j = 0; j < trackLen; j++) {
      if (!strcmp(fileList[i].filename, tracking[j].filename)) {
        tracking[j].modtime = fileList[i].modtime; 
        isExist = true; break;
      }
    }

    if (!isExist) {
      FileData *temp = realloc(tracking, (trackLen + 1) * sizeof(FileData));

      tracking = temp;
      tracking[trackLen].filename = fileList[i].filename;
      tracking[trackLen++].modtime = 0;
    }
  }

  saveTrackingFiles();
}

void untrack() {
  for (int i = 0; i < PM_LEN; i++) {
    if (!strcmp(params[i], "")) break;
    struct stat st;
    if (stat(params[i], &st)) terminate(STAT_FAIL);
    if (S_ISREG(st.st_mode)) listFiles(params[i]);
    else if (S_ISDIR(st.st_mode)) listDirs(params[i]);
  }

  loadTrackingFiles();

  FileData *tempTracking = (FileData *) malloc(sizeof(FileData));
  int count = 0;

  for (int i = 0; i < trackLen; i++) {
    bool isExist = false;
    for (int j = 0; j < listLen; j++) {
      if (strcmp(tracking[i].filename, fileList[j].filename)) continue;
      isExist = true; break;
    }

    if (!isExist) {
      FileData *temp = realloc(tempTracking, (count + 1) * sizeof(FileData));
      tempTracking = temp;
      tempTracking[count].filename = tracking[i].filename;
      tempTracking[count++].modtime = tracking[i].modtime;
    }
  }

  tracking = tempTracking;
  trackLen = count;

  saveTrackingFiles();
}


void getIgnores() {
  FILE *file = fopen(".keepIgnore", "r");
  char **arr;

  int count = 0;
  char line[1000];

  while (fgets(line, 1000, file)) {
    if (line[strlen(line) - 1] == '\n')
      line[strlen(line) - 1] = '\0';
    
    char *newStr = (char *) malloc(strlen(line) + 2);
    sprintf(newStr, "./%s", line);
    char **temp = realloc(arr, (count + 1) * sizeof(char *));

    arr = temp;
    arr[count++] = newStr;
  }

  fclose(file);

  ignore = arr;
  ignoreLen = count;
}

bool beIgnored(char *path) {
  for (int i = 0; i < ignoreLen; i++) {
    char *temp = (char *) malloc((strlen(path) + 2) * sizeof(char));
    if (strncmp(path, "./", 2)) sprintf(temp, "./%s", path);
    else strcpy(temp, path);
    if (!strncmp(ignore[i], temp, strlen(ignore[i]))) return true;
    free(temp);
  }
  return false;
}