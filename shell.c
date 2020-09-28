#include <stdio.h>

int main(){
	run_shell();
	return 0;
}
void run_shell(){
	char* user_command;
	while(1){
		printf("\n$$$ ");
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

char* read_command(){
	
}

void parse_and_execute(char* user_command){
	
}
