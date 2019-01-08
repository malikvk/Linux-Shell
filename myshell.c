#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include "utility.h"
/*******************************************************************/
#define TRUE 1
#define FALSE 0
#define READ 0
#define WRITE 1
#define NEG -1

//COLOR MANIP
#define _GNU_SOURCE
#define BLUE "\x1B[34m"
#define YELLOW "\x1B[33m"
#define WHITE "\x1B[0m"
#define GREEN "\x1B[32m"
/*******************************************************************/
//function prototypes
void switchCmd(int, char*[], int);
void exec_shell();
void exec_batch(char*);
void launch(pid_t, char*[], int*, int*, int, int, int);

//COUNT
int line_count(char*);
int countSpace(char*,char);

//REDIRECT
void checkRedirect(char*[], int, int*, int*, int*, int*);
void cmdRedirect(char*[], int, int*, int*, int*, int*);

//GET
char* getRedirectName(char*[], int, char*, int*, char*);
char* getFileRedir(char*[], int, char*, int*, char*, int*, char*);
void  getPrevFD(int, int, int, int, int*, int*, char*, char*);

//PIPE
int  checkPipe(char*[], int, int*, int*);
void launchPipe(pid_t, int[], char*[], int, int, int);

//PRINT
void printArray(char*[], int, char*);
void printPrompt();

//RESTORE
void restoreOutput(int);
void restoreInput(int);
void restore(int, int, int*, int*, int*, int*);

//READ/WRITE
int writeToFile(char*, int, int);
int readFromFile(char*, int, int);

//PARSE/METHOD
void parseCmd(char*, char*[]);
int  parseRedirect(char*[], int);
int  parsePipe(char*[], char*[], int, int, char*);

void  readBatch(char*[], char*, int);
char* trim(char*, char);
int   verifyIn(char*); //INPUT VERIFICATION
int   check(char*[], int, int*, int*, char*[]);
void  removeNL(char*); //REMOVES NEW LINE

/********************************************************************************/
int main(int argc, char* argv[])
{
   if(argc > 1)
   {									
		//get argument[1] and set it as the filename for the batchfile
		char* batch_file = argv[1]; //the second argument
		      
	  if(access(batch_file, F_OK) == 0) //if file type is valid, do
	  {	
	  	 int i;				  
	 	 int lines = line_count(batch_file);			//SET LINES TO # OF LINES COUNTED 
	 	 char* commands[lines];						//Create space in memory for commands 
	 	 readBatch(commands, batch_file, lines); 		//Fill array with list of commands in file
	 		 
	 	 for(i = 0; i < lines; i++) 
		  {				
			exec_batch(commands[i]);				
	 	  }//END FOR
		    
	  }//END FOR
	  else 
	  {
	 	 printf("ERROR: BATCHFILE: %s FAILED.\n", strerror(errno));
	  }//END ELSE
   }//end if
   
   exec_shell(); //shell will exeute commands with input by user

}//end main
/****************************************************************************************/
void exec_shell()
{
   char* user_command = NULL;
   int bytesRead; 									//# of bytes read from getline() func
   size_t inSize = 100;								//size of input from getLine() func
  
   //List of commands
   char* built_in[] = {"cd", "clr", "dir", "environ", "echo", "help", "pause", "quit", "\0"}; 
   char* in_name; //input name
   char* out_name; //output name arg after >, >>
   
   
   int saved_STDOUT;
   int saved_STDIN;
   int pipeIndex;									//stores index of first pipe
   int pipeCount;									//stores number of pipes it encounters
   
   pid_t pid;										//input arg after <, <<
   int rf     = FALSE;        						//stdout redirection flag > 
   int rrf    = FALSE;								//stdout redirection flag >>
   int l      = FALSE;								//stdin  redirection flag <
   int lL     = FALSE;
   int valid  = FALSE;								//valid command
   int bgf    = FALSE;								//background process flag
   int piping = FALSE;								//stdin  redirection flag <<
   
   
   //Loop forever while true
   while(TRUE)
   {
  	  printPrompt();								//print current path in color
      bytesRead = getline(&user_command, &inSize, stdin); //get user input and the error code
      
	  if(bytesRead == NEG)
	  {							//if Error reading, send a error msg to user
          printf("Error! %d\n", bytesRead);	
      }//END IF 
	  else if(verifyIn(user_command))
	  {
	   //Do nothing
      }//END IF ELSE
	  else 
	  {                                      //user input scanner successful
          user_command = trim(user_command, ' ');			//get rid of leading & trailing spaces
          verifyIn(user_command);						//get rid of '\n' character
          int size = countSpace(user_command, ' ');		//count the # of args delimited by spaces
          char* cmd[size];							//create array with size of # of args
          parseCmd(user_command,cmd);					//parse input into each index of array cmd
      	  
      	  //Redirection Check
      	  if(size > 2 ) //at least 3 arguments total for redirection & pipe check
		  { 
      	  	checkRedirect(cmd, size, &rf, &rrf, &l, &lL);				//check for Redirection
      	  	pipeIndex = checkPipe(cmd, size, &piping, &pipeCount);	//check for pipes
      	  }//END IF
      	  if(rf || rrf || l || lL)	//check for redirection
		  {				
      	  	//Retrieve output file name if > or >> are used
      	  	out_name = getFileRedir(cmd, size, out_name, &rf, ">", &rrf, ">>");
      	  	
      	  	//Retrieve input file name if < or << are used
      	  	in_name = getFileRedir(cmd, size, out_name, &l, "<", &lL, "<<");
      	  	
			//Parse the cmd array removing the redirection symbols and reset the size
      	  	size = parseRedirect(cmd, size);	
      	  }//ENDIF
      	  
		  //get and save the original file descriptors
      	  getPrevFD(rf, rrf, l, lL, &saved_STDOUT, &saved_STDIN, out_name, in_name);

		  //Execute either built_in (> NEG) or non_builtins (== NEG)
      	  int index = check(cmd, size, &valid, &bgf, built_in);
      	  
      	  if(valid  && index > NEG)
		  {					//valid built_in cmd
      	  	valid = FALSE;							//flip the flag to false for next read
      	  	switchCmd(index+1, cmd, size);			//call switch command to call builtin func
      	  }//END IF 
		  else 
		  {									//not a built_in cmd
      	  	//fork the process and let the unix system handle it
      	  	launch(pid, cmd, &bgf, &piping, pipeIndex, pipeCount, size);
      	  }//END ELSE
      	  //restore file descriptors and reset redirection operator flags
      	  restore(saved_STDOUT, saved_STDIN, &rf, &rrf, &l, &lL); 
      }//END ELSE
   }//END WHILE
}//END exec_shell
/**************************************************************************/
void exec_batch(char* user_command)
{
    //List of commands
    char* built_in[] = {"cd", "clr", "dir", "environ", "echo", "help", "pause", "quit", "\0"}; 
    int valid  = FALSE;	//valid command
    int bgf    = FALSE;	//background process flag
    int piping = FALSE;	//piping flag
    char* out_name;	//output arg after >, >>
    char* in_name;	//input arg after <, <<
    
    int rf  = FALSE;    //stdout redirection flag > 
    int rrf = FALSE;	//stdout redirection flag >>
    
    int l  = FALSE;		//stdin  redirection flag <
    int lL = FALSE;		//stdin  redirection flag <<
    int saved_STDOUT;
    int saved_STDIN;
    int pipeIndex;		//stores index of first pipe
    int pipeCount;		//stores number of pipes it encounters
    pid_t pid;
  
    printPrompt(); //print current path in color
    if(verifyIn(user_command))
	{					
    	//Do nothing
	}//END IF 
	else //user input scanner successful
	{                                      	
        user_command = trim(user_command, ' ');				//get rid of leading & trailing spaces
        verifyIn(user_command); 						//get rid of '\n' character
        int size = countSpace(user_command, ' ');		//count the # of args delimited by spaces
        char* cmd[size];							//create array with size of # of args
        parseCmd(user_command,cmd);						//parse input into each index of array cmd
      	  
    	//Redirection Check
      	if(size > 2) //at least 3 arguments total for redirection & pipe check
		{ 
      		checkRedirect(cmd, size, &rf, &rrf, &l, &lL);				//check for Redirection
      	  	pipeIndex = checkPipe(cmd, size, &piping, &pipeCount);	//check for pipes
      	}//END IF
      	if(rf || rrf || l || lL)
		  {						
		  	/*
			  if any redirection symbols are used
			  Retrieve output file name if > or >> are used
			*/
      	  	out_name = getFileRedir(cmd, size, out_name, &rf, ">", &rrf, ">>");
      	  	
			//Retrieve input file name if < or << are used
      	  	in_name = getFileRedir(cmd, size, out_name, &l, "<", &lL, "<<");
      	  	
			//Parse the cmd array removing the redirection symbols and reset the size
      	  	size = parseRedirect(cmd, size);	
      	  }//END IF
      	  
      	//get and save the original file descriptors
      	getPrevFD(rf, rrf, l, lL, &saved_STDOUT, &saved_STDIN, out_name, in_name);

		//Execute either built_in (> NEG) or non_builtins (== NEG)
      	int index = check(cmd, size, &valid, &bgf, built_in);
      	if(valid  && index > NEG)
		{					//valid built_in cmd
      	  	valid = FALSE;							//flip the flag to false for next read
      	  	switchCmd(index+1, cmd, size);			//call switch command to call builtin func
      	}//END IF
		else
		{									
      	  	//fork the process and let the unix system handle it
      	  	launch(pid, cmd, &bgf, &piping, pipeIndex, pipeCount, size);
      	}//END ELSE
      	
      	//restore file descriptors and reset redirection operator flags
      	restore(saved_STDOUT, saved_STDIN, &rf, &rrf, &l, &lL); 
      	
	}//END ELSE
}//END exec_batch
/**************************************************************************/
int line_count(char* filename)
{
	FILE* file;
	file = fopen(filename, "r");								
	char line[128];
	int i = 0;							
	while(fgets(line, sizeof(line), file) != NULL)
	{		
		i++;								
	}//END WHILE
	fclose(file);							//Close filestream
	return i;
}//END line_count
/***********************************************************/
void removeNL(char* str)//remove newline \n from string
{
	while(*str != '\0')
	{
		if(*str == '\n')
		{
			*str = '\0';
        }//END IF
		str++;
	}//END WHILE
}//END REMOVE
/***********************************************************/
int countSpace(char* input, char d)
{
    int i;
    int count = 0;
	for(i = 0; input[i] != '\0'; i++)
		if(input[i] == d) //If input == delimiter count
			count++;
	return count + 1;
}// END COUNTSPACE
/***********************************************************/
char* trim(char* input, char delim)
{
	//remove leading whitespace
	while(*input == delim)
	{		     			//Move from front to back of string 
		input++;					 
	}//END WHILE							
	
	//remove trailing whitespace
	int length = strlen(input);
	char* str = input + length - 1;
	
	//Move from back to front of string
	while(length > 0 && *input == delim)
	{		
		str--;
	}//END WHILE
	*(str+1) = '\0';							//Add a null character at end
	return input;
}//END TRIM
/***********************************************************/
int verifyIn(char* cmd)
{
	int flag = TRUE;			//flag up for \n, ' ', etc. 
	while(*(cmd) != '\0')
	{		
		if(isalpha( *(cmd)) )//if alphanumeric, set flag down
		{	
			flag = FALSE; 
		}
		*(cmd)++;
	}
	return flag;
}//END verifyIn
/***********************************************************/
void readBatch(char* arr[], char* filename, int numLines)
{
	FILE* file;
	file = fopen(filename, "r");			
	char line[100];							//STORE LINE FROM FILE
	int i = 0;
	
	//Loop and read each line (including '\n') and stop when eof or NULL
	while(!feof(file) && fgets(line, sizeof(line), file) != NULL)
	{
		arr[i] = malloc(strlen(line)+1);	
		arr[i] = strdup(line);				//Copy over char array to array
		i++;
	}//END WHILE
	fclose(file);							//Close filestream
}//END READBATCH


/***********************************************************/
void parseCmd(char* input, char* arr[])
{
   char* token;
   const char delim[2] = " ";
   int i = 0;

   token = strtok(input, delim);
   arr[i] = token;
   i++;
   
   while(token != NULL)
   {
   	token = strtok(NULL, delim);	
   	arr[i] = token;
   	i++;	
   }
   printf("\n");
}
/***********************************************************/
int parseRedirect(char* cmd[], int size)
{
	int i = 0;
	int j;
	char* arr[] = {"<", "<<", ">", ">>", "\0"};	//redirection operators
	for(i = 1; i < size - 1; i++)
	{
		j = 0;
		while(strcmp(arr[j], "\0") != 0)
		{		
		    //Scan through arr[] 
			if(strcmp(cmd[i], arr[j]) == 0) //Check to see if any cmd value equal arr value
			{	
				cmd[i] = NULL;	
				return i;
			}//END IF
			j++;
		}//END WHILE
	}//END FOR
}//END PARSEREDIRECT
/***********************************************************/
void getPrevFD(int rf, int rrf, int l, int lL, int* saved_STDOUT, int* saved_STDIN, char* out_name, char* in_name)
{
	//Open file and retrieve STDOUT
    if(rf || rrf)
	{
   		*saved_STDOUT = writeToFile(out_name, rf, rrf);
    }//end if
    //Open file and retrieve STDIN
    if(l || lL)
	{
     	*saved_STDIN = readFromFile(in_name, l, lL);
    }//end if
}//END GETPREVFD
/***********************************************************/
char* getRedirectName(char* args[], int size, char* fileName, int* pointer, char* delim)
{
	int i;
	fileName = NULL;
	for(i = 1; i < size - 1; i++)//first and last redirection operator
	{				 
		if(strcmp(args[i], delim) == 0)//match strings with operator
		{		
			fileName = args[i+1];  //after redirection operator get filename 
			*pointer = TRUE;	   //redirection flag set to true
		}//END IF 
	}//END FOR
	return fileName;
}//END GETREDIRECTNAME
/***********************************************************/
int check(char* input[], int size, int* valid, int* bg, char* built_in[])
{
    int i;
    int index = NEG;
    
	//Loop through builtins and compare against user input
    for(i = 0; strcmp(built_in[i],"\0") != 0; i++)
	{
    	//if the first user argument is in the list of builtins
    	if(strcmp(input[0], built_in[i]) == 0)
		{
    		*valid = TRUE;	//for builtin change flag to true 
    		index = i;		 
    	}//END IF
    }//END FOR
    
    //set background flag to true if last argument is ampersand 
    if(*(input[size - 1]) == '&')
	{
    	*bg = TRUE;	//Set background flag to true
		input[size -1] = NULL;//Get rid of ampersand 
    }// END IF
   return index;
}// END CHECK
/***********************************************************/
void checkRedirect(char* args[], int size, int* rf, int* rrf, int* l, int* lL)
{
	int i;
	
	for(i = 1; i < size - 1; i++)
	{			
		if(strcmp(args[i], ">") == 0)
		{
			*rf = TRUE;
		}//END IF
		else if(strcmp(args[i], ">>") == 0)
		{
			*rrf = TRUE;
		}//END IF
		else if(strcmp(args[i], "<") == 0)
		{
			*l = TRUE;
		}//END IF
		else if(strcmp(args[i], "<<") == 0)
		{
			*lL = TRUE;
		}//END IF
	}//END FOR
}//END CHECKREDIRECT
/***********************************************************/
char* getFileRedir(char* cmd[], int size, char* fileName, int* a, char* symbolA, int* b, char* symbolB)
{
	if(*a)
	{
		fileName = getRedirectName(cmd, size, fileName, a, symbolA);
	}//END ID
	else if(*b)
	{
	  	fileName = getRedirectName(cmd, size, fileName, b, symbolB);
	}//END IF-ELSE
	return fileName;
}//END GETFILEREDIR
/***********************************************************/
void launchPipe(pid_t pid2, int pipefd[], char* args[], int index, int countPipe, int size)
{
	int status;
	char* output[index + 1];					// 'ls'   - array that writes in pipe 
	char* input[size - (index+1) + 1];			// 'more' - array that read in pipe    
	int outSize = parsePipe(args, output, 0, index, "output");				
	int inSize = parsePipe(args, input, index+1, size, "input");			
	
	pipe(pipefd);
	pid2 = fork();
	if(pid2 == 0)
	{
		dup2(pipefd[0], 0);	//copy over read
		close(pipefd[1]);
		execvp(input[0], input); //takes in input and exec
		perror("Error piping for read end\n");
		exit(1); 
		
	} //END IF
	else if(pid2 < 0)
	{
		perror("Error incorrect arguments\n");
		exit(NEG);
	}//END IF-ELSE
	else 
	{
		dup2(pipefd[1], 1); //copy over write 
		close(pipefd[0]);			
		execvp(output[0], output); //pushes out output
		perror("Error piping for write end\n");
		exit(1);
	}//END ELSE
}//END LAUNCHPIPE
/***********************************************************/
void launch(pid_t pid, char* args[], int* bg, int* piping, int indexP, int countP, int size)
{
	pid = fork(); //fork the current process
	int status;
	pid_t pid2;
	int pipefd[2];
	if(pid == 0)
	{
		if(*piping) //is it a pipe?
		{									
			launchPipe(pid2, pipefd, args, indexP, countP, size);	//pipe
		}//END IF
		else 
		{
			execvp(args[0], args);	//replace child with cmd with option args
		}//END ELSE
		exit(0);
	}//END IF 
	else if(pid < 0) 
	{								
		perror("Error incorrect arguments\n");		
        exit(NEG);									
	}//END ELSEIF
	else 
	{
		if(!(*bg)) //wait iff background process
		{									
			if(*piping) //is it a pipe?
			{							
				if(wait(NULL) < 0) //wait
				{					
					printf("Error with piping! %s\n", strerror(errno));
				}//END IFIFIF
				*piping = FALSE; //pipe set to false
			}//END IFIF 
			else 
			{
				if(waitpid(pid, &status, 0) < 0) //wait for child process to finish
				{	
					perror("PID ERROR\n");
				}//END IF 
			}//END ELSE
		}//END IF 
		else //if background, don't wait
		{		
			*bg = FALSE;
		}//END ELSE
	}//END ELSE
}//END LAUNCH
/***********************************************************/
int writeToFile(char* fileName, int rf, int rrf)
{
	int fd;
	if(rf)
	{
		if((fd = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, 0666)) == NEG ) //create and truncate file
		{ 
			printf("Error with '>' Redirection! %s\n", strerror(errno));
		}//END IF
    }//END IF 
	else if(rrf)
	{
      	if((fd = open(fileName, O_WRONLY|O_CREAT|O_APPEND, 0666)) == NEG)//append
		  { 
      		printf("Error with '>>' Redirection! %s\n", strerror(errno));
          }//END IF 	
    }//END ELSEIF
	int saved_STDOUT = dup(1);		//duplicate old descriptor
	dup2(fd, STDOUT_FILENO);		//replace old descriptor with a new open file
	
	return saved_STDOUT;
}//END WRITETOFILE
/***********************************************************/
void restore(int saved_STDOUT, int saved_STDIN, int* rf, int* rrf, int* l, int* lL){
	if(*rf || *rrf)
	{
  		restoreOutput(saved_STDOUT);//restore STD_OUT
  	}//END IF
   	if(*l || *lL)
	{
      	restoreInput(saved_STDIN);//restore STD_IN
    }//END IF
    //flip the redirection flags
    *rf = FALSE;
    *rrf= FALSE;
    *l  = FALSE;
    *lL = FALSE;  
}//END RESTORE

/***********************************************************/
void restoreOutput(int out)
{
	dup2(out, 1);
	close(out);
}//END RESTOREOUTPUT

/***********************************************************/
void restoreInput(int input)
{
	dup2(input, 0);
	close(input);
}//END RESTOREINPT
/***********************************************************/
void printArray(char* arr[], int size, char* str)
{
	int i;
	printf("%s size: %d\n", str, size);
	for(i = 0; i < size NEG; i++)
	{
		printf("%s : %s ", str, arr[i]);
	}//END ARRAY
	printf("\n");
}//END PRINTARRAY
/***********************************************************/

void switchCmd(int num, char* args[], int size)
{
	switch(num)
	{
		case 1	: 	myCd(args, size);
					break;
		case 2	:	myClear();	
					break;
		case 3	:	myDir();
					break;
		case 4	:	myEnv();
					break;
		case 5 	:	myEcho(args);
					break;
		case 6 	:	myHelp(args, size);
					break;
		case 7	: 	myPause();
					break;
		case 8	:	myQuit();
					break;
		default :	printf("COMMAND DOESNT EXIST\n");
					break;
	}//END SWITCH
}//END SWITCHCMD

/***********************************************************/
int parsePipe(char* cmd[], char* arr[], int start, int end, char* name)
{
	int i;
	int j = 0;
	int size = end - start;		
	for(i = start; i < end; i++)
	{
		arr[j] = cmd[i]; 		//copy string passed 
		j++;
	}//END FOR
	arr[j] = NULL;	//Null terminate array
	return size;
}//END PARSEPIPE
/***********************************************************/
int checkPipe(char* cmd[], int size, int* pipe, int* count)
{
	int i;
	int index = NEG;
	*count = 0;
	//make sure first and last values in array aren't pipes
	if(strcmp(cmd[0], "|") != 0 || strcmp(cmd[size - 1], "|") != 0)
	{
		for(i = 1; i < size - 1; i++)
		{
			if(strcmp(cmd[i], "|") == 0)
			{
			  *pipe = TRUE;					//set flag to true
			  index = i;						//store index of pipe
			  (*count)++;						//increment counter
			}//END IF
		}//END FOR
	}//END IF 
	
	else 
	{
		printf("Error! \"|\" character cannot be at the beginning or end of cmd\n");
	}//EDN ELSE
	return index;
}//END CHECK PIPE
/***********************************************************/
void printPrompt()
{
	char hostName[1024];
	gethostname(hostName, sizeof hostName);		//get host name
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));         				//get current working directory 
	printf(GREEN);
	printf("\e[1m%s@%s\e[0m", getenv("LOGNAME"), hostName); //print login and host name in bold
	printf(YELLOW);
    printf("\e[1m MyShell~%s:\e[0m ", cwd);  		//print directory in bold 
    printf(WHITE);
}//END PRINTPROMPT
/***********************************************************/
int readFromFile(char* fileName, int l, int lL)
{
	int fd;
	if(l)
	{
		if((fd = open(fileName, O_RDONLY, 0666)) == NEG)
		{
			printf("Error with '<' Redirection! %s\n", strerror(errno));
		}//END IFIF
	}//END IF
	else if(lL)
	{
		if((fd = open(fileName, O_RDONLY, 0666)) == NEG)
		{
			printf("Error with '<<' Redirection! %s\n", strerror(errno));
		}//END IF
	}//END IF-ELSE
	int saved_STDIN = dup(0);	//duplicate old file descriptor
	dup2(fd, STDIN_FILENO);		//replace old file descriptor with newly open file
	return saved_STDIN;
}//END READFORMFILE



