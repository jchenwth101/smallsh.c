/***************************************************************
 * Filename: smallsh.c
 * Author: Joel Chenoweth
 * * Date: 25 July 2021
 * Description: Main file for smallsh, CS 344 Program 3
***************************************************************/
// reference:  https://stackoverflow.com/questions/9860671/double-pointer-char-operations
// reference:  https://github.com/gkochera/cs-344-smallsh/blob/main/smallsh.c
// reference: https://stackoverflow.com/questions/33863720/sigaction-implicit-declaration
// reference: https://stackoverflow.com/questions/33862205/writing-my-own-shell-in-c-stuck-on-user-input-during-sleep

#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
//#include <bits/siginfo.h>

#define INPUTLENGTH 2048

int allowBackground = 1;
struct sigaction one;

void getInput(char*[], int*, char[], char[], int);
void execCmd(char*[], int*, struct sigaction, int*, char[], char[]);
void catchSIGTSTP(int);
void printExitStatus(int);

/** Main function **/ 
/*	main() collect: input, check: blanks,comments.
 *	check: CD, EXIT, and STATUS
 * check: ls, status, exit, or else blank
 * ------------ */
// To Perform the first three functions

int main() {

	int pid = getpid();
	int cont = 1;
	int i;
	int exitStatus = 0;
	int background = 0;

	char inputFile[256] = "";
	char outputFile[256] = "";
	char* input[512];
	for (i=0; i<512; i++) {
		input[i] = NULL;
	}

	// Signal Handlers Structs reference:  https://github.com/ryancraigdavis/SmallSh/blob/master
	// reference;  https://stackoverflow.com/questions/2794973/add-a-function-to-a-program-and-call-that-function-from-the-command-line-in-the
	// Below lists struct for Ignore ^C
	struct sigaction sa_sigint;
	//memset(sa_sigint, '\0', 10*sizeof(int));
	//sa_sigint = {0};
	(sa_sigint).sa_handler = SIG_IGN;
	sigfillset((sigset_t*)(&(sa_sigint).sa_mask));
	(sa_sigint).sa_flags = 0;
	sigaction(SIGINT, &sa_sigint, NULL);

	// Below to Redirect ^Z to catchSIGTSTP();
	struct sigaction sa_sigtstp = {0};
	sa_sigtstp.sa_handler = catchSIGTSTP;
	sigfillset(&sa_sigtstp.sa_mask);
	sa_sigtstp.sa_flags = 0;
	sigaction(SIGTSTP, &sa_sigtstp, NULL);

	do {
		// Below to check to Get input
		getInput(input, &background, inputFile, outputFile, pid);

		// Below to check to COMMENT OR BLANK
		if (input[0][0] == '#' ||
					input[0][0] == '\0') {
			continue;
		}
		
		// Below to check EXIT
		else if (strcmp(input[0], "exit") == 0) {
			cont = 0;
		}
	
		// Below to check CD
		else if (strcmp(input[0], "cd") == 0) {
			// This will change to the directory specified
			if (input[1]) {
				if (chdir(input[1]) == -1) {
					printf("The Directory is not found.\n");
					fflush(stdout);
				}
			} else {
			// If directory is not specified, go to Home 
				chdir(getenv("HOME"));
			}
		}

		// Below to check STATUS
		else if (strcmp(input[0], "status") == 0) {
			printExitStatus(exitStatus);
		}

		// Else options to pass OS1 werror flags
		else {
			execCmd(input, &exitStatus, sa_sigint, &background, inputFile, outputFile);
		}

		// Below to check Reset variables
		for (i=0; input[i]; i++) {
			input[i] = NULL;
		}
		background = 0;
		inputFile[0] = '\0';
		outputFile[0] = '\0';

	} while (cont);
	
	return 0;
}

/*****
 * void getInput(char*[], int, char*, char*, int)
 reference: https://github.com/Ryanwall15; https://github.com/Ryanwall15/CS344-Smallsh/blob/master/smallsh.c
 reference: https://stackoverflow.com/questions/2794973/add-a-function-to-a-program-and-call-that-function-from-the-command-line-in-the
 * Prompts the user to check the input into an array 
 
 *****/
void getInput(char* arr[], int* background, char inputName[], char outputName[], int pid) {
	
	char input[INPUTLENGTH];
	int i, j;

	// Below to check to Get input
	printf(": ");
	fflush(stdout);
	fgets(input, INPUTLENGTH, stdin);

	// Below to Remove newline
	int found = 0;
	for (i=0; !found && i<INPUTLENGTH; i++) {
		if (input[i] == '\n') {
			input[i] = '\0';
			found = 1;
		}
	}

	// Below to check blank/return blank line
	if (!strcmp(input, "")) {
		arr[0] = strdup("");
		return;
	}

	// Below to analyze raw Input forindividual strings
	const char space[2] = " ";
	char *token = strtok(input, space);

	for (i=0; token; i++) {
		// Below to Check for background process
		if (!strcmp(token, "&")) {
			*background = 1;
		}
		// Below to Check for "<"" to denote input file
		else if (!strcmp(token, "<")) {
			token = strtok(NULL, space);
			strcpy(inputName, token);
		}
		// Below to Check for ">"" to denote output file
		else if (!strcmp(token, ">")) {
			token = strtok(NULL, space);
			strcpy(outputName, token);
		}
		// Else it's apart of the command prompt
	else {
			arr[i] = strdup(token);
			// Below to replace $$ with pid
			// Below happens at end of string in testscirpt
			char buff_tmp[256];
			for (j=0; arr[i][j]; j++) {
				if (arr[i][j] == '$' &&
					 arr[i][j+1] == '$') {
					arr[i][j] = '\0';
					//snprintf(arr[i], 256, "%s%d", arr[i], pid);
					snprintf(buff_tmp, 256, "%s%d", arr[i], pid);
					strcpy(arr[i],buff_tmp);
				}
			}
		}
		// 
		token = strtok(NULL, space);
	}
}

 /*****
 * void execCmd(char*[], int*, struct sigaction, int*, char[], char[])
 *
 * Executes the command parsed into arr[]
 *
 
****/
void execCmd(char* arr[], int* childExitStatus, struct sigaction sa, int* background, char inputName[], char outputName[]) {
	
	int input, output, result;
	pid_t spawnPid = -5;
	pid_t actualPid;

	// Below to fork()
	// from lecture notes https://oregonstate.instructure.com/courses/1821221/pages/exploration-processes-and-i-slash-o?module_item_id=21139743
	spawnPid = fork();
	actualPid = spawnPid;
    if(actualPid == -2){ printf("error\n"); }
	switch (spawnPid) {
		
		case -1:	;
			perror("Error. These are not the droids you're looking for.\n");
			exit(1);
			break;
		
		case 0:	;
			// Below to take process will now take ^C as default
			sa.sa_handler = SIG_DFL;
			sigaction(SIGINT, &sa, NULL);

			// Below to handle input, 
			if (strcmp(inputName, "")) {
				// Below to open file
				input = open(inputName, O_RDONLY);
				if (input == -1) {
					perror("Unable to open input file\n");
					exit(1);
				}
				// Below to assign file
				result = dup2(input, 0);
				if (result == -1) {
					perror("Unable to assign input file\n");
					exit(2);
				}
				// Below to close file
				fcntl(input, F_SETFD, FD_CLOEXEC);
			}

			// Below to Handle output, from lecture notes https://oregonstate.instructure.com/courses/1821221/pages/exploration-processes-and-i-slash-o?module_item_id=21139743
			if (strcmp(outputName, "")) {
				// Below to open file
				output = open(outputName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				if (output == -1) {
					perror("Unable to open output file\n");
					exit(1);
				}
				// Below to assign file
				result = dup2(output, 1);
				if (result == -1) {
					perror("Unable to assign output file\n");
					exit(2);
				}
				// Below to determine process its close
				fcntl(output, F_SETFD, FD_CLOEXEC);
			}
			
			// Below to Execute
			if (execvp(arr[0], (char* const*)arr)) {
				printf("%s: There is no such file or directory\n", arr[0]);
				fflush(stdout);
				exit(2);
			}
			break;
		
		default:	;
			// Below to check to execute a process in the background ONLY if allowBackground
			if (*background && allowBackground) {
				actualPid = waitpid(spawnPid, childExitStatus, WNOHANG);
				printf("The Background pid is %d\n", spawnPid);
				fflush(stdout);
			}
			// Below, the else will  execute as if normal
			else {
				actualPid = waitpid(spawnPid, childExitStatus, 0);
			}

		// Below to check for terminated background process	
		while ((spawnPid = waitpid(-1, childExitStatus, WNOHANG)) > 0) {
			printf("The child %d terminated\n", spawnPid);
			printExitStatus(*childExitStatus);
			fflush(stdout);
		}
	}
}

/****
 * void catchSIGTSTP(int)
 *
 * When SIGTSTP is called, toggle to allow Background boolean.
 ref: https://github.com/Ryanwall15/CS344-Smallsh/blob/master/smallsh.c
 ref: https://stackoverflow.com/questions/33862205/writing-my-own-shell-in-c-stuck-on-user-input-during-sleep
****/
void catchSIGTSTP(int signo) {

	// If == 1, set to 0, display a message 
	if (allowBackground == 1) {
		char* message = "Entering foreground-only mode\n";
		write(1, message, 49);
		fflush(stdout);
		allowBackground = 0;
	}

	// If == 0, set to 1, display message 
	else {
		char* message = "Exiting foreground-only mode\n";
		write (1, message, 29);
		fflush(stdout);
		allowBackground = 1;
	}
}
/***
 * void printExitStatus(int)
 *
 * Calls the exit status notes from modules
ref: https://oregonstate.instructure.com/courses/1821221/pages/exploration-processes-and-i-slash-o?module_item_id=21139743
****/
void printExitStatus(int childExitMethod) {
	
	if (WIFEXITED(childExitMethod)) {
		// Below to check if exited by status
		printf("exit value %d\n", WEXITSTATUS(childExitMethod));
	} else {
		// Below to check if terminated by signal
		printf("terminated by signal %d\n", WTERMSIG(childExitMethod));
	}
}
