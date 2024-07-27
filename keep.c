/* VIDEO LINK */
// https://youtu.be/gySz7dYSMg0

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
  NO_UPDT, MT_NOTE, VER_NF, 
  STORE_FST, ERR_LEN
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
  "modified file exists",
};

char **ignore;
int ignoreLen;

char cmdStr[10], *params[PM_LEN];
Command cmd;

FileData *fileList;
int listLen = 0;

FileData *tracking;
int trackLen = 0;

unsigned int latestVersion;

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
void getIgnores();
bool beIgnored(char *path);
void getCommand(int argc, char **argv);
void freeCommand();
void loadTrackingFiles();
void saveTrackingFiles();
void loadLatestVersion();
void saveLatestVersion();
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

  fileList = malloc(1000 * sizeof(FileData));
  tracking = malloc(1000 * sizeof(FileData));

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
  free(fileList);
  free(tracking);

  return err;
}

/// @brief Calculates the number of digits in a given integer 
///        when it is converted to a string
/// @param i The integer to calculate the number of digits for
/// @return The number of digits in the integer as an integer value
int intlen(int i) { return 1 + (i ? (int) log10(i) : 1); }

/// @brief Returns the smaller of two integers
/// @param x The first integer to compare
/// @param y The second integer to compare
/// @return The smaller of the two integers
int min(int x, int y) { return x < y ? x : y; }

/// @brief Returns the smaller of two integers
/// @param x The first integer to compare
/// @param y The second integer to compare
/// @return The bigger of the two integers
int max(int x, int y) { return x > y ? x : y; }


/// @brief Loads and lists files or directories based on the given path
/// @param path The path to a file or directory
void loadFileList(char *path) {
  struct stat st;
  if (stat(path, &st)) terminate(STAT_FAIL);
  if (S_ISREG(st.st_mode)) listFiles(path);
  else if (S_ISDIR(st.st_mode)) listDirs(path);
}

/// @brief Adds a file to the fileList if it should not be ignored
/// @param filepath The path to the file to be listed
void listFiles(char *filepath) {
  struct stat st;
  stat(filepath, &st);

  if (!beIgnored(filepath)) {
    char *newStr = (char *) malloc(strlen(filepath) + 1);
    strncpy(newStr, filepath, strlen(filepath) + 1);
    fileList[listLen].filename = newStr + 2 * !strncmp(newStr, "./", 2);
    fileList[listLen++].modtime = st.st_mtime;
  }
}

/// @brief Lists the files and subdirectories within a directory
/// @param dirpath The path to the directory to list
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

/// @brief Extracts the directory part of a given path
/// @param path The path to extract the directory from
/// @return The directory part of the path
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

/// @brief Copies the contents of one file to another
/// @param des The destination directory path
/// @param src The source file path
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


/// @brief Terminates the program with an error message
/// @param err The error type
void terminate(ErrorType err) {
  fprintf(stderr, "[ERROR] %s\n", errMsg[err]);
  exit(err);
}

/// @brief Gets the number of parameters for a given command
/// @param cmdStr The command string
/// @return The number of parameters for the command
int getPmNum(char *cmdStr) {
  for (int i = 0; i < CMD_LEN; i++)
    if (!strcmp(cmds[i], cmdStr)) { cmd = i; return pmNums[i]; }
  return -1;
}

/// @brief Loads ignore patterns from a file
/// This function reads ignore patterns from the `.keepignore` file and other default patterns, and stores them in the `ignore` array
void getIgnores() {
  FILE *file = fopen(".keepignore", "r");
  char **arr = (char **) malloc(1000 * sizeof(char *));

  int count = 0;
  char line[1000];

  if (file) { 
    while (fgets(line, 1000, file)) {
      if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';
      
      char *newStr = (char *) malloc(strlen(line) + 2);
      sprintf(newStr, "./%s", line);
      arr[count++] = newStr;
    }

    fclose(file);
  }

  arr[count++] = "./.keep";
  arr[count++] = "./.git";
  arr[count++] = "./keep.c";
  arr[count++] = "./keep";

  ignore = arr;
  ignoreLen = count;
}

/// @brief Checks if a path should be ignored based on ignore patterns
/// @param path The path to check
/// @return True if the path should be ignored, otherwise false
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

/// @brief Parses command line arguments and sets the command and parameters
/// @param argc The argument count
/// @param argv The argument vector
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

/// @brief Frees the memory allocated for command parameters
void freeCommand() {
  for (int i = 0; i < PM_LEN; i++) free(params[i]);
}

/// @brief Loads tracking files from a file into the tracking array
void loadTrackingFiles() {
  FILE *tffp = fopen(".keep/tracking-files", "r");
  if (!tffp) return;

  char line[1000];
  FileData *arr = malloc(1000 * sizeof(FileData));
  int count = 0;

  while (fgets(line, 1000, tffp)) {
    if (line[strlen(line) - 1] == '\n')
      line[strlen(line) - 1] = '\0';

    char *name = strtok(line, " ");
    char *time = strtok(NULL, " ");

    arr[count].filename = (char *) malloc(strlen(name) * sizeof(char));
    strcpy(arr[count].filename, name);
    arr[count++].modtime = (time_t) atoi(time);
  }

  tracking = arr;
  trackLen = count;

  fclose(tffp);
}

/// @brief Saves tracking files from the tracking array into a file
void saveTrackingFiles() {
  FILE *tffp = fopen(".keep/tracking-files", "w");
  if (!tffp) return;

  for (int i = 0; i < trackLen; i++)
    fprintf(tffp, "%s %lu\n", tracking[i].filename, tracking[i].modtime);

  fclose(tffp);
}

/// @brief Loads the latest version number from a file
void loadLatestVersion() {
  FILE *fp = fopen(".keep/latest-version", "r");
  
  char versionStr[100];
  fscanf(fp, "%s", versionStr);

  latestVersion = (unsigned int) atoi(versionStr);
  
  fclose(fp);
}

/// @brief Saves the latest version number to a file, incrementing it by one
void saveLatestVersion() {
  FILE *fp = fopen(".keep/latest-version", "w");
  fprintf(fp, "%d", latestVersion + 1);
  fclose(fp);
}

/// @brief Saves a note for the current version
/// @param msg The note message to save
void saveNote(char *msg) {
  int verlen = strlen(".keep//note") + intlen(latestVersion + 1);
  char path[verlen + 1];
  sprintf(path, ".keep/%d/note", latestVersion + 1);
  
  FILE *fp = fopen(path, "w");
  if (!fp) terminate(FILE_OPN);

  fprintf(fp, "%s", msg);
  fclose(fp);
}

/// @brief Initializes the .keep directory and files if they do not exist
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

/// @brief Tracks the specified files by adding them to the tracking list
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
      tracking[trackLen].filename = fileList[i].filename;
      tracking[trackLen++].modtime = 0;
    }
  }

  saveTrackingFiles();
}

/// @brief Untracks the specified files by removing them from the tracking list
void untrack() {
  for (int i = 0; i < PM_LEN; i++) {
    if (!strcmp(params[i], "")) break;
    loadFileList(params[i]);
  }
  
  loadTrackingFiles();

  FileData *tempTracking = (FileData *) malloc(1000 * sizeof(FileData));
  int count = 0;

  for (int i = 0; i < trackLen; i++) {
    bool isExist = false;
    for (int j = 0; j < listLen; j++) {
      if (strcmp(tracking[i].filename, fileList[j].filename)) continue;
      isExist = true; break;
    }

    if (!isExist) {
      tempTracking[count].filename = tracking[i].filename;
      tempTracking[count++].modtime = tracking[i].modtime;
    }
  }

  tracking = tempTracking;
  trackLen = count;

  saveTrackingFiles();
}

/// @brief Stores the tracked files into the .keep directory as a new version
void store() {
  if (!strlen(params[0])) terminate(MT_NOTE);

  loadLatestVersion();
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

  int verlen = intlen(latestVersion + 1);

  char *desdir = (char *) malloc((strlen(".keep/") + verlen) * sizeof(char));  
  sprintf(desdir, ".keep/%d", latestVersion + 1);
  
  mkdir(desdir, 0755);
  
  char *tgtdir = (char *) malloc((strlen(".keep//target") + verlen) * sizeof(char));
  sprintf(tgtdir, ".keep/%d/target", latestVersion + 1);
  mkdir(tgtdir, 0755);
  
  for (int i = 0; i < trackLen; i++) {
    int slen = strlen(tgtdir) + strlen(tracking[i].filename);
    char *target = (char *) malloc(1000 * sizeof(char));
  
    sprintf(target, "%s/%s", tgtdir, tracking[i].filename);
    copyFile(target, tracking[i].filename);
  
    free(target);
  }

  for (int i = 0; i < trackLen; i++) {
    struct stat st;
    stat(tracking[i].filename, &st);
    tracking[i].modtime = st.st_mtime;
  }

  printf("stored as version %d\n", latestVersion + 1);

  saveTrackingFiles();

  char *src = ".keep/tracking-files";
  char *des = (char *) malloc((strlen(src) + verlen + 1) * sizeof(char));
  sprintf(des, "%s/tracking-files", desdir);

  copyFile(des, src);  
  saveNote(params[0]);

  saveLatestVersion();
  
  free(desdir);
  free(des);
}

/// @brief Restores the tracked files to a specified version
/// @param version The version number to restore
void restore() {
  loadLatestVersion();

  int ver = atoi(params[0]);
  if (ver < 1 || ver > latestVersion) terminate(VER_NF);

  loadFileList(".");

  loadTrackingFiles();

  bool modified = false;
  for (int i = 0; i < trackLen; i++) {
    struct stat st;
    stat(tracking[i].filename, &st);
    modified |= tracking[i].modtime < st.st_mtime;
  }

  if (modified) terminate(STORE_FST);
  
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
    struct stat st1, st2;
    bool exist = !stat(name, &st1);
    if (!exist) continue;
    copyFile(tracking[i].filename, name);
    
    stat(tracking[i].filename, &st2);
    tracking[i].modtime = st2.st_mtime;
  }

  saveTrackingFiles();

  printf("restored as version %d\n", ver);

}

/// @brief Displays all stored versions and their notes
void versions() {
  loadLatestVersion();

  for (int i = 0; i < latestVersion; i++) {
    char name[50], note[50];
    sprintf(name, ".keep/%d/note", i + 1);
    FILE *fp = fopen(name, "r");
    fscanf(fp, "%[^\n]", note);
    printf("%-2d %s\n", i + 1, note);
    fclose(fp);
  }
}