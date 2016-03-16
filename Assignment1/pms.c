#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>

// Generate currentNum processes
void generate_processes(int currentNum)
{
	pid_t parentPid;
	pid_t firstChildPid;
	pid_t idleChildPid;
	int random;
	int i;

	// Mark the top level process to return when process creation is done
	parentPid = getpid();

	while(currentNum > 0)
	{
		// If currentNum is 2 or more, find out how many children this tier will have
		if (currentNum > 1)
		{
			// Seed rand() to get slightly better random numbers
			srand(time(NULL));

			// Random number between 0 and currentNum - 1
			random = rand() % (currentNum);
		}
		// Otherwise, set random to 0 and only make the firstChild on this tier
		else
		{
			random = 0;
		}
		// Update currentNum before forking. -1 for firstChild -random for the idle processes.
		currentNum = currentNum - (random + 1);

		// Now we make the "firstChild" that we will create processes in the next tier
		firstChildPid = fork();
		if (firstChildPid == 0)
		{
			// Unless there are processes left to generate, set firstChild to idle (otherwise let it continue)
			if (currentNum <= 0)
			{
				while(1){};
			}
		}
		else if (firstChildPid <= -1)
		{
			perror("Failed to fork.");
			exit(0);
		}
		else
		{
			// Generate the remaining "idle" processes on this tier (using the random number)
			for (i = 0; i < random; i++)
			{
				idleChildPid = fork();
				// Catch each "idle" child process and let them loop on while(1)
				if (idleChildPid == 0)
				{
					while(1){};
				}
				else if (idleChildPid == -1)
				{
					perror("Failed to fork.");
					exit(0);
				}
			}
			// When the firstChild is done creating processes on this tier, set it to idle (unless it is the parent process)
			if (getpid() != parentPid)
			{
				while(1){};
			}
			else
			{
				// The initial process will wait until the expected number of processes have been created, then it will return
				// Couldn't figure out a way to wait until processes are done, so give it 3 seconds to complete (should be plenty of time)
				sleep(3);
				printf("Process generation complete...\n");
				return;
			}
		}
	}
}

void list()
{
	pid_t childPid;
	int ret;

	// Create new process to run ps
	childPid = fork();
	if(childPid == 0){
		printf("\n");
		char *const parmList[] = {"ps", "-o", "pid,ppid,time", NULL};

		// Exectute PS with the specified parameters
		execvp("/bin/ps", parmList);

		printf("Execvp Error");
		exit(-1);
	}
	else if (childPid <= -1)
	{
		perror("Failed to fork.");
					exit(0);
	}
	else 
	{
		if (waitpid( childPid, &ret, 0 ) == -1 ) 
		{
			perror( "Waitpid error." );
		} 
		else if ( WIFEXITED( ret ) && WEXITSTATUS( ret ) != 0 ) 
		{
		    perror( "Child error." );
		}
	}
	printf("\n");
}

void suspend()
{
	const char* prompt ="input pid to suspend>";
	char* reply = readline(prompt);
	int replyInt = atoi(reply);
	if (replyInt != 0)
	{
		int retVal = kill(replyInt, SIGSTOP);
		printf("Suspending process: %d\n", replyInt);
	}
	else
	{
		fprintf(stderr, "Invalid input.\n");
	}
}

void resume()
{
	const char* prompt ="input pid to resume>";
	char* reply = readline(prompt);
	int replyInt = atoi(reply);
	if (replyInt != 0)
	{
		int retVal = kill(replyInt, SIGCONT);
		printf("Resuming process: %d\n", replyInt);
	}
	else
	{
		fprintf(stderr, "Invalid input.\n");
	}
}

void terminate()
{
	const char* prompt ="input pid to terminate>";
	char* reply = readline(prompt);
	int replyInt = atoi(reply);
	if (replyInt != 0)
	{
		int retVal = kill(replyInt, SIGTERM);
		printf("Terminating process: %d\n", replyInt);
	}
	else
	{
		fprintf(stderr, "Invalid input.\n");
	}
}

int
main(int argc, char* argv[])
{
	const char* prompt = "input> ";
	int numChildren;

	if (argc < 2) {
		fprintf(stderr, "No processes generated, please provide a process_count arg.\n");
		
	} else {
		numChildren = atoi(argv[1]);
	}
	printf("Generating %d processes...\n", numChildren);
	generate_processes(numChildren);

	int bailout = 0;
	while (!bailout) {

		printf("Please input your selection:\n 1. List\n 2. Suspend\n 3. Resume\n 4. Terminate\n 5. Exit\n");

		char* reply = readline(prompt);

		if (!strcmp(reply, "1")) {
			list();
		} else if (!strcmp(reply, "2")) {
			suspend();
		} else if (!strcmp(reply, "3")) {
			resume();
		} else if (!strcmp(reply, "4")) {
			terminate();
		} else if (!strcmp(reply, "5")) {
			bailout = 1;
		} else {
			printf("\nInvalid Input\n\n");
		}
	
		free(reply);
	}
	printf("Exiting PMS...\n");
}

