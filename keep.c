/* VIDEO LINK */
// 

/* TEAM MEMBER */
// Hyeon Seungjoon (21800788)
// Song San (22000375)

/* PREPROCESSORS */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>
#include <math.h>

#define INF_PM INT_MAX
#define PM_LEN 10

/* ENUMS */
typedef enum {
  NO_ERROR, NO_KEEP, KP_EXST, 
  PM_UNMATCH, STAT_FAIL, FILE_OPN,
  NO_UPDT, MT_NOTE, VER_NF, ERR_LEN
} ErrorType;

typedef enum {
  INIT, TRACK, UNTRACK, STORE, 
  RESTORE, VERSIONS, CMD_LEN
} Command;

/* STRUCTURE */
typedef struct {
  char *filename;
  time_t modtime;
} FileData;

/* GLOBAL VARIABLES */
int pmNums[CMD_LEN] = { 0, INF_PM, INF_PM, 1, 1, 0 };
char *cmds[CMD_LEN] = {
  "init", "track", "untrack",
  "store", "restore", "versions"
};

char *errMsg[ERR_LEN] = {
  0x0, "not in a keep directory",
  "already in a keep directory",
  "unmatched number of parameter",
  "failed to access stat",
  "file open error",
  "nothing to update",
  "note is empty",
  "version not found",
};

char **ignore;
int ignoreLen;

char cmdStr[10], *params[PM_LEN];
Command cmd;

FileData *fileList;
int listLen = 0;

FileData *tracking;
int trackLen = 0;

unsigned int currentVersion;

/* FUNCTION PROTOTYPES */
// utility functions
int intlen(int i);
int min(int x, int y);
int max(int x, int y);

// file management functions
void loadFileList(char *path);
void listFiles(char *filepath);
void listDirs(char *dirpath);
char *getDir(char *path);
void copyFile(char *des, char *src);

// keep basic functions
void terminate(ErrorType err);
int getPmNum(char *cmdStr);
void visParams(char **params);
void getIgnores();
bool beIgnored(char *path);
void getCommand(int argc, char **argv);
void freeCommand();
void loadTrackingFiles();
void saveTrackingFiles();
void loadCurrentVersion();
void saveCurrentVersion();
void saveNote(char *msg);

// keep main functions
void init();
void track();
void untrack();
void store();
void restore();
void versions();

/* MAIN FUNCTION */
int main(int argc, char **argv) {
  ErrorType err = NO_ERROR;

  if (argc == 1) terminate(PM_UNMATCH);

  if (strcmp(argv[1], "init")) {
    DIR *dir = opendir(".keep");
    if (!dir) terminate(NO_KEEP);
    closedir(dir);
  }

  getIgnores();
  getCommand(argc, argv);

  switch (cmd) {
    case INIT: init(); break;
    case TRACK: track(); break;
    case UNTRACK: untrack(); break;
    case STORE: store(); break;
    case RESTORE: restore(); break;
    case VERSIONS: versions(); break;
    default: break;
  }

  freeCommand();
  return err;
}

/* FUNCTIONS */
// utility functions
int intlen(int i) { return 1 + (i ? (int) log10(i) : 1); }
int min(int x, int y) { return x < y ? x : y; }
int max(int x, int y) { return x > y ? x : y; }

// file management functions
void loadFileList(char *path) {
  struct stat st;
  if (stat(path, &st)) terminate(STAT_FAIL);
  if (S_ISREG(st.st_mode)) listFiles(path);
  else if (S_ISDIR(st.st_mode)) listDirs(path);
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

char *getDir(char *path) {
  char *dir = malloc(strlen(path) * sizeof(char));

  char *lastSlash = strrchr(path, '/');
  if (lastSlash != NULL) {
    size_t length = lastSlash - path + 1;
    strncpy(dir, path, length);
    dir[length] = '\0';
  } else dir[0] = '\0';

  return dir;
}

void copyFile(char *des, char *src) {
  char *desdir = getDir(des);
  mkdir(desdir, 0755);

  struct stat st;
  stat(src, &st);

  FILE* srcfp = fopen(src, "rb");
  FILE* desfp = fopen(des, "wb");

  if (!srcfp || !desfp) terminate(FILE_OPN);

  char buf[BUFSIZ];
  size_t read;

  while ((read = fread(buf, 1, sizeof(buf), srcfp)) > 0) {
    fwrite(buf, 1, read, desfp);
  }

  free(desdir);
  fclose(srcfp);
  fclose(desfp);
}

// keep basic functions
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

  char **temp = realloc(arr, (count + 1) * sizeof(char *));
  arr = temp;
  arr[count++] = "./.keep";

  fclose(file);

  ignore = arr;
  ignoreLen = count;
}

bool beIgnored(char *path) {
  for (int i = 0; i < ignoreLen; i++) {
    char *temp = (char *) malloc((strlen(path) + 2) * sizeof(char));
    if (strncmp(path, "./", 2)) sprintf(temp, "./%s", path);
    else strcpy(temp, path);

    struct stat st;
    stat(ignore[i], &st);
    
    bool cond = !strncmp(ignore[i], temp, strlen(ignore[i]));
    if (temp[strlen(ignore[i]) - 1] != '/') cond = !strcmp(ignore[i], temp);

    if (cond) return true;

    free(temp);
  }
  return false;
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

void loadCurrentVersion() {
  FILE *fp = fopen(".keep/latest-version", "r");
  
  char versionStr[100];
  fscanf(fp, "%s", versionStr);

  currentVersion = (unsigned int) atoi(versionStr);
  
  fclose(fp);
}

void saveCurrentVersion() {
  FILE *fp = fopen(".keep/latest-version", "w");
  fprintf(fp, "%d", currentVersion + 1);
  fclose(fp);
}

void saveNote(char *msg) {
  int verlen = strlen(".keep//note") + intlen(currentVersion + 1);
  char path[verlen + 1];
  sprintf(path, ".keep/%d/note", currentVersion + 1);
  
  FILE *fp = fopen(path, "w");
  if (!fp) terminate(FILE_OPN);

  fprintf(fp, "%s", msg);
  fclose(fp);
}

// keep main functions
void init() {
  struct stat st;

  bool alreadExist = !stat(".keep", &st) && S_ISDIR(st.st_mode);

  if (alreadExist) terminate(KP_EXST);
  else {
    int result = mkdir(".keep", 0755);

    FILE *lvfp = fopen(".keep/latest-version", "w");
    FILE *tffp = fopen(".keep/tracking-files", "w");
    fprintf(lvfp, "0");

    fclose(tffp);
    fclose(lvfp);
  }
}

void track() {
  for (int i = 0; i < PM_LEN; i++) {
    if (!strcmp(params[i], "")) break;
    loadFileList(params[i]);
  }

  loadTrackingFiles();

  for (int i = 0; i < listLen; i++) {
    bool isExist = false;
    for (int j = 0; j < trackLen; j++) {
      if (!strcmp(fileList[i].filename, tracking[j].filename)) {
        if (!tracking[j].modtime) tracking[j].modtime = 0; 
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
    loadFileList(params[i]);
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

void store() {
  if (!strlen(params[0])) terminate(MT_NOTE);

  loadCurrentVersion();
  loadTrackingFiles();

  FileData *arr = (FileData *) malloc(trackLen * sizeof(FileData));
  int count = 0;

  bool update = false;
  
  for (int i = 0; i < trackLen; i++) {
    struct stat st;
    bool exist = !stat(tracking[i].filename, &st);
  
    if (!exist) { update = true; continue; }
    update |= !tracking[i].modtime || tracking[i].modtime < st.st_mtime;
    
    arr[count].filename = malloc((strlen(tracking[i].filename) + 1) * sizeof(char));
    strcpy(arr[count].filename, tracking[i].filename);
    arr[count++].modtime = tracking[i].modtime;
  }

  tracking = arr;
  trackLen = count;

  if (!update) terminate(NO_UPDT);

  int verlen = intlen(currentVersion + 1);

  char *desdir = (char *) malloc((strlen(".keep/") + verlen) * sizeof(char));  
  sprintf(desdir, ".keep/%d", currentVersion + 1);
  
  mkdir(desdir, 0755);
  
  char *tgtdir = (char *) malloc((strlen(".keep//target") + verlen) * sizeof(char));
  sprintf(tgtdir, ".keep/%d/target", currentVersion + 1);
  mkdir(tgtdir, 0755);
  
  for (int i = 0; i < trackLen; i++) {
    int slen = strlen(tgtdir) + strlen(tracking[i].filename);
    char *target = (char *) malloc(slen * sizeof(char));
  
    sprintf(target, "%s/%s", tgtdir, tracking[i].filename);
    copyFile(target, tracking[i].filename);
  
    free(target);
  }

  for (int i = 0; i < trackLen; i++) {
    struct stat st;
    stat(tracking[i].filename, &st);
    tracking[i].modtime = st.st_mtime;
    printf("%s %lu\n", tracking[i].filename, tracking[i].modtime);
  }

  printf("stored as version %d\n", currentVersion + 1);

  saveTrackingFiles();

  char *src = ".keep/tracking-files";
  char *des = (char *) malloc((strlen(src) + verlen + 1) * sizeof(char));
  sprintf(des, "%s/tracking-files", desdir);

  copyFile(des, src);  
  saveNote(params[0]);

  saveCurrentVersion();
  
  free(desdir);
  free(des);
}

void restore() {
  loadCurrentVersion();
  int ver = atoi(params[0]);
  if (ver < 1 || ver > currentVersion) terminate(VER_NF);

  loadFileList(".");
  
  char name[50];
  sprintf(name, ".keep/%d/tracking-files", ver);
  copyFile(".keep/tracking-files", name);
  loadTrackingFiles();

  for (int i = 0; i < listLen; i++) {
    char *dir = getDir(fileList[i].filename);
    remove(fileList[i].filename); remove(dir);
  }

  for (int i = 0; i < trackLen; i++) {
    sprintf(name, ".keep/%d/target/%s", ver, tracking[i].filename);
    struct stat st;
    bool exist = !stat(name, &st);
    if (exist) copyFile(tracking[i].filename, name);
  }

  printf("restored as version %d\n", ver);

}

void versions() {
  loadCurrentVersion();

  for (int i = 0; i < currentVersion; i++) {
    char name[50], note[50];
    sprintf(name, ".keep/%d/note", i + 1);
    FILE *fp = fopen(name, "r");
    fscanf(fp, "%[^\n]", note);
    printf("%-2d %s\n", i + 1, note);
    fclose(fp);
  }
}