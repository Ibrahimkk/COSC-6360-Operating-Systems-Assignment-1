//These includes are so I can use the STL and other functions
#include <iostream>
#include <string>
#include <queue>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <cstring>

using namespace std; //so I don't have to type std:: before the stl functions

class command //Class to contain the primary variables and functions used throughout the program.
{
	char pathname[8192];
	char pname[8192];
	char nameChar[8192];
	const char ** argvec;
	deque<pid_t> processId;
	deque<string> processCommand;
	deque<string> names;
	deque<string> pathNames;
	string input;
public:
	string readLine();
	deque<string> split(string str, char ch);
	void getPath();
	bool bgProcess(bool backg);
	void changeDirectory();
	bool inputParsing();
	void argumentVector();
	int outputRedirection();
	int inputRedirection();
	void checkProcess();
	void executableFunc(bool backg);
	bool inputOperations(bool backg);
};

int main(int argc, char * argv[]) //main
{ 
	command cmd;
	while(true) //will keep looping until user enters exit
	{
		bool backg = false; //bool used for background process
		bool cont = false; //bool used for continuing instead of going through the whole loop

		cont = cmd.inputParsing(); //calls inputParsing function to 
		if(cont == true){continue;} //if inputParsing returns true for cont, then it will continue because the function is a built in function and should be executed right away, then the loop should be reset.

		backg = cmd.inputOperations(backg); //calling inputOperations function to parse the split input and see if it is background or a fullpath and deal with it accordingly
		cmd.argumentVector(); //calling argumentVector function to create and set the argument vector to be used in execv
		cmd.executableFunc(backg); //calling executableFunc function to fork a child and run execv as well as check to see if the command is a background process or not, and deal with it accordingly
	}
}

string command::readLine() //this function gets the user input using getchar(), because getline was causing errors.
{
	string str = ""; //empty string to start out with, so I can append characters to it as the user puts their input in
	while(true)
	{
		int c = getchar(); //getting char as an int and converting it into a char later, to avoid errors
		if(c == '\n' || c == EOF) //if it is end of file, or user inputs enter, it will stop.
			break;
		else
			str += (char)c; //this is the string, which I am adding every character to
	}
		return str; //returning the str
}

deque<string> command::split(string str, char ch) //this function splits the user input by the character ch provided so I can parse through it and do things with it later on.
{
	deque<string> strings;
	stringstream ss(str);
	string token;

	while(getline(ss, token, ch)) {
		strings.push_back(token);
	}
	return strings;
}

void command::getPath() //this function takes the input and finds the executable in the correct path
{
	for(int i = 0; i < pathNames.size(); i++) //Finding the right executable
		{
			memset(&pathname[0], 0, sizeof(pathname)); //resets the char array, so that it doesn't run into memory issues
			memset(&nameChar[0], 0, sizeof(nameChar)); //resets the char array, so that it doesn't run into memory issues
			memset(&pname[0], 0, sizeof(pname)); //resets the char array, so that it doesn't run into memory issues
			for(int k = 0; k < names.at(0).size(); k++) //copies the elements of the split user input deque into a char arrary so I can use it in sprintf
			{
				nameChar[k] = names.at(0).at(k);
			}
			for(int j = 0; j < pathNames.at(i).size(); j++) //does same thing as the above for loop but for the path variables that I got using getenv("PATH")
			{
				pname[j] = pathNames.at(i).at(j);
			}
			pname[pathNames.at(i).size()] = '/'; //adding a forward slash into the char array so that when we append the char arrays in sprintf we get the full path with the forward slash in the end before the executable
			sprintf(pathname, "%s%s", pname, nameChar); // appends the char array path and char array user input for the executable to char* pathname
			if(access(pathname, X_OK) == 0) //checks to see if the executable is found
				break;
			else if(i == pathNames.size() - 1) //if no executable is found, then it tells the user to input a different command
				cout << "Not found, please try a different executable" << endl;
		}
}


bool command::bgProcess(bool backg) //This sets the boolean for background process to true, and it erases the bg from the deque, so that we can proceed to find the path of the original input without bg
{
	backg = true;
	names.erase(names.begin());
	getPath();
	return backg; //returns backg bool to be used in main in the end when checking to see if the process is background or not
}

void command::changeDirectory() //changes directory according to user input
{
	deque<string> cDirectory = split(input, ' '); //takes user input and splits it by whitespace, and since the command for cd is cd somepathname, it should only be 2 elements,
	chdir(cDirectory.at(1).c_str()); //so I take the second element, and use that as the directory I am changing to.
}

bool command::inputParsing()
{
	bool cont = false;
	cout << "Myshell: ";
	input = readLine(); //gets user input
	while(input == "") //makes sure a user inputs something, loops until user inputs something.
	{
		cout << "Myshell: ";
		input = readLine();
	} 
	if(input == "exit") //will exit while loop when user enters exit. It will also check for any background processes and terminate them.
		{
			if(!processId.empty()) //if there are background processes
				for(int i = 0; i < processId.size(); i++) 
					kill(processId.at(i),0); //for each process, kill that process
			exit(0); //then exit the program
		}
	else if(input == "processes") //if user inputs processes
	{
		if(!processId.empty()) //if there are background processes
			for(int i = 0; i < processId.size(); i++)
				cout << "Process Id: " << processId.at(i) << " Process Command: " << processCommand.at(i) << endl; //Output the process id's and commands for all the background processes
		else
			cout << "There are no background processes." << endl; //if there are no background processes
		cont = true;
	}
	else if(input.at(0) == 'c' && input.at(1) == 'd')
	{
		changeDirectory();
		cont = true;
	}
	return cont;
}

void command::argumentVector() //This function is to set the argument vector for execv function.
{
	argvec = new const char*[names.size()+1]; //creating a character array which is the size of the elements in the deque that contains the user input split by white spaces.
	argvec[0] = pathname; //setting the first element in the character array to the full pathname that is gotten from the getPath function above
	if(names.size() > 1) //if there are more than 1 element in the names array, aka ls > output.txt which is 3 elements
	{	
		if(names.at(names.size()-2).at(0) == '>' || names.at(names.size()-2).at(0) == '<') //then it checks to see if it is input or output redirection
		{	
			if(names.size() > 4)
			{
				if(names.at(names.size()-4).at(0) == '>' || names.at(names.size()-4).at(0) == '<')
				{
					for(int i = 3; i < names.size()-2; i++) //gets rid of the tokens > < and then continues to copy the rest of the names deque and sets last element to null which is required for execv
						argvec[i-2] = names.at(i).c_str();
					argvec[names.size()-4] = NULL;
				}
			}
			else
			{
				for(int i = 1; i < names.size()-2; i++) //gets rid of the tokens > < and then continues to copy the rest of the names deque and sets last element to null which is required for execv
					argvec[i] = names.at(i).c_str();
				argvec[names.size()-2] = NULL;
			}
		}
		else //if it is not input or output redirection, then it just copies the elements into the char array and sets last element to null
		{
			for(int i = 1; i < names.size(); i++)
				argvec[i] = names.at(i).c_str();
			argvec[names.size()] = NULL;
		}
	}	
	else //if the input is just any one word input(like ls for example), then it just sets the second element to NULL, since the first element is the pathname already
		argvec[1] = NULL;
}

int command::outputRedirection() //function for output redirection
{
	string outputFile = "";
	outputFile = names.at(names.size()-1).substr(0);
	int fd = open(outputFile.c_str(), O_WRONLY |O_CREAT, 0644); //opens the file for writing to, reading from, and creates it, if it doesn't exist.
	close(1); //we close the stdout first,
	dup(fd); //then we duplicate into file descriptor
	return fd;
}
int command::inputRedirection() //function for input redirection
{
	string inputFile = "";
	inputFile = names.at(names.size()-1).substr(0);
	int fd = open(inputFile.c_str(), O_RDONLY); //opens inputFile for reading from only.
	close(0); //We close stdin first
	dup(fd); //Then we duplicate into file descriptor
	return fd;
}

void command::checkProcess() //will check for any background processes that finish
{
	int status;
	pid_t ppid;
	while((ppid = waitpid(-1, &status, WNOHANG)) > 0) //checks to see if background process has exited
    {
    	cout << "Background process: " << ppid << " exited with status: " << WEXITSTATUS(status) << endl;
    	if(processId.size() > 0) //if there are background processes
    	{
    		for(int i = 0; i < processId.size(); i++)
    		{
    			if(ppid == processId.at(i) && processId.size() > 0) //delete the process that corresponds to the process id of the process that has exited
    			{
    				processId.erase(processId.begin()+i);
    				processCommand.erase(processCommand.begin()+i);
    			}
    		}
    	}
    }
}

void command::executableFunc(bool backg) //function for forking a child and running execv and seeing whether there is input or output redirection as well as if it is a background of interactive process and dealing with it
{
	int fd;
	pid_t pid;
	if ((pid = fork()) == 0) //if child
		{
			if(names.size() > 1)
			{
				if(names.at(names.size()-2).at(0) == '>') //sets file name for output
				{
					fd = outputRedirection();
					close(fd);
				}

				if(names.at(names.size()-2).at(0) == '<') //sets file name for input
				{
					fd = inputRedirection();
					close(fd);
				}
			}
			execv(pathname, (char**)argvec);
			_exit(1); // failure
		} //child process
		if (backg == false) //if process is interactive aka not a background process
		{
			checkProcess(); //check to see if background processes have ended and remove them from the list of processes
			while (wait(0) != pid); //wait for child to finish
		}
		else if(backg == true) //if process is a background process
		{
			processId.push_back(pid); //add the process id to the deque of process id's
			processCommand.push_back(input); //add the process command to the deque of process commands
			checkProcess(); // check to see if any background processes have finished.
		}
		cout << endl;
}

bool command::inputOperations(bool backg) //splits the user input and gets path variables and gets the path that contains the executable, also returns whether it is a background process or not so other functions can react accordingly
{
	string buffer;

	names = split(input, ' '); //split the user input by whitespace and store it into the deque names

	buffer = getenv("PATH"); //getting the full path
	pathNames = split(buffer, ':'); //Splitting full path into directories
	 
	if(names.at(0).at(0) != '/' && names.at(0) != "bg") //if the process is not a full path, or a background process, then find path and set it.
		getPath();
	else if(names.at(0) == "bg") //if process is a background process, call the background process function to remove bg from the beginnning and to set the path
	{
		backg = bgProcess(backg);
	}
	else if(names.at(0).at(0) == '/') //if full pathname/executable is given then just use the pathname/executable instead of searching for the executable in all the paths
	{
		for(int i = 0; i < names.at(0).size(); i++)
			pathname[i] = names.at(0).at(i);
	}
	return backg; //returns whether the command is a background command or not.
}