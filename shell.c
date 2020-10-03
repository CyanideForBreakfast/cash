#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define BUFFER_SIZE 20

int commands_stored = 0;
char **last_ten_commands;

void run_shell();
void handle_signal(int);
void handle_sigint();
void handle_sigquit();
char *read_command();
void add_command(char *);
void parse_and_execute();

int main()
{
	last_ten_commands = (char **)malloc(10 * sizeof(char *));
	printf("\n");

	signal(SIGQUIT, handle_signal);
	signal(SIGINT, handle_signal);

	run_shell();
	return 0;
}

void run_shell()
{
	char *user_command;
	while (1)
	{
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

void handle_signal(int signum)
{
	if (signum == 3)
	{
		handle_sigquit();
		return;
	}
	if (signum == 2)
	{
		handle_sigint();
		return;
	}
}

void handle_sigquit()
{
	char c = '\0';
	while (c != 'y' && c != 'n')
	{
		printf("\nDo you really want to exit (y/n)?");
		c = getchar();
	}
	if (c == 'n')
		return;
	raise(SIGKILL);
}

void handle_sigint()
{
	printf("\n");
	for (int i = 0; i < commands_stored; i++)
	{
		printf("%s\n", last_ten_commands[i]);
	}
	raise(SIGKILL);
}

/*
 * read till EOF or \n
 * allocate buffer space for storing command
 * reallocate if exceed buffer
 * add command to last_ten_commands 
*/
char *read_command()
{
	char *user_command = (char *)malloc(BUFFER_SIZE * sizeof(char));
	int command_size = BUFFER_SIZE - 1, index = 0, reallocated = 1;
	char c = '\0';
	c = getchar();
	while (c != EOF && c != '\n')
	{
		if (0 < command_size--)
		{
			user_command[index++] = c;
			c = getchar();
		}
		else
		{
			user_command = (char *)realloc(user_command, BUFFER_SIZE * sizeof(char) * (++reallocated));
			command_size = BUFFER_SIZE - 1;
			user_command[index++] = c;
			c = getchar();
		}
	}
	user_command[index++] = '\0';
	add_command(user_command);
	return user_command;
}

// add commands to last ten commands
void add_command(char *command)
{
	if (commands_stored < 10)
	{
		last_ten_commands[commands_stored] = command;
		commands_stored++;
	}
	else
	{
		free(last_ten_commands[0]);
		//shift commands list to left
		for (int i = 0; i < 9; i++)
		{
			last_ten_commands[i] = last_ten_commands[i + 1];
		}
		last_ten_commands[9] = command;
	}
}

/*
 * parse single line command into possibly multiple line bash equivalents
 * execute each one
*/
void parse_and_execute(char *user_command)
{
	
}
