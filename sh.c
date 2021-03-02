/* CISC 361 FALL 2019
 * Project 2: A Unix Shell 
 * written by: Luke Grippa
 * professor: 
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "sh.h"
#include <glob.h>

extern char **environ;

int sh( int argc, char **argv, char **envp ) {
  char *prompt = calloc(PROMPTMAX, sizeof(char));	// storing prompt
  size_t pnbytes = sizeof(prompt);
  char *commandline = calloc(MAX_CANON, sizeof(char));	// storing command user inputs before splitting up 
  int bytes_read;
  size_t cnbytes = sizeof(commandline);
  char *command, *arg, *commandpath, *p, *pwd, *owd;	// 
  char **args = calloc(MAXARGS, sizeof(char*));  	// storing the split up commandline 
  char **execArgs = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;


  uid = getuid();  // get user ID
  password_entry = getpwuid(uid);  // get passwd info for that user id 
  homedir = password_entry->pw_dir;		//  Home directory to start
						//  out with

  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL ) {
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';

  //  Put PATH into a linked list 
  pathlist = get_path();

  while (go) {	

    /* print your prompt */
	char cwd[255];
	getcwd(cwd, sizeof(cwd));
	printf("%s[%s]>", prompt, cwd);

    /* get command line and process */
	bytes_read = getline(&commandline, &cnbytes, stdin);
	
	int len = (int)strlen(commandline); // get the length of the prompttemp
	commandline[len-1] = '\0'; 	    // place a null terminator in the last spot for compare to work
	
	// command = (char *)malloc(100);
	command  = strtok(commandline, " "); // the first tok in the commandline string is stored in command pointer

	// goes through the rest of the commandline and store the tokens in the args pointer
	int argCount = 0;
	while ((args[argCount] = strtok(NULL, " ")) != NULL) {	
		// since strtok maintains a static pointer to the previous
		// string passed, we can pass NULL in as the first argument.	
		argCount++;
	} 

    /* copies args array into execArgs array inorder to call execve later on */
	if (command != NULL) {
		execArgs[0] = command;
	}
	for (int i = 0; i < argCount; i++) {
		if (args[i] != NULL) {
			execArgs[i+1] = args[i];
		}		
	}			
	
    /* check for each built in command and implement */
	// return
	if (command == NULL || strcmp(command, "^C") == 0 || strcmp(command, "^Z") == 0) { // prevents segmentation fault when you press enter with no command
		signal(EOF, SIG_DFL);	
	}

	// exit
	else if (strcmp(command, "exit") == 0) {
		printf("Executing built-in exit\n");
		printf("Freeing memory\n");
		fflush(stdout);
		/* free memory */
		free(pwd);
		free(owd);		// line 47
		free(prompt);		// line 24
		// free(commandline);	// line 26
		free(command);		// line 67
		// free args[] and execArgs[]	// line 30 and 31
		for (int i = 0; i < MAXARGS; i++) {
			free(args[i]);
		//	free(execArgs[i]);
		}
		free(args);
		free(execArgs);
		// free pathlist	// line 52
		struct pathelement *node = pathlist;
		struct pathelement *tmp = pathlist;
		free(pathlist->element);
		while(node != NULL) {
			tmp = node;
			node = node->next;
			free(tmp);
		}
		free(node);

		go = 0; // causes the while loop to exit	
	}

	// which       
	else if (strcmp(command, "which") == 0) {
		if (argCount == 0) {
			perror("'which' needs an arugment");
		} else if ((strcmp(args[0], "exit") == 0) || (strcmp(args[0], "which") == 0) || (strcmp(args[0], "where") == 0) || (strcmp(args[0], "cd") == 0) || (strcmp(args[0], "pwd") == 0) || (strcmp(args[0], "list") == 0) || (strcmp(args[0], "pid") == 0) || (strcmp(args[0], "kill") == 0) || (strcmp(args[0], "prompt") == 0) || (strcmp(args[0], "printenv") == 0) || (strcmp(args[0], "setenv") == 0) ) {
			printf("%s: shell built-in command.\n", args[0]);
			fflush(stdout);	
		} else if (which(args[0], pathlist) == NULL) {
			printf("%s: Command not found.\n", args[0]);
		} else {
			printf("Executing built-in which\n");
			for (int i = 0; i < argCount; i++) {
				char *path = which(args[i], pathlist);
				printf("%s\n", path);
				fflush(stdout);
				free(path);	// line 127
			}
		}
	}
       
	// where
	else if (strcmp(command, "where") == 0) {
		if (argCount == 0) {
			perror("'where' needs an arugment");
		} else if ((strcmp(args[0], "exit") == 0) || (strcmp(args[0], "which") == 0) || (strcmp(args[0], "where") == 0) || (strcmp(args[0], "cd") == 0) || (strcmp(args[0], "pwd") == 0) || (strcmp(args[0], "list") == 0) || (strcmp(args[0], "pid") == 0) || (strcmp(args[0], "kill") == 0) || (strcmp(args[0], "prompt") == 0) || (strcmp(args[0], "printenv") == 0) || (strcmp(args[0], "setenv") == 0) ) {
			printf("%s: shell built-in command.\n", args[0]);
			fflush(stdout);	
		} else if (where(args[0], pathlist) == NULL) {
			printf("%s: Command not found.\n", args[0]);
		} else {
			printf("Executing built-in where\n");
			for (int i = 0; i < argCount; i++) {
				char **paths = where(args[i], pathlist);
				for (int i = 0; paths[i] != NULL; i++) {
					printf("%s\n", paths[i]);
				}
				fflush(stdout);
				// free paths[]	// line 147
				for (int i = 0; i < 11; i++) {
					free(paths[i]);
				}
				free(paths);
			}
		}
	}
       
	// cd
	else if (strcmp(command, "cd") == 0) { // cd command
		if (argCount == 0) {
			if (chdir(homedir) == -1) {
				perror("chdir failed");
			} else {
			printf("Executing built-in cd\n");
			fflush(stdout);
				strcpy(pwd, cwd);
			}
		} else if (strcmp(args[0], "-") == 0) {
			if (chdir(pwd) == -1) {
				perror("chdir failed");
			} else {
			printf("Executing built-in cd\n");
			fflush(stdout);
				strcpy(pwd, cwd);
			}
		} else {
			if (chdir(args[0]) == -1) {
				perror("chdir failed");
			} else {
			printf("Executing built-in cd\n");
			fflush(stdout);
				strcpy(pwd, cwd);
			}
		}
	} 
	
	// pwd
	else if (strcmp(command, "pwd") == 0) { // pwd command
		if (argCount == 0) {
			printf("Executing built-in pwd\n");
			printf("%s\n", cwd); // prints the current working directory	
			fflush(stdout); // flush to prevent any errors from leftover characters
		} else {
			perror("pwd failed");
		}
	} 
	
	// list
	else if (strcmp(command, "list") == 0) { // list command
		printf("Executing built-in list\n");
		fflush(stdout);
		if (argCount == 0) {
			list(cwd);
		} else {
			int i = 0;
			while (args[i] != NULL) {
				printf("Directory: %s\n", args[i]);
				list(args[i]);
				printf("\n");
				fflush(stdout);
				i++;
			}
		}
	} 
	
	// pid
	else if (strcmp(command, "pid") == 0) {
		if (argCount == 0) {
			printf("Executing built-in pid\n");
			printf("Shell Process ID: %ld\n", (long)getpid()); // get process id and print
			fflush(stdout); // flush to prevent any errors from leftover characters
		} else {
			printf("PID does not take any arguments\n"); 
			fflush(stdout); // flush to prevent any errors from leftover characters
		}
	} 
	
	// kill
	else if (strcmp(command, "kill") == 0) {	
		printf("Executing built-in kill\n");
		fflush(stdout);
		if (argCount > 0) {
			if ((char)args[0][0] == '-') {
				kill(atoi(args[1]), args[0][1]);
				printf("Killing %d\n", atoi(args[0]));
			} else {
				kill(atoi(args[0]), 2);
				printf("Killing %d\n", atoi(args[0]));
			}
		} else {
			printf("'kill' needs argument\n");
		}
	} 
	
	// prompt
	else if (strcmp(command, "prompt") == 0) {
		printf("Executing built-in prompt\n");
		fflush(stdout);
		if (argCount != 0) {  // check if the first argument is empty
			strcpy(prompt, args[0]);
		} else {
			printf("Enter a prompt: ");
			fflush(stdout); // flush to prevent any errors from leftover characters
			char *prompttemp = (char *)malloc(pnbytes);
			getline(&prompttemp, &pnbytes, stdin);
			int plen = (int)strlen(prompttemp); // get the length of the prompttemp
			prompttemp[plen-1] = '\0'; 	    // place a null terminator in the last spot for compare to work
			strcpy(prompt, prompttemp);
			free(prompttemp);
		}
	} 

	// printenv
	else if (strcmp(command, "printenv") == 0) {
		if (argCount == 0) {
			printf("Executing built-in printenv\n");
			fflush(stdout);
			for (int i = 0; environ[i] != NULL; i++) {
				printf("%s\n", environ[i]);
				fflush(stdout);
			}
		} else if (argCount == 1) {
			printf("Executing built-in printenv\n");
			fflush(stdout);
			if (getenv(args[0]) != NULL) {
				printf("%s: %s\n", args[0],  getenv(args[0]));
				fflush(stdout);
			}
		} else {
			printf("printenv: Too many arguments\n");
			fflush(stdout);
		}
	}

	// setenv
	else if (strcmp(command, "setenv") == 0) {
		if (argCount == 0) {
			printf("Executing built-in setenv\n");
			fflush(stdout);
			for (int i = 0; environ[i] != NULL; i++) {
				printf("%s\n", environ[i]);
				fflush(stdout);
			}
		} else if (argCount == 1) {
			printf("Executing built-in setenv\n");
			fflush(stdout);
		  	if (getenv(args[0]) != NULL) {
				setenv(args[0], " ", 1);
			} else {
				setenv(args[0], " ", 0);
			}	
		} else if (argCount == 2) {
			printf("Executing built-in setenv\n");
			fflush(stdout);
			if (getenv(args[0]) != NULL) {
				setenv(args[0], args[1], 1);
			} else {
				setenv(args[0], args[1], 0);
			}
		} else {
			printf("setenv: Too many arguments\n");			
			fflush(stdout);
		}
	}

     /*  else  program to exec */
	else {
       /* find it */		
		if (execArgs[0][0] == '/' || execArgs[0][0] == '.') {
			commandpath = malloc(64 * sizeof(char*));
			realpath(execArgs[0], commandpath);
		} else { 
			commandpath = which(execArgs[0], pathlist);
		}	
		
       /* do fork(), execve() and waitpid() */
		if (access(commandpath, F_OK) == 0) {
			// fork and execute
			pid_t pid;
			int status;
			if ((pid = fork()) < 0) {
				perror("fork error\n");
				exit(1);
			} else if (pid == 0) { // child process
				signal(SIGINT, SIG_DFL);
				int wildcardbool = 1;
				while(wildcardbool) {
				for (int i = 0; i < argCount; i++) {
					if(strstr(execArgs[i], "*") || strstr(execArgs[i], "?")) {
						glob_t globs;
						glob(execArgs[i], GLOB_NOCHECK, 0, &globs);
						for (int p = 0; p < globs.gl_pathc; p++) {
							execArgs[i + p] = malloc(64 * sizeof(char));
							strcpy(execArgs[i + p], globs.gl_pathv[p]);
						}
						globfree(&globs);
						wildcardbool = 0;
					}
				}
				wildcardbool = 0;
				}
				printf("Executing [%s]\n", commandpath);
				execve(commandpath, execArgs, 0);
				printf("failed to execute %s\n", execArgs[0]); // won't print if execve() is succefful
			}
			if ((pid = waitpid(pid, &status, 0)) < 0) {
				printf("waitpid error\n");    
		       	}
			pid = waitpid(pid, &status, WNOHANG);
		} else {
			fprintf(stderr, "%s: Command not found.\n", execArgs[0]);
		
		}
		free(commandpath);	// line 304	
	}

	
  }
  return 0;
} /* sh() */

char *which(char *command, struct pathelement *pathlist )
{
   /* loop through pathlist until finding command and return it.  Return
   NULL when not found. */
	char tmppath[100];	
	char *returnPath;
	while(pathlist) {
		sprintf(tmppath, "%s/%s", pathlist->element, command);
		if (access(tmppath, F_OK) == 0) {
			returnPath = malloc(strlen(tmppath) + 1);
			strcpy(returnPath, tmppath);
			return returnPath;
		}
		pathlist = pathlist->next;
	}
	free(returnPath);;
	return NULL;
} /* which() */

char **where(char *command, struct pathelement *pathlist )
{
  /* similarly loop through finding all locations of command */
	char tmppath[64];	 
	char **returnPaths = calloc(11, sizeof(char*));
	int i = 0;
	while(pathlist) {
		sprintf(tmppath, "%s/%s", pathlist->element, command);
		if (access(tmppath, F_OK) == 0) {
			returnPaths[i]=malloc(strlen(tmppath)+1);
			strcpy(returnPaths[i],tmppath);
			i++;
		}
		pathlist = pathlist->next;
	}
	if (returnPaths[0] != NULL) {
		return returnPaths;
	} else {
		for (int i = 0; i < 11; i++) {
			free(returnPaths[i]);
		}
		free(returnPaths);
		return NULL;
	}
} /* where() */

void list ( char *dir )
{
  /* see man page for opendir() and readdir() and print out filenames for
  the directory passed */
	struct dirent *direct;
	DIR *dr;

	dr = opendir(dir);
	if (dr == NULL) {
		perror(dir);
	} else {
		while((direct = readdir(dr)) != NULL) {
			printf("%s\n", direct->d_name);
			fflush(stdout);
		}
	}
	closedir(dr);
	free(dr);
	free(direct);
} /* list() */

