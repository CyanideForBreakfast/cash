#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 20

int commands_stored = 0;
char** last_ten_commands;

void run_shell();
char* read_command();
void add_command(char*);
void parse_and_execute();

int main(){
	last_ten_commands = (char**)malloc(sizeof(char*));
	printf("\n");
	run_shell();
	return 0;
}
void run_shell(){
	char* user_command;
	while(1){
		printf("$$$ ");
		/* read till \n or EOF encountered
		 * store and return pointer to location
		 * */
		user_command = read_command();
		/*
		 * parse user_command 
		 * check for new operators - || and |||
		 * convert to corresponding list of bash commands
		 * execute all
		 * 
		 */
		parse_and_execute(user_command);
		
	}
}

/*
 * signal handling
 * handle SIGINT
 * handle SIGQUIT
 * */

/*
 * read till EOF or \n
 * allocate buffer space for storing command
 * reallocate if exceed buffer
 * add command to last_ten_commands 
*/
char* read_command(){
	char* user_command = (char*)malloc(BUFFER_SIZE*sizeof(char));
	int command_size = BUFFER_SIZE-1, index=0, reallocated = 1;
	char c=getchar();
	while(c!=EOF && c!='\n'){
		if(0<command_size--){
			user_command[index++]=c;
			c=getchar();
		}
		else{
			user_command = (char*)realloc(user_command, BUFFER_SIZE*sizeof(char)*(++reallocated));
			command_size=BUFFER_SIZE-1;
			user_command[index++]=c;
			c=getchar();
		}
	}
	user_command[index++] = '\0';
	return user_command;
}

// adds commands to last ten commands
void add_command(char* command){
	if(commands_stored<9){
		last_ten_commands[commands_stored] = command;
		commands_stored++;
	}
	else{
		free(last_ten_commands[0]);
		//shift commands list to left
		char* temp = last_ten_commands[1];
		for(int i=0;i<9;i++){
			last_ten_commands[i] = temp;
			temp=last_ten_commands[i+1];
		}
		last_ten_commands[9] = command;
	}
}
/*
 * parse single line command into possibly multiple line bash equivalents
 * execute each one
*/
void parse_and_execute(char* user_command){
	
}
