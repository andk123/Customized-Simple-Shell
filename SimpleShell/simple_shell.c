/*
 * simple_shell.c
 *
 *  Created on: 2017-02-01
 *      Author: ArnoldK
 */

//Small comment about the comments : Even though I use "we" in the comments, I am the only contributor

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#include "simple_shell.h"

#define HISTORY_SIZE 100


//UNCOMMENT THIS LINE TO DEBUG
//#define DEBUG





/* To represent the jobs, we will use a linked list. Since there is no linked list collection in C
 * we will have to implement it. I used the Linked List tutorial available on turorialspoint at
 * the following address to help me create the data structure:
 * https://www.tutorialspoint.com/data_structures_algorithms/linked_list_program_in_c.htm
 *
 */
struct node {
	int number;
	int pid;
	char *line;
	struct node *next;
};


//Global variables
char *commandHistory[HISTORY_SIZE];
int commandNumber = 0;
int process_pid;
struct node *head_job = NULL;
struct node *current_job = NULL;
char *argsPRight[20];
char *argsPLeft[20];


//Initiliaze the args(input) to null once the command has been processed
void initialize(char *args[]){
	for (int i = 0; i < 20; i++) {
		args[i] = NULL;
	}
	return;
}


//Save all inputs (except spaces) to the history list using wrap around index (to save us from shifting whole array)
void addCommandToHistory(char *line){
	int i = commandNumber;

	/*For the first HISTORY_SIZE spots, just right on the first available place
	 * We have to add 1 byte because a \n (new line) is sometimes appended in the history during the program
	 */
	if(commandNumber < HISTORY_SIZE){
		commandHistory[i]= malloc(sizeof(char) * strlen(line)+8);
		memset (commandHistory[i],'-',sizeof(char) * strlen(line)+8);
		sprintf(commandHistory[i],"%s",line);
	}
	/*Once the array is full, Use a pointer to overwrite the oldest commands in the
	 *the array using the commandNumber mod (size of the array). This will give
	 *the index where the new command should be insert in the array. We will also
	 *use this pointer (and the mod) when we will have to display the history to display
	 *it in order. This is the same idea as a wrap around queue.
	 *
	 */
	else {
		//commandHistory[(i%HISTORY_SIZE)] = realloc(commandHistory[(i%HISTORY_SIZE)], sizeof(char) * strlen(line)+1);
		sprintf(commandHistory[(i%HISTORY_SIZE)],"%s",line);
	}

	//Use to debug the elements in history
#ifdef DEBUG
	for(int i = 0; i < HISTORY_SIZE; i++) {
		printf("%s", commandHistory[i]);
		printf("\n");
	}
#endif

	commandNumber++;
	return;
}


//Print the history of commands from the current command to the last command in the window HISTORY_SIZE using the wrap around index
void getHistory(){
	int j = commandNumber;
	for(int i = commandNumber-1; i >= commandNumber-HISTORY_SIZE; i--){
		printf("  %d\t\t%s\n",commandNumber - i,commandHistory[(j-i-1)%HISTORY_SIZE]);
		if(i == 0) break;
	}
}


/* Add a job to the list of jobs
 *
 * Inspired by the linked list tutorial founded on :
 * https://www.tutorialspoint.com/data_structures_algorithms/linked_list_program_in_c.htm
 */
void addToJobList(char *args[]){

	struct node *job = malloc(sizeof(struct node));

	//If the job list is empty, create a new head
	if (head_job == NULL){
		job->number = 1;
		job->pid = process_pid;

		//Found the past command using the wrap around indexing to find the line in history
		job->line = commandHistory[commandNumber%HISTORY_SIZE-1];

		//the new head is also the current node
		job->next = NULL;
		head_job = job;
		current_job = head_job;
	}

	//Otherwise create a new job node and link the current node to it
	else {

		job->number = current_job->number + 1;
		job->pid = process_pid;

		job->line =  commandHistory[commandNumber%HISTORY_SIZE-1];

		current_job->next = job;
		current_job = job;
		job->next = NULL;
	}
}


//Put a job in the Foreground with the fg command
void foregroundJob(char *args){

	//If there is no argument after fg.
	if (args == NULL){
		printf("no argument");
		return;
	}

	//If the argument has to be a number other we can perform the fg
	for (int i = 0; i < (int)strlen(args); i++){
		if (!isdigit(args[i])){
			fprintf(stderr,"fg: %s: no such job", args);
			return;
		}
	}

	char *ptr;
	long int jobNumber = strtol(args, &ptr, 10);

	int found = 0;
	struct node *job = head_job;
	while(!found){
		if (jobNumber <= 0){
			fprintf(stderr,"fg: %s: no such job", args);
			break;
		}
		else if (job== NULL){
			fprintf(stderr,"fg: %s: no such job", args);
			break;
		}
		else if (job->number > jobNumber){
			fprintf(stderr,"fg: %s: no such job", args);
			break;
		}

		//We founded the job
		else if(job->number == jobNumber){
			found = 1;
		}
		//Select the next job
		else{
			job = job->next;
		}
	}
	//put the job in foreground
	int pid = job->pid;
	int status;
	waitpid(pid, &status, WUNTRACED);
}

/* Print the list of jobs by print the head job first and continue with the job it is pointing to
 * At the same time, if we get to a dead job, remove the job from the list
 */
void getJobs(){
	struct node *ptr = head_job;
	struct node *previous_ptr;
	int status;

	while(ptr != NULL) {

		if(ptr == head_job){
			if (waitpid(ptr->pid, &status, WNOHANG) == -1){
				head_job = ptr->next;
				printf("  %d:  Done   \t\t%s\n",ptr->number, ptr->line);
				free(ptr);
				ptr = head_job;
				continue;
			}
			else {
				printf("  %d:  Running\t\t%s\n",ptr->number, ptr->line);
			}
		}
		else{
			if(waitpid(ptr->pid, &status, WNOHANG) == -1){
				previous_ptr->next = ptr->next;
				printf("  %d:  Done   \t\t%s\n",ptr->number, ptr->line);
				free(ptr);
				ptr = previous_ptr->next;
				continue;
			}
			else {
				printf("  %d:  Running\t\t%s\n",ptr->number, ptr->line);
			}
		}

		previous_ptr = ptr;
		ptr = ptr->next;

	}
}


/*
 *Using ! followed by a number, get the command in the history.
 *Look if "!" is entered to call a previous command. If nothing valid found, send the user a message
 *This is implemented in getcmd() right before we save the command in the history
 *because in the shell (in ubuntu) when "!" is typed, the shell does not save the command [! 'followed by digits'] but instead
 *the command that is called. And if the event is not found (!a for example), it does not save anything at all in the history
 *
 */
int getCommandHistory(char *args[], char *copyCmd, int *background){

	//If there is a character in the command... it won't find anything. Don't botter searching
	//Because of this, we won't support adding an argument to a previous command via the history call
	for (int i = 1; i < (int)strlen(args[0]); i++){
		if (!isdigit(args[0][i])){
			printf("%s: event not found",args[0]);
			return 0;
		}
	}

	//Search to see if the number matches a stored number

	//Ignore the exclamation point
	args[0]++;
	char *ptr;
	long int number = strtol(args[0], &ptr, 10);

#ifdef DEBUG
	printf("Command is:%d\n", commandNumber);
	printf("Number is:%ld\n", number);
#endif

	//We know we can't find the command if the number is greater than the number of commands entered
	//or is not in the window of saved commands
	if(number > commandNumber || number < commandNumber - HISTORY_SIZE || number <= 0){
		printf("!%s: event not found",args[0]);
		return 0;
	}

	//Found the past command using the wrap around indexing and populate args with this line.
	int index;
	if(number<=HISTORY_SIZE) index = number-1;
	else index = ((commandNumber-number)%HISTORY_SIZE);

	//A new line is fed in the string for printing purposes
	char* pastCommand = commandHistory[index];
	strcat(pastCommand, "\n");
	//We have to initialize the elements in args back to null in case there are unwanted arguments in the call
	initialize(args);

	//Recursive call to generate the previous command
	return getcmd(pastCommand, args, background);

}

//Method for handling signals especially CTLR+C
static void sigHandler(int sig){

#ifdef DEBUG
	printf("%d\n",process_pid);
#endif

	/* For CTLR + C (kill program) we have 2 situations
	 * 1. If we are running a command in the shell --> kill the program running in the shell
	 * 2. If we are inside the shell running no programs in foreground --> do nothing.
	 *
	 * Since process_pid is initialized to 0, if its value change, we know it is a child process
	 */
	if(sig == SIGINT && process_pid == 0){
		//printf("\n");
	}
	else if(sig == SIGINT){
#ifdef DEBUG
		printf("Hey! Kill the signal %d\n", process_pid);
#endif
		if (kill(process_pid, SIGINT) == -1){
			printf("ERROR! Could not bind the signal handler\n");
			exit(EXIT_FAILURE);
		}
	}
}

// Verify if a first command is piped to another one
int pipedCommand(char *args[]) {
	int i = 0;
	while(args[i] != NULL){

		if (strcmp("|", args[i]) == 0) {
			return 1;
		}
		i++;
	}
	return 0;
}

//Perform a pipe between two commands
void commandPiping(){

	int pidL, pidR, result;
	int filedes[2];

	if (pipe(filedes)== -1){
		exit(EXIT_FAILURE);
	}

	//Run the command on the left in a child first
	pidL = fork();

	//Child process
	if (pidL == 0) {

		//Redirect the output to the input so that the second process can be fed with this output
		close(1);
		if(dup(filedes[1]) == -1) exit(EXIT_FAILURE);

		close(filedes[0]);
		close(filedes[1]);

		result = execvp(argsPLeft[0], argsPLeft);

		//Usually I would use perror to print the error from the signal number the system produced
		//however it points to a file/directory since execvp executes a file from the PATH environment
		if (result == -1) {
			//perror(args[0]);
			fprintf(stderr, "%s: command not found\n", argsPLeft[0]);
		}

		//Exit so a new shell process is not launched
		exit (EXIT_SUCCESS);
	}

	else{

		//Launch the command of the right on a new child in the parent of the first child
		pidR = fork();

		//2nd command
		if (pidR == 0) {

			//Redirect the input to the output
			close(0);
			if(dup(filedes[0]) == -1) exit(EXIT_FAILURE);

			close(filedes[0]);
			close(filedes[1]);

			result = execvp(argsPRight[0], argsPRight);

			//Usually I would use perror to print the error from the signal number the system produced
			//however it points to a file/directory since execvp executes a file from the PATH environment
			if (result == -1) {
				//perror(args[0]);
				fprintf(stderr, "%s: command not found\n", argsPRight[0]);
			}

			//Exit so a new shell process is not launched
			exit (EXIT_SUCCESS);

		} else {


			//Restore the output and input
			close(filedes[0]);
			close(filedes[1]);


			//Wait for both child processes to be done before continuing (the first child has to finish 1st)
			int status;
			waitpid(pidL, &status, WUNTRACED);
			waitpid(pidR, &status, WUNTRACED);

		}

	}
	return ;
}

/***************************************************************************************************************************/

// getcmd is able to be called recursively. (To generate the previous command from history)
int getcmd(char *line, char *args[], int *background)
{
	int i = 0;
	char *token, *loc;

	//Copy the line to a new char array because after the tokenization a big part of the line gets deleted since the null pointer is moved
	char *copyCmd = malloc(sizeof(char)*sizeof(line)*strlen(line));
	sprintf(copyCmd,"%s",line);

	// Check if background is specified..
	if ((loc = index(line, '&')) != NULL) {
		*background = 1;
		*loc = ' ';
	} else
		*background = 0;

	//Create a new line pointer to solve the problem of memory leaking created by strsep() and getline() when making line = NULL
	char *lineCopy = line;
	while ((token = strsep(&lineCopy, " \t\n")) != NULL) {
		for (int j = 0; j < strlen(token); j++)
			if (token[j] <= 32)
				token[j] = '\0';
		if (strlen(token) > 0)
			args[i++] = token;
	}

	//If there is a exclamation point as the first letter of the command, it is to call a command in the history
	if (line[0] == '!' && (int)strlen(line) > 1){

		i = getCommandHistory(args, copyCmd, background);

		free(copyCmd);
		return i;
	}

	//If anything else than space(s) was entered, save the input to the history
	//Skip the last byte of the command line because it is a new line \n
	if(i>0){
		copyCmd[strlen(copyCmd)-1] = 0;
		addCommandToHistory(copyCmd);
	}

	free(copyCmd);
	return i;
}


/****************************************************************************************************************************************************/
int main(void)
{
	char *args[20];
	int bg;

	//Get the name of the User in the environment setting for the shell user.
	//If the name is not written in the env settings, use a default name
	char *user=getenv("USER");
	if(user==NULL) user="User";

	char str[sizeof(char)*strlen(user)+4];
	sprintf(str, "\n%s>> ",user);
	printf("Welcome to the Simple Shell %s.\n"
			"Press help or -h for a quick help\n"
			"Press exit to Exit\n", user);


	// Listen to the signal CTRL+C (kill program) and CTRL+Z (ignore this command)
	if (signal(SIGINT, sigHandler) == SIG_ERR){
		printf("ERROR! Could not bind the signal handler\n");
		exit(EXIT_FAILURE);
	}
	if (signal(SIGTSTP, SIG_IGN) == SIG_ERR){
		printf("ERROR! Could not bind the signal handler\n");
		exit(EXIT_FAILURE);
	}


	while(1) {
		//For execvp to run without error, the last element of args has to be NULL
		initialize(args);

		//Each time initialize background to zero, and process_pid to zero to differentiate it from process ran as a child
		bg = 0;
		process_pid = 0;

		int length = 0;
		char *line = NULL;
		size_t linecap = 0;
		printf("%s", str);
		length = getline(&line, &linecap, stdin);

		if (length <= 0) {
			exit(-1);
		}

		int cnt = getcmd(line, args, &bg);

		/****************************************************************************/

		//If no command (or spaces) was entered. Ignore and Wait for the next input
		if(cnt == 0) {}

		/****************************************************************************/

		// Check if piped
		else if (pipedCommand(args)){

			//Make sure no previous commands in the arguments
			initialize(argsPRight);
			initialize(argsPLeft);

			//Feed the left argument with the command left to the pipe
			int i = 0;
			while (strcmp(args[i], "|") !=0){
				argsPLeft[i] = args[i];
				i++;
			}

			//Feed the right argument with the command right to the pipe
			i++;
			int j = i;
			while(args[i] != NULL){
				argsPRight[i-j] = args[i];
				i++;
			}

			// Make the pipe between the two processes
			commandPiping();
		}

		/****************************************************************************/

		//Command is jobs
		else if (!strcmp("jobs", args[0])){
			getJobs();
		}

		/****************************************************************************/

		//Command is jobs
		else if (!strcmp("fg", args[0])){
			foregroundJob(args[1]);
		}

		/****************************************************************************/

		//Command is history
		else if (!strcmp("history", args[0])){
			getHistory();
		}
		/****************************************************************************/

		//Command is pwd
		else if (!strcmp("pwd", args[0])) {

			//The path length limit on Linus is 4096 bytes including null but we will keep the array to 200
			char cwd[200];
			if (getcwd(cwd, sizeof(cwd)) != NULL)
				printf("%s\n", cwd);
			else
				perror("pwd");
		}
		/****************************************************************************/

		//Command is cd
		else if(!strcmp("cd", args[0])) {

			//If user enters cd without any attribute --> go to home directory if specified in environment variable
			//otherwise do nothing
			int result = 0;
			if (args[1] == NULL) {
				char *home=getenv("HOME");
				if(home!=NULL){
					result = chdir(home);
				}
				else{
					printf("cd: No $HOME variable declared in the environment");
				}
			}
			//Otherwise go to specified directory
			else{
				result = chdir(args[1]);
			}
			if(result == -1) fprintf(stderr, "cd: %s: No such file or directory",args[1]);
		}
		/****************************************************************************/

		//Command is help
		else if (!strcmp("help", args[0]) || !strcmp("-h", args[0])){
			printf("To see a previous command, tap history\nTo use it. Press the number preceded by a '!'\n");
				printf("To go to your home directory press 'cd ..'\n");
				printf("Press CTRL+C to kill a program\nAnd CTRL+D to quit the shell");
		}
		/****************************************************************************/

		//Command is exit
		else if (!strcmp("exit", args[0])) {
			exit(EXIT_SUCCESS);
		}
		/****************************************************************************/

		//The user is accessing a command that is not pre built so we execvp the process after forking it
		else{

			//We fork the process into two new processes where the ID of the child is 0
			//int process_pid = fork();
			process_pid = fork();

			/*******************************************************************************************************/
			//Child process
			if (process_pid == 0){

				//Verify if the output was to be redirected.
				int i = 0;
				int redirected = 0;
				while(args[i] != NULL) {
					if(strcmp(">", args[i]) == 0){
						redirected = 1;
						break;
					}
					i++;
				}

				int result;

				//If it is, transfer stdout to the file specified after the '>' by closing the file descriptor
				//and transferring to the opened file
				if (redirected){
					int copy_stdout = dup(1);
					close(1);
					int file = open(args[i+1], O_RDWR | O_CREAT );

					//We do not want to copy the file name and the arrow into the file
					args[i] = NULL;
					args[i+1] = NULL;

					//Execute the command, and save the return value: If failure it will be -1
					result = execvp(args[0], args);

					//dup2 will close the file we just opened and store by the file descript to stdout
					dup2(copy_stdout, 1);
					close(1);
				}
				else{
					result = execvp(args[0], args);
				}

				//Usually I would use perror to print the error from the signal number the system produced
				//however it points to a file/directory since execvp executes a file from the PATH environment
				if (result == -1) {
					//perror(args[0]);
					fprintf(stderr, "%s: command not found\n", args[0]);
				}

				//Exit the child process when done otherwise, it will start a new shell process from the beginning
				exit(EXIT_SUCCESS);
			}

			/*******************************************************************************************************/
			//Parent process with the same ID as original process
			else{

				//Verify if the command is to be ran in background, if yes, add it to the jobs and redirect to normal shell
				//Otherwise wait for the child process to be finished
				if(bg == 1){
					addToJobList(args);
					bg = 0;
				}
				else{
					int status;
					waitpid(process_pid, &status, WUNTRACED);
				}

			}
		}
		//Free line at the very end to avoid memory leaks
		free(line);
	}
}
