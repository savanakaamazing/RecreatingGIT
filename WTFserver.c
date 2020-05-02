/*importing all necessary libraries needed */
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <limits.h>
#include <openssl/sha.h>


int creator (char * name);
int checkers (char * name);
int fyleBiter (int fd, char ** buffer);
int mkdir(const char * pathname, mode_t mode);
int destroyer (DIR * myDirectory, char * currDirec);
int traverser (DIR * myDirectory, int counter, int currSize, char * currDirec);
int writeFile(char * path);

char ** files; // global variable that holds all the files within a directory!

int main (int argc, char ** argv) {
    int lsocket; // declaring the file descriptor for our listening (server) socket
    int csocket; // declaring the file descriptor from the respective client socket
    int caddysize = -1;
    int portnum = atoi(argv[1]);
    char crequest[6]; // get requests from clients!
    int manSuc = 1;
    int manFail = -1;
    char sCon[7] = {'f', 'u', 'c', 'k', ' ', 'u', '\0'};
    memset(crequest, '\0', 6);
    lsocket = socket(AF_INET, SOCK_STREAM, 0); // creating the socket
    /* stuff that comprises the server addy */
    struct sockaddr_in serveraddy;
    struct sockaddr_in clientaddy;
    caddysize = sizeof(clientaddy);
    bzero((char*) &serveraddy, sizeof(serveraddy));
    serveraddy.sin_family = AF_INET;
    serveraddy.sin_port = htons(portnum); // port number must be taken from command line
    serveraddy.sin_addr.s_addr = INADDR_ANY;
    bind(lsocket, (struct sockaddr*) &serveraddy, sizeof(serveraddy)); // binding socket to address
    listen(lsocket, 0); // has 0 clients on backlog
    while (((csocket = accept(lsocket, (struct sockaddr *) &clientaddy, (socklen_t *) &caddysize)))) {
      send(csocket, sCon, 7, 0);
    while (recv(csocket, &crequest, 5, 0)) {
      printf("The client has requested the server to: %s\n", crequest);
      if (crequest[3] == 'a') { // this means you know you have to create the project (Will come in as Crea:)
        int projLen = 0;
        recv(csocket, &projLen, sizeof(int), MSG_WAITALL);
        char projName[projLen];
        int creRet; // what creator returns!
        memset(projName, '\0', projLen);
        recv(csocket, projName, projLen, MSG_WAITALL);
        creRet = creator(projName);
        if (creRet == 1) { // the file was created successfully
          send(csocket, &manSuc, sizeof(int), 0); // send 1 to the client
        }
        else { // file already existed yo
          send(csocket, &manFail, sizeof(int), 0);
        }
      }
      else if(crequest[0] == 'F') { // FILE:Path command
          int numBytes; // the number of bytes in a file
          int pfd; // file descriptor for Path
          int pathSize;
          recv(csocket, &pathSize, sizeof(int), 0);
          char pathName[pathSize]; // the path where the file to be opened resides
          char * fileBuf; // storing bytes in a file
          memset(pathName, '\0', pathSize);
          recv(csocket, &pathName, pathSize, MSG_WAITALL);
          pfd = open(pathName, O_RDONLY);
          if(pfd == -1) {
            send(csocket, &manFail, sizeof(int), 0);
          }
          numBytes = fyleBiter(pfd, &fileBuf);
          send(csocket,&numBytes, sizeof(int), 0);
          send(csocket, fileBuf, strlen(fileBuf) + 1, 0);
        }
        else if(crequest[0] == 'D') { // destroying a directory!!
          char dName[30]; // name of
          memset(dName, '\0', 30);
          recv(csocket, &dName, sizeof(dName), MSG_WAITALL); // getting directory to be DESTROYED
          DIR * myDirec = opendir(dName); // making direct struct for the directory to be DESTROYED
          destroyer(myDirec, dName);
          char success[8] = {'s', 'u', 'c', 'c', 'e', 's', 's', '\0'};
          send(csocket, success, sizeof(success), 0);
        }
        else if (crequest[1] == 'h') { // checks to see if file exists
          int returnstat;
          int fileNameS;
          recv(csocket, &fileNameS, sizeof(int), MSG_WAITALL);
          char project[fileNameS];
          memset(project, '\0', fileNameS);
          recv(csocket, &project, sizeof(project), MSG_WAITALL);
          returnstat = checkers(project);
          if(returnstat == 1) { // success!
            send(csocket, &manSuc, sizeof(int), 0);
          }
          else {
            send(csocket, &manFail, sizeof(int), 0);
          }
        }
        else if(crequest[3] == 'j') { // Proj: protocol
          files = (char **) malloc(100 * sizeof(char *));
          char * fileboof; // buffer to store file into
          int nbytes; // number of bytes in a file
          int lenProjName; // length of project name
          int bfg; // file descriptors for each file!
          recv(csocket, &lenProjName, sizeof(int), MSG_WAITALL); // getting lentgh of proj name + 1 from client
          char pject[lenProjName]; // making buffer
          memset(pject, '\0', lenProjName); // presetting it beforehand
          recv(csocket, &pject, lenProjName, MSG_WAITALL); // getting project name from client
          DIR * projDirec = opendir(pject);
          int numfiles; // number of files within said project
          numfiles = traverser(projDirec, 0, 100, pject);
          send(csocket, &numfiles, sizeof(int), 0); // send client number of files
          int i = 0;
          int len = 0;
          char tempPath[PATH_MAX];
          while (i < numfiles) { // traverse files array
            len = strlen(files[i]) + 1;
            send(csocket, &len, sizeof(int), 0);
            send(csocket, files[i], len, 0); // send path of file
            recv(csocket, tempPath, 5, MSG_WAITALL);
            recv(csocket, &len, sizeof(int), MSG_WAITALL);
            recv(csocket, tempPath, len, MSG_WAITALL);
            bfg = open(files[i], O_RDONLY); // open the file
            nbytes = fyleBiter(bfg, &fileboof); // how many bytes in said file
            send(csocket, &nbytes, sizeof(int), 0); // sending number of bytes to client
            printf("%s\n", fileboof);
            len = 0;
            while(len < nbytes) {
              len += send(csocket, fileboof + len, nbytes - len, 0); // sending buffer to client
            }
            i++;
          }
        }
        else if(crequest[3] == 'm') { // commit!!
          int fps; // the file path size including null terminator
          int cfd; // file descriptor for the commit file that will be given to us
          int eb; // expected bytes for the committ
          recv(csocket, &fps, sizeof(int), MSG_WAITALL); // get file path size including null from client and store into fps
          char fcomm[fps]; // making char array that the file path will be loaded into!
          memset(fcomm, '\0', fps); // setting everything to null terminator beforehand
          recv(csocket, fcomm, fps, MSG_WAITALL); // getting the actual file path from the client, includes the hashcode
          cfd = open(fcomm, O_TRUNC | O_CREAT | O_RDWR, S_IRUSR | S_IWUSR); // making the commit file in the proper directory
          recv(csocket, &eb, sizeof(int), MSG_WAITALL); // getting number of bytes contained in committ
          char cbuff[eb]; // making a buffer for the commit, based on bytes in it
          memset(cbuff, '\0', eb); // setting everything in buffer to null terminator
          recv(csocket, cbuff, eb, MSG_WAITALL); // loading the contents of the commit into said buffer
          write(cfd, cbuff, eb - 1); // writing to said commit file the contents, write eb number of bytes(whole cbuff)
        }
        else if(crequest[3] == 'h') { // the push command
          int filePathSize; // file path size including null terminator
          int fcd; // file descriptor of commit
          int md; // manifest file descriptor
          int hd; // file descriptor of history
          int mfv = 0; // manifest file version
          char * comBuf; // the buffer file thatll hold the bytes in the commit!
          char * manBuf; // the buffer thatll hold the bytes in the manifest
          recv(csocket, &filePathSize, sizeof(int), MSG_WAITALL);
          char commfp[filePathSize]; // initializing the buffet thatll hold the file path to the specific commit
          memset(commfp, '\0', filePathSize); // setting them all to 0
          recv(csocket, commfp, filePathSize, MSG_WAITALL); // getting the said file path of the commit!
          fcd = open(commfp, O_RDONLY); // open the commit, with read only permission
          if (fcd == -1) { // file does not exist!
            send(csocket, &manFail, sizeof(int), 0);
          }
          else { // the file exists!
            send(csocket, &manSuc, sizeof(int), 0);
          }
          fyleBiter(fcd, &comBuf); // storing the commit into the buffer 
          char * projDir;
          projDir = strtok(commfp, "."); // getting the project directory!
          char histfp[PATH_MAX]; // file path of the projects history
          memset(histfp, '\0', PATH_MAX); // setting everything to null terminator
          strcpy(histfp, projDir); // adding projdirectory part to history file path
          strcat(histfp, ".History"); // adding history part to history file path
          char manfp[PATH_MAX]; // creating file path for manifest
          memset(manfp, '\0', PATH_MAX); // presetting everything in buffer to null terminator
          strcpy(manfp, projDir); // setting project/ to file path
          strcat(manfp, ".Manifest"); // adding the .Manifest file to it;
          md = open(manfp, O_RDONLY); // open the manifest
          fyleBiter(md, &manBuf); // loading manifest into the buffer
          close(md);
          sscanf(manBuf, "%d\n", &mfv); // getting project version from manifest buffer
          char pvBUF[999];
          memset(pvBUF, '\0', 999);
          sprintf(pvBUF, "%d", mfv); // setting project version into the buffer
          strcat(pvBUF, "\n");
          hd = open(histfp, O_RDWR); // opening the history with the 
          write(hd, pvBUF, strlen(pvBUF)); // writing the manifest version number to the history
          write(hd, comBuf, strlen(comBuf)); // writing the commit to the history!
          while(projDir != NULL){
            projDir = strtok(NULL, ".");
          }
          char * line; // the particular line in the commit
          char * commCopy = malloc(strlen(comBuf)+1);
          memset(commCopy, '\0', strlen(comBuf)+1);
          strcpy(commCopy, comBuf);
          line = strtok(commCopy, "\n"); // tokenizing it by new line
          int i = 0;
          int x = 0;
          char instruction;
          char hash[40+1]; 
          int fileVer;
          int currFD;
          char checksPath[PATH_MAX];
          int fplen;
          int byter; 
          while(line != NULL){
            memset(checksPath, '\0', PATH_MAX);
            sscanf(line, "%c %s %s %d", &instruction, checksPath, hash, &fileVer);
            if(instruction == 'A') {
              recv(csocket, &fplen, sizeof(int), MSG_WAITALL);
              char fpName[fplen]; // creating buffer for file path
              memset(fpName, '\0', fplen);
              recv(csocket, fpName, fplen, MSG_WAITALL);
              writeFile(fpName);
              currFD = open(fpName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR); // create the file with read and write permissions
              recv(csocket, &byter, sizeof(int), MSG_WAITALL); // getting bytes in file
              char fb[byter]; // buffer for the file
              recv(csocket, fb, byter, MSG_WAITALL);
              write(currFD, fb, byter);
            }
            else if(instruction == 'M') {
              recv(csocket, &fplen, sizeof(int), MSG_WAITALL);
              char fpName[fplen]; // creating buffer for file path
              memset(fpName, '\0', fplen);
              recv(csocket, fpName, fplen, MSG_WAITALL);
              currFD = open(fpName, O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR); // open the file but get rid of everything in it since its modified
              recv(csocket, &byter, sizeof(int), MSG_WAITALL); // getting bytes in file
              char fb[byter]; // buffer for the file
              recv(csocket, fb, byter, MSG_WAITALL);
              write(currFD, fb, byter);
            }
            else if(instruction == 'D') { // delete file
              remove(checksPath); // we dont get anything from the client!
            }
            strcpy(commCopy, comBuf);
            line = strtok(commCopy, "\n");
            while(x <= i && line != NULL){
              line = strtok(NULL, "\n");
              x++;
            }
            x = 0;
            i++;
          }

          int mlen;
          int nmd; // new manifest fd
          recv(csocket, &mlen, sizeof(int), MSG_WAITALL);
          char mP[mlen]; // manifest path
          memset(mP, '\0', mlen);
          recv(csocket, mP, mlen, MSG_WAITALL); // populates manifest path
          nmd = open(mP, O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR); // open manifest but also clear it and give read write permission
          int mbytes; // number of bytes in manifest
          recv(csocket, &mbytes, sizeof(int), MSG_WAITALL);
          char manBoof[mbytes];
          memset(manBoof, '\0', mbytes);
          recv(csocket, manBoof, mbytes, MSG_WAITALL); // populate the manifest buffer
          write(nmd, manBoof, mbytes);
        }
      }
    }
    return 0;
}


int creator (char * name) { // will see if the name of the project is there or not, whatever
    char manPath[PATH_MAX];
    char hisPath[PATH_MAX];
    char newMan[1] = {'1'};
    memset(manPath, '\0', PATH_MAX);
    memset(hisPath, '\0', PATH_MAX);
    strcpy(manPath, name);
    strcpy(hisPath, name);
    strcat(manPath, "/.Manifest");
    strcat(hisPath, "/.History" );
    int mfd; // manpage file descriptor
    DIR * proj = opendir(name);
    if (!proj) { // the directory does not exist in this world
      mkdir(name, S_IRWXU); // makes the directory
      mfd = open(manPath, O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR); // creates manifest in directory
      open(hisPath, O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR); // creates the history in the directory also
      write(mfd, newMan, 1);
      return 1;
    }
    else {
      return -1; // the project already existed!!
    } 
}

int checkers (char * name) { // almost like the opposite of create in my opinion, check to see if project exists!
  DIR * proj = opendir(name);
  if(!proj) { // project doesnt exist!
    return -1;
  }
  else {
    return 1; // the project exists!
  }
}

int fyleBiter (int fd, char ** buffer) { // given a file path, it will find how many bytes are within a file!!
  int buffSize = lseek(fd, (size_t)0, SEEK_END);
  *buffer = malloc((buffSize+1)*sizeof(char)); //store the contents of the config file (freed)
  lseek(fd, (size_t)0, SEEK_SET);   //set the offset back to the start
  memset(*buffer, '\0', buffSize+1);   //memset the buffer to null terminators
  read(fd, *buffer, buffSize);   //read the buffsize amount of bytes into the buffer
  return buffSize + 1;   //return the size of the file

}

int destroyer (DIR * myDirectory, char * currDirec) { // takes in a directory that is meant to be deleted
  char filePBuff[PATH_MAX + 1]; //stores the filepath of our subdirectories
  char fylePBuff[PATH_MAX + 1]; // stores filepath of files!!
  strcpy(filePBuff, currDirec); //in the case of recursion, update the filepath so that we do not get lost
  strcat(filePBuff, "/");   //add a forward-slash at the end to get ready to add more to the path
  struct dirent * currDir;   //the dirent struct which holds whatever readdir returns
  while((currDir = readdir(myDirectory)) != NULL) {   //loop through the contents of the directory and store in files array
    //skip the . and .. and dsstore file
    if(strcmp(currDir->d_name, ".") == 0 || strcmp(currDir->d_name, "..") == 0 || strcmp(currDir->d_name,".DS_Store") == 0) {
      continue;
    }
    if(currDir->d_type == DT_DIR) {  //first check if the currdir is a regular file or a directory
      strcat(filePBuff, currDir->d_name); //add the directory in question to the path
      destroyer(opendir(filePBuff), filePBuff);  //traverse the new directory
      strcpy(filePBuff, currDirec);  //we are back in the original file, get rid of the previous file path
      strcat(filePBuff, "/");  //put the forward-slash back in there
      rmdir(filePBuff);
    } 
    else if(currDir -> d_type == DT_REG) {
      strcpy(fylePBuff,filePBuff); //add the file path to the array
      strcat(fylePBuff, currDir->d_name); //store the names of the files in our files array
      remove(fylePBuff);
    }
  }
  return 1;   //return 1
}

int traverser (DIR * myDirectory, int count, int currSize, char * currDirec){ // takes in a directory and gets files!
  char filePBuff[PATH_MAX + 1]; //stores the filepath of our subdirectories
  strcpy(filePBuff, currDirec); //in the case of recursion, update the filepath so that we do not get lost
  strcat(filePBuff, "/"); //add a forward-slash at the end to get ready to add more to the path
  struct dirent * currDir; //the dirent struct which holds whatever readdir returns
  while((currDir = readdir(myDirectory)) != NULL){ //loop through the contents of the directory and store in files array
    if(strcmp(currDir->d_name, ".") == 0 || strcmp(currDir->d_name, "..") == 0 || strcmp(currDir->d_name,".DS_Store") == 0){ //skip the . and .. and dsstore file
      continue; //skip the iteration
    }
    if(currDir->d_type == DT_DIR) { //directory is a directory
      strcat(filePBuff, currDir->d_name); //add the directory in question to the path
      count = traverser(opendir(filePBuff), count, currSize, filePBuff); //traverse the new directory
      strcpy(filePBuff, currDirec); //we are back in the original file, get rid of the previous file path
      strcat(filePBuff, "/"); //put the forward-slash back in there
      currSize = ((count%100)+1)*100; //find the new max size of the array
    } 
    else if(currDir -> d_type == DT_REG){ // directory is a file
      files[count] = (char *)malloc((PATH_MAX+1) * sizeof(char)); //allocate space for the file path
      strcpy(files[count],filePBuff); //add the file path to the array
      strcat(files[count], currDir->d_name); //store the names of the files in our files array
      if(++count >= currSize){   //check if files array needs more space
        //realloc 100 more spaces in our files array
        files = realloc(files, (currSize+100) * sizeof(char *));  //realloc 100 more spaces in our files array
        currSize += 100;  //change the curr size accordingly
      }
    }
  }
  return count; //return the current count
}

int writeFile(char * path){
  /*line: holds the part of the string to append, next is used to make sure that we do not mkdir the final
  piece wich is the file, appendage string holds the parts, file size stores the number of bytes to be read
  server file holds the file that is retrieved from the server*/
  char original[PATH_MAX];
  strcpy(original, path);
  char * notLine = strtok(original, "/");
  char * next = strtok(NULL, "/");
  char appendageString[PATH_MAX];
  memset(appendageString, '\0', PATH_MAX);
  DIR *myDirec;
  //loop through and get all the subdirectories
  while(next != NULL){
    strcat(appendageString, notLine);
    //check is subdirectory exists
    myDirec = opendir(appendageString);
    if(!myDirec){
      mkdir(appendageString, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    } else{
      free(myDirec);
    }
    //traverse
    notLine = next;
    next = strtok(NULL, "/");
    strcat(appendageString, "/");
  }
  strcat(appendageString, notLine);
  return 0;
}