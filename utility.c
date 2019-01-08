#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>


/*******************************************************************************************/
void myHelp(char* args[], int size) //DISPLAYS HELP FOR BUILT_IN COOMANDS TO CONSOLE
{
	
	
	printf("type `help' to see list of internally defined commands for shell. \n");
	printf("Type `help name' to find out more about the function `name'.\n\n\n");
    
	int i;
    char* built_in[] = {"cd", "clr", "dir", "environ", "echo", "help", "pause", "quit", "\0"};
   
    //if only 'help' is typed prompt user
    if(size == 1)
	{
		printf("what shell command would you like help with:");
		for(i = 0; strcmp(built_in[i], "\0") != 0; i++)
		{
			printf("%s\n", built_in[i]);
		}//END FOR
	}//END IF
	else if(strcmp(args[1], "cd") == 0)
	{
		printf("Usage: cd [dir][options]\n"
			   "'cd' changes the shell working directory. \n\n"
		       "Change the current directory to DIR.  The default DIR is the value of the HOME shell variable. \n\n"
		       "The variable CDPATH defines the search path for the directory containing"
			   "DIR.  Alternative directory names in CDPATH are separated by a colon (:). A null "
			   "directory name is the same as the current directory. If DIR begins with a slash "
		       "(/), then CDPATH is not used.\n\n"
		       "If the directory is not found, and the shell option `cdable_vars' is set, "
		       "the word is assumed to be  a variable name.  If that variable has a value, its " 			
			   " value	is used for DIR.\n\n");
	} //END ELSE-IF
	else if(strcmp(args[1], "clr") == 0)
	{
		printf("Usage: clr\n");
		printf("Clears the screen\n\n");
	} //END ELSE-IF
	else if(strcmp(args[1], "dir") == 0)
	{
		printf("Usage: dir [-clpv][+N][-N]\n"       
		       "Displays directory stack.\n\n"
		       "Display the list of current/recent directories.\n\n");
	} //END ELSE-IF
	else if(strcmp(args[1], "environ") == 0)
	{
		printf("Usage: environ\n\n"
		       "Displays system environment variables for user.\n\n");
	} //END ELSE-IF
	else if(strcmp(args[1], "echo") == 0)
	{
		printf("Usage: echo [-neE] [arg ...]\n\n"
		       "Write arguments to the standard output.\n\n"
		       "Display the ARGs, spearated by a single space character and followed by a "
		       "newline, on the standard output.\n\n");
	} //END ELSE-IF
	else if(strcmp(args[1], "help") == 0)
	{
		printf("Usage: help [-dms][pattern ...]\n\n"
		       "Display information about builtin commands.\n\n"
		       "Display brief summaris of bultin commands. If PATTERN is specified, gives "
		       "detailed help on all commands matching Pattern, otherwise the list of help topis is "
		       "printed.\n\n");
	} //END ELSE-IF
	else if(strcmp(args[1], "pause") == 0)
	{
		printf("Usage: pause\n"
		       "Freezes all running processes until ENTER key is pressed.\n\n");
	} //END ELSE-IF
	else if(strcmp(args[1], "quit") == 0)
	{
		printf("Usage: quit\n"
		       "User exits the shell program, returning exit signal 0 to Unix system.\n\n");
	}//END ELSE-IF
	else 
	{
		printf("help: no help topics match '%s'. Try 'help help'.\n", args[1]);
	}//END ELSE
}//END MY_HELP 
/*********************************************************************************************/

void myCd(char* args[], int size)
{
	if(size > 1) //if more than 1 argument excluding 'cd' then error
	{ 								
		if(chdir(args[1]) != 0)
		{
			printf("**ERROR** %s\n", strerror(errno));
		} //END IFIF
	}//END IF 
	else //if no arguments 
	{									
	    char cwd[1024];
	    getcwd(cwd, sizeof(cwd));  //get current working directory 
	    printf("MyShell~%s\n", cwd);  			
	}//END ELSE
}//END MY_CD
/*******************************************************************************************/

void myClear() //FUNC CLEARS SCRREN
{
	printf("SUCCESS!\n");	
	system("clear");
}//END MY_CLEAR
/*******************************************************************************************/

void myDir() //CURRENT FILE LIST + ERROR HANDLING
{
	DIR *dp = NULL;
	struct dirent *ep = NULL;
	
	if((dp = opendir("./")) == NULL)
	{
		perror("Failure opening directory");
		exit(1);
	} //END IF
	else 
	{
		while((ep = readdir(dp)) != NULL)
		{
			printf("%s\n", ep->d_name);
		}//END WHILE
	}//END ELSE
	closedir(dp);
	
}//END MY_DIR
/*******************************************************************************************/

void myEnv() //ENVIRONMENT SYS VARIABLES
{
	extern char **environ;
	
	int i;
	for(i = 0; environ[i] != NULL; i++)
	{
		printf("%s\n", environ[i]);
	}//END FOR
}//MY_ENV
/*******************************************************************************************/

void myEcho(char* args[]) //ECHO USER ARGUMENTS
{
	int i;
	//printf("echo called, size of array passed: %ld\n", sizeof(args)/sizeof(args[0]));
	for(i = 1; args[i] != NULL; i++)
	{
		printf("%s ", args[i]);
	}//END MY_ECHO
	printf("\n");

}//END MY_ECHO
/*******************************************************************************************/

void myPause() //SYSTEM PAUSE - WAITS FOR USER TO PRESS ENTER
{
	do 
	{
		printf("System Paused. Please press enter key to continue.\n");
	}//DO-WHILE
	while(getchar()!= '\n');
}//END MY_PAUSE
/*********************************************************************************************/

void myQuit() //QUIT - CLOSES SHELL
{
	exit(EXIT_SUCCESS);
}//END MY_QUIT


