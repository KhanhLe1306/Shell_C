/*---------------------------------------------------------------*/
/* A simple shell                                                */
/* Stanley Wileman & Pei-Chi Huang                      		 */
/*                                                               */
/* Process command lines (100 char max) with at most 16 "words"  */
/* and one command. No wildcard or shell variable processing.    */
/* No pipes, conditional or sequential command handling is done. */
/* Each word contains at most MAXWORDLEN characters.             */
/* Words are separated by spaces and/or tab characters.          */
/*                                                               */
/* This file is provided to students in CSCI 4500 for their use  */
/* in developing solutions to the first programming assignment.  */
/*---------------------------------------------------------------*/
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#define MAXLINELEN 100 /* max chars in an input line */
#define NWORDS 16	   /* max words on command line */
#define MAXWORDLEN 64  /* maximum word length */

extern char **environ; /* environment */

char line[MAXLINELEN + 1]; /* input line */
char *words[NWORDS];	   /* ptrs to words from the command line */
int nwds;				   /* # of words in the command line */
char path[2][MAXWORDLEN];	   /* path to the command */
char *argv[NWORDS + 1];	   /* argv structure for execve */

int isPipe = 0;
int numberOfPipe = 0;
char pipeLine[2][MAXLINELEN + 1]; /* input pipe line */
int pipeIndex = 0; 

/*------------------------------------------------------------------*/
/* Get a line from the standard input. Return 1 on success, or 0 at */
/* end of file. If the line contains only whitespace, ignore it and */
/* get another line. If the line is too long, display a message and */
/* get another line. If read fails, diagnose the error and abort.   */
/*------------------------------------------------------------------*/
/* This function will display a prompt ("# ") if the input comes    */
/* from a terminal. Otherwise no prompt is displayed, but the       */
/* input is echoed to the standard output. If the input is from a   */
/* terminal, the input is automatically echoed (assuming we're in   */
/* "cooked" mode).                                                  */
/*------------------------------------------------------------------*/
int Getline(void)
{
	int n;		/* result of read system call */
	int len;	/* length of input line */
	int gotnb;	/* non-zero when non-whitespace was seen */
	char c;		/* current input character */
	char *msg;	/* error message */
	int isterm; /* non-zero if input is from a terminal */

	isterm = isatty(0); /* see if file descriptor 0 is a terminal */
	for (;;)
	{
		if (isterm)
			write(1, "# ", 2);
		gotnb = len = 0;
		for (;;)
		{

			n = read(0, &c, 1); /* read one character */

			if (n == 0)	  /* end of file? */
				return 0; /* yes, so return 0 */

			if (n == -1)
			{ /* error reading? */
				perror("Error reading command line");
				exit(1);
			}

			if (!isterm)		 /* if input not from a terminal */
				write(1, &c, 1); /* echo the character */

			if (c == '\n') /* end of line? */
				break;

			if (len >= MAXLINELEN)
			{		   /* too many chars? */
				len++; /* yes, so just update the length */
				continue;
			}

			if (c != ' ' && c != '\t') /* was input not whitespace? */
				gotnb = 1;

			if (c == '|')
			{
				isPipe = 1;
				numberOfPipe++;
			}

			line[len++] = c; /* save the input character */
		}

		if (len >= MAXLINELEN)
		{ /* if the input line was too long... */
			char *msg;
			msg = "Input line is too long.\n";
			write(2, msg, strlen(msg));
			continue;
		}
		if (gotnb == 0) /* line contains only whitespace */
			continue;

		line[len] = '\0'; /* terminate the line */
		return 1;
	}
}

/*------------------------------------------------*/
/* Parse the command line into an array of words. */
/* Return 1 on success, or 0 if there is an error */
/* (i.e. there are too many words, or a word is   */
/* too long), diagnosing the error.               */
/* The words are identified by the pointed in the */
/* 'words' array, and 'nwds' = number of words.   */
/*------------------------------------------------*/
int parse(void)
{
	char *p;   /* pointer to current word */
	char *msg; /* error message */

	//printf("Number of pipes: %d \n", numberOfPipe);

	if (numberOfPipe > 0)
	{
		int index = 0;
		char *pipe;
		pipe = strtok(line, "|");
		nwds = 0;
		while (pipe != NULL)
		{
			if(index != 0)
				pipeIndex = nwds;

			strcpy(pipeLine[index], pipe);
			index++;
			pipe = strtok(NULL, "|");
			// printf("Inside parse for PIPE : ");
			// printf("The command is: %s \n", pipeLine[index - 1]);

			// split into words 
			p = strtok(pipeLine[index - 1], " \t");
			while (p != NULL)
			{
				if (nwds == NWORDS)
				{
					msg = "*** ERROR: Too many words.\n";
					write(2, msg, strlen(msg));
					return 0;
				}
				if (strlen(p) >= MAXWORDLEN)
				{
					msg = "*** ERROR: Word too long.\n";
					write(2, msg, strlen(msg));
					return 0;
				}
				// printf("Word %d is %s \n", nwds + 1, p);
				words[nwds] = p;		 /* save pointer to the word */
				nwds++;					 /* increase the word count */
				p = strtok(NULL, " \t"); /* get pointer to next word, if any */
			}
		}

		// printf("Outside of parse pipe \n");

		// Print out words

		// size_t numwords = sizeof(words) / sizeof(words[0]);
		// for (size_t i = 0; i < numwords; i++){
		// 	if(words[i] == NULL) break;
		// 	printf("Word : %s \n", words[i]);
		// }
		//printf("Slit at index: %d \n", pipeIndex);

		return 1;
	}
	else{	// If there is no pipe
		nwds = 0;
		p = strtok(line, " \t");
		while (p != NULL)
		{
			if (nwds == NWORDS)
			{
				msg = "*** ERROR: Too many words.\n";
				write(2, msg, strlen(msg));
				return 0;
			}
			if (strlen(p) >= MAXWORDLEN)
			{
				msg = "*** ERROR: Word too long.\n";
				write(2, msg, strlen(msg));
				return 0;
			}
			words[nwds] = p;		 /* save pointer to the word */
			nwds++;					 /* increase the word count */
			p = strtok(NULL, " \t"); /* get pointer to next word, if any */
		}
		return 1;
	}
}

/*--------------------------------------------------------------------------*/
/* Put in 'path' the relative or absolute path to the command identified by */
/* words[0]. If the file identified by 'path' is not executable, return -1. */
/* Otherwise return 0.                                                      */
/*--------------------------------------------------------------------------*/
int execok(void)
{
	char *p;
	char *pathenv;

	/*-------------------------------------------------------*/
	/* If words[0] is already a relative or absolute path... */
	/*-------------------------------------------------------*/
	if (strchr(words[0], '/') != NULL)
	{							   /* if it has no '/' */
		strcpy(path[0], words[0]);	   /* copy it to path */
		return access(path[0], X_OK); /* return executable status */
	}

	/*-------------------------------------------------------------------*/
	/* Otherwise search for a valid executable in the PATH directories.  */
	/* We do this by getting a copy of the value of the PATH environment */
	/* variable, and checking each directory identified there to see it  */
	/* contains an executable file named word[0]. If a directory does    */
	/* have such a file, return 0. Otherwise, return -1. In either case, */
	/* always free the storage allocated for the value of PATH.          */
	/*-------------------------------------------------------------------*/
	pathenv = strdup(getenv("PATH")); /* get copy of PATH value */
	p = strtok(pathenv, ":");		  /* find first directory */
	while (p != NULL)
	{
		strcpy(path[0], p);		/* copy directory to path */
		strcat(path[0], "/");		/* append a slash */
		strcat(path[0], words[0]); /* append executable's name */
		if (access(path[0], X_OK) == 0)
		{				   /* if it's executable */
			free(pathenv); /* free PATH copy */
			return 0;	   /* and return 0 */
		}
		p = strtok(NULL, ":"); /* get next directory */
	}
	free(pathenv); /* free PATH copy */
	return -1;	   /* say we didn't find it */
}

int execokPipe(int k)
{
	char *p;
	char *pathenv;

	/*-------------------------------------------------------*/
	/* If words[0] is already a relative or absolute path... */
	/*-------------------------------------------------------*/
	if (strchr(words[0], '/') != NULL)
	{							   /* if it has no '/' */
		if (k == 1){
			strcpy(path[k], words[pipeIndex + 1]);	   /* copy it to path */
		}else{
			strcpy(path[k], words[0]);	   /* copy it to path */
		}
		return access(path[k], X_OK); /* return executable status */
	}

	/*-------------------------------------------------------------------*/
	/* Otherwise search for a valid executable in the PATH directories.  */
	/* We do this by getting a copy of the value of the PATH environment */
	/* variable, and checking each directory identified there to see it  */
	/* contains an executable file named word[0]. If a directory does    */
	/* have such a file, return 0. Otherwise, return -1. In either case, */
	/* always free the storage allocated for the value of PATH.          */
	/*-------------------------------------------------------------------*/
	pathenv = strdup(getenv("PATH")); /* get copy of PATH value */
	p = strtok(pathenv, ":");		  /* find first directory */
	while (p != NULL)
	{
		strcpy(path[k], p);		/* copy directory to path */
		strcat(path[k], "/");		/* append a slash */
		if (k == 1){
			strcat(path[k], words[pipeIndex + 1]); /* append executable's name */
			printf("Path %d : %s\n", k, words[pipeIndex + 1]);
		}else{
			strcat(path[k], words[0]); /* append executable's name */
		}
		if (access(path[k], X_OK) == 0)
		{				   /* if it's executable */
			free(pathenv); /* free PATH copy */
			return 0;	   /* and return 0 */
		}
		p = strtok(NULL, ":"); /* get next directory */
	}
	free(pathenv); /* free PATH copy */
	return -1;	   /* say we didn't find it */
}

/*--------------------------------------------------*/
/* Execute the command, if possible.                */
/* If it is not executable, return -1.              */
/* Otherwise return 0 if it completed successfully, */
/* or 1 if it did not.                              */
/*--------------------------------------------------*/
int execute(void)
{
	int i, j;
	int status;
	char *msg;
	char *cmd1[NWORDS]; // "echo Khanh Le"
	char *cmd2[NWORDS];	// "wc -w


	if(numberOfPipe > 0){	// Pipe

		for(size_t i = 0; i < pipeIndex; i++){
					//printf("%s ", words[i]);
					cmd1[i] = words[i];
				}
			
			cmd1[pipeIndex] = NULL; // Mark the end


		int k = 0;
				while(words[pipeIndex] != NULL && pipeIndex < nwds){
					//printf("%s ", words[pipeIndex]);
					cmd2[k] = words[pipeIndex];
					pipeIndex++;
					k++;
				}
				cmd2[k] = NULL; // Mark the end

				if (execokPipe(0) == -1)
				{
					printf("Execokpipe 0 failed");
					return 0;
				}

					int fd[2];

					pipe(fd);

					int pid1 = fork();

					if (pid1 < 0)
					{
						close(fd[0]);
						close(fd[1]);
					}

					if (pid1 == 0)
					{ // Child process
						//printf("Inside the first child process \n");
						dup2(fd[1], STDOUT_FILENO);
						close(fd[0]);
						close(fd[1]);
						execve(path[0], cmd1, environ); // Execute
						perror("execve"); /* we only get here if */
						exit(1);
					}
					wait(&pid1);

					int pid2 = fork();
					if (pid2 < 0)
					{
						close(fd[0]);
						close(fd[1]);
						kill(pid1, 9);
						wait(NULL);
						//continue;
					}

					if (pid2 == 0)
					{
						//close(0);
						//printf("Inside the second child process \n");
						dup2(fd[0], STDIN_FILENO);
						close(fd[0]);
						close(fd[1]);
						execvp(cmd2[0], cmd2); // Execute
						perror("execve");				/* we only get here if */
						kill(pid1, 9);
						exit(1);
					}

					close(fd[0]);
					close(fd[1]);
					wait(&pid1);
					wait(&pid2);

					// Main process
					//printf("Inside the parent process \n");

					pipeIndex = 0; // Reset pipeIndex
					numberOfPipe = 0;			
	}else{	// No Pipe
		if (execok() == 0)
		{					 /* is it executable? */
			status = fork(); /* yes; create a new process */

			if (status == -1)
			{ /* verify fork succeeded */
				perror("fork");
				exit(1);
			}

			if (status == 0)
			{						/* in the child process... */
				//printf("Inside the child process \n");
				words[nwds] = NULL; /* mark end of argument array */

				//printf("Path: %s \n", path[0]);

				status = execve(path[0], words, environ); /* try to execute it */

				perror("execve"); /* we only get here if */
				exit(0);		  /* execve failed... */
			}

			/*------------------------------------------------*/
			/* The parent process (the shell) continues here. */
			/*------------------------------------------------*/
			//printf("Inside the parent process \n");
			wait(&status); /* wait for process to end */
		}
		else
		{
			/*----------------------------------------------------------*/
			/* Command cannot be executed. Display appropriate message. */
			/*----------------------------------------------------------*/
			msg = "*** ERROR: '";
			write(2, msg, strlen(msg));
			write(2, words[0], strlen(words[0]));
			msg = "' cannot be executed.\n";
			write(2, msg, strlen(msg));
		}
	}
}

int main(int argc, char *argv[])
{
	while (Getline())
	{				  /* get command line */
		if (!parse()){	/* parse into words */
			continue; /* some problem... */
		} else{
			execute();
		}
	}
	write(1, "\n", 1);
	return 0;
}
