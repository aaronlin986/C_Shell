#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
#include <sys/fcntl.h>
#include <errno.h>
struct node
{
	struct node* next;
	char * command;
};

struct node* insertEnd(struct node* list, char * cmd)
{
	struct node* current = list;
	struct node* new = malloc(sizeof(struct node));
	new->command = cmd;
	new->next = NULL;
	if(list == NULL){
		list = new;
	}
	else{
		while(current->next != NULL){
			current = current->next;
		}
		current->next = new;
	}

	return list;
};

void  parse(char *line, char **argv)
{
     while (*line != '\0') {       /* if not the end of line ....... */
          while (*line == ' ' || *line == '\t' || *line == '\n') {
              *line++ = '\0';     /* replace white spaces with 0    */
          }

          if(*line != '\0') {
              *argv = line;          /* save the argument position     */
              *argv++;
          }
          while (*line != '\0' && *line != ' ' &&
                 *line != '\t' && *line != '\n')
               line++;             /* skip the argument until ...    */
     }
     *argv = '\0';                 /* mark the end of argument list  */
}

void  execute(char *comm[1][10], int j, int background) //write your code here
{
    pid_t  pid;
    int    status;
    int redirFlag = 0, redirIndex = 0;
    char *holder;

    for(int i = 0; comm[j][i] != '\0'; i++){
        char *temp[2] = {"<", ">"};
        if(strcmp(comm[j][i], temp[0]) == 0){
            redirFlag = 1;
            redirIndex = i;
        }
        else if(strcmp(comm[j][i], temp[1]) == 0){
            redirFlag = 2;
            redirIndex = i;
        }
        if(redirFlag != 0){
            if(strcmp(comm[j][0], "echo") == 0){
                holder = malloc(sizeof(comm[j][i + 1]));
                strcpy(holder, comm[j][i+1]);
                comm[j][i] = '\0';
                break;
            }
            comm[j][i] = comm[j][i+1];
        }
    }

    if ((pid = fork()) < 0) {     //Forks child process
        puts("ERROR : Failed to fork child process.");
        exit(1);
    }
    else if (pid == 0) {          //Child Process
        int fdin;

        if(redirFlag == 1){
            if(strcmp(comm[j][0], "echo") == 0){
                fdin = open(holder, O_RDONLY);
            }
            else{
                fdin = open(comm[j][redirIndex], O_RDONLY);
            }
            dup2(fdin, 0);
            close(fdin);
        }
        else if(redirFlag == 2){
            int fdout;
            if(strcmp(comm[j][0], "echo") == 0){
                fdout = open(holder, O_WRONLY | O_CREAT, 0666);
            }
            else{
                fdout = open(comm[j][redirIndex], O_WRONLY | O_CREAT, 0666);
                fdin = open(comm[j][redirIndex - 1], O_RDONLY);
                dup2(fdin, 0);
                close(fdin);
            }
            dup2(fdout, 1);
            close(fdout);
        }
        if (execvp(comm[j][0], comm[j]) < 0) {     //Executes command
            puts("ERROR: Failed to execute.");
            exit(1);
        }
        exit(0);
    }
    else{
        fputs("\n", stdout);
        if(background != 1){
            while (waitpid(-1, &status, 0) != pid) ;      //Waits for Child Process
        }
    }
}

void  executePipes(char *comm[1][10], int fdin, int fdout, int i, int background){
    pid_t pid;
    int status;
    int redirFlag = 0;

    for(int j = 0; comm[i][j] != '\0'; j++){
        char *temp[2] = {"<", ">"};
        if(strcmp(comm[i][j], temp[0]) == 0){
            redirFlag = 1;
            break;
        }
        else if(strcmp(comm[i][j], temp[1]) == 0){
            redirFlag = 1;
            break;
        }
    }

    if ((pid = fork()) < 0) {     //Forks child process
        puts("ERROR : Failed to fork child process.");
        exit(1);
    }
    else if (pid == 0) {          //Child Process
        if(redirFlag != 0){
            execute(comm, i, background);
        }
        if(fdin != 0){
            dup2(fdin, 0);
            close(fdin);
        }
        if(fdout != 1){
            dup2(fdout, 1);
            close(fdout);
        }
        if (execvp(comm[i][0], comm[i]) < 0) {     //Executes command
            puts("ERROR: Failed to execute.");
            exit(1);
        }
        exit(0);
    } else{
        if(background != 1){
            while(waitpid(-1, &status, 0) != pid);
        }
    }
}

void  main(void)
{
     char  line[1024];             /* the input line                 */
     char  *argv[64];              /* the command line argument      */
    struct node* head;
    int quoteFlag = 0;
    int count;
    int numPipes;
    int background;

     while (1) {                   /* repeat until done ....         */
         fputs("Shell -> ", stdout);     /*   display a prompt             */
         fgets(line, 1024, stdin);              /*   read in the command line     */
         fputs("\n", stdout);
         parse(line, argv);       /*   parse the line               */

         count = 0;
         head = NULL;
         numPipes = 0;
         background = 0;

         char *argTemp[64];
         memcpy(argTemp, argv, sizeof(*argv) * 64);

         //Puts each argument into a node
         while (*argTemp != '\0') {
             char *entry = malloc(sizeof(char) * 50);
             *entry = '\0';
             count++;
             while (**argTemp != '\0' || (**argTemp == '\0' && quoteFlag == 1)) {
                 char temp[2] = {**argTemp, '\0'};
                 if (**argTemp == '\0' && quoteFlag == 1) {
                     count++;
                     temp[0] = ' ';
                 }
                 else if (**argTemp == '"' && quoteFlag == 1) {
                     quoteFlag = 0;
                 }
                 else if (**argTemp == '"' && quoteFlag == 0) {
                     quoteFlag = 1;
                 }
                 strcat(entry, temp);

                 (*argTemp) = (*argTemp) + 1;
             }
             head = insertEnd(head, entry);
             *argTemp = *(argTemp + count);
         }

         char *comm[10][10];

         //Prints commands
         int i = 0, j = 0;
         struct node* current = head;
         fputs("Commands : ", stdout);
         fputs(current->command, stdout);
         comm[i][j] = current->command;
         j++;
         current = current->next;
         char *temp[2] = {"|", "&"};
         while(current != NULL){
             if(strcmp(current->command, temp[0]) == 0){
                 comm[i][j] = NULL;
                 i++;
                 j = 0;
                 numPipes++;
                 fputs(" ", stdout);
                 fputs(current->next->command, stdout);
                 current = current->next;
             }
             else if(strcmp(current->command, temp[1]) == 0){
                 background = 1;
                 comm[i][j] == NULL;
                 current = current->next;
                 continue;
             }
             comm[i][j] = current->command;
             j++;
             current = current->next;
         }
         comm[i][j] = NULL;

         //Prints each command's arguments
         fputs("\n", stdout);
         current = head;
         fputs(current->command, stdout);
         fputs(" : ", stdout);
         current = current->next;
         while(current != NULL){
             if(strcmp(current->command, temp[0]) != 0){
                 fputs(current->command, stdout);
                 fputs(" ", stdout);
             }
             else{
                 fputs("\n", stdout);
                 current = current->next;
                 fputs(current->command, stdout);
                 fputs(" : ", stdout);
             }
             current = current->next;
         }
         fputs("\n", stdout);

         while(head != NULL){
             current = head;
             head = head->next;
             free(current);
         }

         if (strcmp(argv[0], "exit") == 0){ /* is it an "exit"?     */
             exit(0);   /*   exit if it is                */
         }
         else if(strcmp(argv[0], "cd") == 0){
             chdir(argv[1]);
         }
         else if(numPipes > 0){
             int p[2], fdin = 0;
             int i = 0;
             for(; i < numPipes; i++){
                 pipe(p);
                 executePipes(comm, fdin, p[1], i, background);
                 close(p[1]);
                 fdin = p[0];
             }

             executePipes(comm, fdin, 1, i, background);
             close(p[0]);
         }
         else {
             execute(comm, 0, background);           /* otherwise, execute the command */
         }
     }
}

