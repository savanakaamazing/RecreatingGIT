#ifndef _CLIENTHEADER_H_
#define _CLIENTHEADER_H_

//libraries to import
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <linux/limits.h>

#define TRUE 1
#define FALSE 0

//struct declarations, this one is for the contents of the manifest
struct entry {
    char filePath[PATH_MAX];
    int fileVer;
    char fileHash[SHA_DIGEST_LENGTH];
    struct entry * next;
    struct entry * prev;
};

//global variables
int port;
char IP[100];
int sfd;

//global variables used when editing manifests
struct entry * servManHead;
struct entry * clienManHead;

//functions
int configure(char *, char *);
int checkout(char *);
int readConf();
int readFile(int, char **);
void populateManifest(char *, struct entry **);
int update(char *);
int charComparator (char *, char *);
void insertionSortHelper(struct entry**,struct entry*, int(*comparator)(char *, char *));
int insertionSort(struct entry**, int(*comparator)(char *, char *));
int upgrade(char *);
int commit(char *);
void freeLL(struct entry *);
#endif