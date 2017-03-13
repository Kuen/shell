#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <glob.h>



void signal_handler(int signal);
void loop_pipe(char ***cmd);
void runPipedCommands(char ***cmd, int pipescount);


typedef struct jobs{
  char cmd[255];
  pid_t pidList;
  struct jobs *next;
}Jobs;

Jobs* creatNewJob(char cmd[255], pid_t pidList){
  Jobs *newNode = malloc(sizeof(Jobs));
  strcpy(newNode->cmd,cmd);
  newNode->pidList= pidList;
  newNode->next = NULL;
  return newNode;
}

void deleteJob(Jobs* head, pid_t pid){
  Jobs *temp = head;
  Jobs *delPtr;
  while(temp->next->pidList != pid){
    temp = temp->next;
  }
  delPtr = temp->next;
  temp->next = temp->next->next;
  // free(temp->next-> &pidList);
  free(delPtr);

}



Jobs *head = NULL;
int main(int argc, char const *argv[]) {


  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);


  char buf1[255];
  char buf2[255];
  char cwd[PATH_MAX+1];
  char ***commands;
  int argListSize[255];
  char *token;
  int i, j;
  int tokenSize = 0;
  int pipe_position[255];
  int commandscount;
  int pipescount;
  int argcount;


  while (1) {


    char cmd[255];
    i = 0; j = 0;
    commandscount = 0; pipescount = 0; argcount = 0;
    tokenSize = 0;
    if (getcwd(cwd, PATH_MAX+1) != NULL) {
      printf("[3150 shell:%s]$ ", cwd);
    } else {
      printf("[3150 shell:Error Path]$ ");
    }
    fgets(buf1, 255, stdin);             //get input
    buf1[strlen(buf1)-1] = '\0';
    strcpy(buf2, buf1);
    strcpy(cmd, buf1);

    token = strtok(buf1, " ");

    if (token != NULL) {
      while (token != NULL) {
        if (strcmp(token, "|") != 0) {  //normal command
          token = strtok(NULL, " ");
          argcount++;
          argListSize[i] = argcount;
        } else {                      //reach pipe "|"
        pipescount++;
        i++;
        argcount = 0;
        token = strtok(NULL, " ");
      }
    }
    printf("pipescount = %d\n", pipescount);

    commands = (char***) malloc(sizeof(char**) * (pipescount+2));
    for (i = 0; i < pipescount+1; i++) {
      printf("argListSize[%d] = %d\n",i, argListSize[i] );
      commands[i] = (char**)malloc(sizeof(char*)* (argListSize[i]+1));
    }

    i=0; j=0;
    token = strtok(buf2, " ");

    while (token != NULL) {
      int index = 0;
      if (strcmp(token, "|") != 0) {

        glob_t results;
        char *input = (char*)malloc(sizeof(char)*strlen(token)+1);
        strcpy(input, token);
        glob("this is NULL", GLOB_NOCHECK, NULL, &results);
        glob(input, GLOB_NOCHECK | GLOB_APPEND, NULL, &results);
        commands[i] = (char**)realloc(commands[i], sizeof(char*)*(results.gl_pathc+1));
        for (index = 1; index < results.gl_pathc; index++) {
          commands[i][j] = (char*)malloc(sizeof(char)*(strlen(results.gl_pathv[index])+1));
          strcpy(commands[i][j],results.gl_pathv[index]);
          j++;
          commands[i][j] = NULL;
        }
        globfree(&results);

        // commands[i][j] = (char*)malloc(sizeof(char)*(strlen(token)+1));
        // strcpy(commands[i][j], token);
        // printf("%s\n",commands[i][j] );
        token = strtok(NULL, " ");
        // j++;
        // commands[i][j] = NULL;       // make sure the end of the argList is NULL
      } else {
        token = strtok(NULL, " ");
        i++;
        j=0;
      }
    }

    /************************Process command part************************/

    if (pipescount == 0) {
      if (strcmp(**commands, "exit") == 0) {   //exit the shell
        if (commands[0][1] != NULL) {
          printf("exit: wrong number of arguments\n");
        } else {
          printf("logout\n\n");
          printf("[Process completed]\n");
          break;  //exit the loop
        }

      } else if (strcmp(**commands, "cd") == 0) {   //change the dir
        if (tokenSize == 2) {
          if (chdir(*commands[1]) != -1) {
            //changed dir
          } else {
            printf("Cannot Change Directory\n");
          }
        } else {
          printf("cd: wrong number of arguments\n");
        }


      } else if (strcmp(**commands, "jobs") == 0){
        if(head == NULL){
          printf("No suspended jobs\n");
        }else{
          Jobs *temp;
          int JobNumber=1;
          for(temp = head; temp->next != NULL; temp = temp -> next){
            printf("[%d] %s\n", JobNumber, temp->cmd);
            JobNumber++;
          }
          printf("[%d] %s\n", JobNumber, temp->cmd);
        }
      } else if(strcmp(**commands, "fg") == 0){  // fg command
        if (tokenSize <= 2) {
          if(commands[0][1]!= NULL){
            if(head != NULL){
              Jobs *temp = head;
              int count = atoi(commands[0][1]);
              int over = 0;
              printf("%d\n", count);

              if(count == 1){
                if(head == NULL){
                  printf("fg: no such job\n");
                }else{
                  Jobs *newHead = head->next;
                  printf("Job wake up: %s\n", head->cmd);
                  int child  = head->pidList;
                  free(head);
                  head = newHead;
                  kill(child, SIGCONT);
                  int status;
                  waitpid(child, &status, WUNTRACED);
                  if( WIFSTOPPED(status)) {// if the child stop, create a new job node and link to the list
                    //  printf("\nA new Job node is creating...\n");
                    Jobs *newJob = creatNewJob(cmd, child);
                    if(head == NULL){
                      head = newJob;
                    }else{
                      Jobs* temp;
                      for(temp = head; temp->next != NULL; temp = temp -> next);
                      temp->next = newJob;
                    }
                    //  printf("the new node is %p\n", newJob);
                    /*wake child up*/
                  }

                }


              }else{


                for (int i = 1; i<count;i++){
                  if(temp->next!= NULL){
                    temp = temp->next;
                  }else{
                    over = 1;
                    break;
                  }
                }
                if(over == 1){
                  printf("fg: no such job\n");
                }else{


                  /*wake child up*/
                  int child = temp->pidList;
                  //printf("this is child pid %d\n", child);
                  printf("Job wake up: %s\n", temp->cmd);

                  deleteJob(head, child);

                  kill(child, SIGCONT);
                  int status;
                  waitpid(child, &status, WUNTRACED);
                  if( WIFSTOPPED(status)) {// if the child stop, create a new job node and link to the list
                    //printf("\nA new Job node is creating...\n");
                    Jobs *newJob = creatNewJob(cmd, child);
                    if(head == NULL){
                      head = newJob;
                    }else{
                      Jobs* temp;
                      for(temp = head; temp->next != NULL; temp = temp -> next);
                      temp->next = newJob;
                    }
                    //    printf("the new node is %p\n", newJob);
                    /*wake child up*/
                  }
                }
              }
            }else{
              printf("fg: no such job\n");
            }
          }
        } else {
          printf("fg: wrong number of arguments\n");
        }

        // fg command end
      }else {
        setenv("PATH", "/bin:/usr/bin:.",1);
        pid_t pid = fork();
        if (pid < 0) {
          perror("fork() Error");
          exit(-1);
        }

        if (pid > 0) {
          //parent
          int status;
          waitpid(pid, &status, WUNTRACED);
          if( WIFSTOPPED(status)) {// if the child stop, create a new job node and link to the list
            printf("\nA new Job node is creating...\n");
            Jobs *newJob = creatNewJob(cmd, pid);
            if(head == NULL){
              head = newJob;
            }else{
              Jobs* temp;
              for(temp = head; temp->next != NULL; temp = temp -> next);
              temp->next = newJob;
            }
            printf("the new node is %p\n", newJob);

          }

        } else if (pid == 0){
          signal(SIGINT, SIG_DFL);
          signal(SIGTERM, SIG_DFL);
          signal(SIGQUIT, SIG_DFL);
          signal(SIGTSTP, SIG_DFL);
          //child
          //do all stuff
          execvp(**commands,*commands);
          if (errno == ENOENT) {
            printf("%s:  command not found\n", buf1);
            exit(-1);
          } else {
            printf("[%s]:  unknown error\n", buf1);
            exit(-1);
          }
        }
      }
    } else if (pipescount >= 1) {

      // char *cat1[] = {"cat", NULL};
      // char *cat2[] = {"ls",  NULL};
      // char *cat3[] = {"cat", NULL};
      // char **cmd[] = {cat1, cat2, cat3, NULL};
      // *commands[pipescount+1] = NULL;
      // loop_pipe(commands);

      runPipedCommands(commands, pipescount);
    }
  }
}
return 0;
}

// void loop_pipe(char ***cmd) {
//   int   p[2];
//   pid_t pid;
//   int   fd_in = 0;
//
//   while (*cmd != NULL)
//   {
//     pipe(p);
//     if ((pid = fork()) == -1)
//     {
//       exit(EXIT_FAILURE);
//     }
//     else if (pid == 0)
//     {
//       dup2(fd_in, 0); //change the input according to the old one
//       if (*(cmd + 1) != NULL)
//       dup2(p[1], 1);
//       close(p[0]);
//       execvp((*cmd)[0], *cmd);
//       exit(EXIT_FAILURE);
//     }
//     else
//     {
//       waitpid(pid, NULL, WUNTRACED);
//       close(p[1]);
//       fd_in = p[0]; //save the input for the next command
//       cmd++;
//     }
//   }
// }

void runPipedCommands(char ***cmd, int pipescount) {
  int numPipes = pipescount;

  int status;
  int i = 0;
  pid_t pid;

  int pipefds[2*numPipes];

  for(i = 0; i < (numPipes); i++){
    if(pipe(pipefds + i*2) < 0) {
      perror("couldn't pipe");
      exit(EXIT_FAILURE);
    }
  }

  int j = 0;
  int z = 0;
  while(cmd[z] != NULL) {
    pid = fork();
    if(pid == 0) {
      //if not last command
      signal(SIGINT, SIG_DFL);
      signal(SIGTERM, SIG_DFL);
      signal(SIGQUIT, SIG_DFL);
      signal(SIGTSTP, SIG_DFL);

      if( j != (pipescount+1)){
        if(dup2(pipefds[j + 1], 1) < 0){
          // perror("dup2");
          exit(EXIT_FAILURE);
        }
      }

      //if not first command&& j!= 2*numPipes
      if(j != 0 ){
        if(dup2(pipefds[j-2], 0) < 0){
          // perror(" dup2");///j-2 0 j+1 1
          exit(EXIT_FAILURE);

        }
      }


      for(i = 0; i < 2*numPipes; i++){
        close(pipefds[i]);
      }


      if( execvp(*cmd[z], cmd[z]) < 0 ){

        perror(*cmd[z]);
        exit(EXIT_FAILURE);
      }
    } else if(pid < 0){
      perror("error");
      exit(EXIT_FAILURE);
    }

    z++;
    // command = command->next;
    j+=2;
  }
  /**Parent closes the pipes and wait for children*/

  for(i = 0; i < 2 * numPipes; i++){
    close(pipefds[i]);
  }

  for(i = 0; i < numPipes + 1; i++)
  wait(&status);
}

void signal_handler(int signal) {
  if (signal == SIGINT) {
    printf("\nreceived SIGINT pid %d\n", getpid());
    // kill(getpid(), SIGKILL);
  } else if (signal == SIGTERM) {

  } else if (signal == SIGQUIT) {

  } else if (signal == SIGTSTP) {
    printf("It is sleep pid %d\n", getpid());

  }

}
