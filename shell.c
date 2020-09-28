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

/*
 * signal handling
 * handle SIGINT
 * handle SIGQUIT
 * */

/*
 * read till EOF or \n
 * allocate buffer space for storing command
 * reallocate if exceed buffer 
*/
char* read_command(){
	
}

/*
 * parse single line command into possibly multiple line bash equivalents
 * execute each one
*/
void parse_and_execute(char* user_command){
	
}
