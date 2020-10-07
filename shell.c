#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 100
#define PIPE_SIZE 1000000
#define PIPE_READ 0
#define PIPE_WRITE 1

FILE *file;
int commands_stored = 0, file_mode = 1; // file_mode = {0: read from file, 1: write to file}
char **last_ten_commands;

void run_shell();
void handle_signal(int);
void handle_sigint();
void handle_sigquit();
char *read_command();
void add_command(char *);
void parse_and_execute();
int command_extractor(char commands[][3][BUFFER_SIZE], char *user_command);
char **argument_extractor(char *command);
FILE *file_redirector(char *user_command);

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
		
		// Set file pointer to handle redirection
		file = file_redirector(user_command);

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
	do
	{
		c = getchar();
	} while (c == ' ');

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
	if (strlen(user_command) > 0)	//	Add a non-null command to history
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
	char commands[10][3][BUFFER_SIZE];
	for (size_t i = 0; i < 10; i++)
	{
		for (size_t j = 0; j < 3; j++)
		{
			memset(commands[i][j], '\0', BUFFER_SIZE);
		}
	}

	int num_commands = command_extractor(commands, user_command);

	char *input = (char *)calloc(PIPE_SIZE, sizeof(char));
	char *output = (char *)calloc(PIPE_SIZE, sizeof(char));

	for (int i = 0; i < num_commands; i++)
	{
		//	Pipes established to and from child
		int from_child[2];
		int to_child[2];
		pipe(from_child);
		pipe(to_child);
		strcpy(input, output);

		for (int j = 0; j < 3 && strlen(commands[i][j]) != 0; j++)
		{
			char curr_command[BUFFER_SIZE];
			strcpy(curr_command, commands[i][j]);

			//	Argument vector for execv call
			char **argv = argument_extractor(curr_command);
			// for (size_t k = 0; argv[k] != NULL; k++)
			// {
				// printf("argument\t%ld\t%s\n", k, argv[k]);
			// }

			pid_t p = fork();
			int status;
			switch (p)
			{
			case -1:
				perror("fork\n");
				exit(1);
				break;

			case 0:
				close(to_child[PIPE_WRITE]);
				close(from_child[PIPE_READ]);
				dup2(from_child[PIPE_WRITE], STDOUT_FILENO); //stdout to string to parent process
				dup2(to_child[PIPE_READ], STDIN_FILENO);		//stdin of string from parent process
				close(from_child[PIPE_WRITE]);
				close(to_child[PIPE_READ]);

				// sleep(2);
				// char str[PIPE_SIZE];
				// printf("child\n");
				// read(to_child[PIPE_READ], str, 1000000);
				// scanf("%s", str);
				// printf("%s\n", str);
				// wait(NULL);
				// int a = 0;
				// char ch;
				// while ((ch = getchar()) != '\0')
					// str[a++] = ch;
				
				// perror(str);
				// printf("reached\t%lu\n", strlen(str));

				wait(NULL);
				execvp(argv[0], argv);
				perror("execv failed\n");
				exit(1);
				break;

			default:
				close(to_child[PIPE_READ]);
				close(from_child[PIPE_WRITE]);
				printf("\nProcess id:\t%d\t%s\n", p, argv[0]);
				
				write(to_child[PIPE_WRITE], input, strlen(input) + 1);
				close(to_child[PIPE_WRITE]);
				printf("written to pipe:-----\n%s\n------\n", input);
				// getchar();
				wait(&status);
				read(from_child[PIPE_READ], output, PIPE_SIZE);
				break;
			}

			// Deallocate the argument vector
			for (int index = 0; argv[index] != NULL; index++)
			{
				free(argv[index]);
			}
			free(argv);
		}
	}

	printf("%s\n", output);
	printf("command execution ended\n\n");
}

int command_extractor(char commands[][3][BUFFER_SIZE], char *user_command)
{
	int num_commands = 0;
	char ch, *itr = user_command;

	while (*itr != '\0' && *itr != '\n')
	{
		int num_pipes = 0;
		if (num_commands != 0)
			while (*itr == '|')
			{
				num_pipes++;
				itr++;
			}
		else
			num_pipes = 1;

		for (int p = 0; p < num_pipes; p++)
		{
			// ignore leading white-spaces before each command
			while (*itr == ' ')
				itr++;

			int ch_index = 0;
			while (*itr != '|' && *itr != '\0' && *itr != '\n')
			{
				if (*itr == ',')
				{
					ch_index++;
					itr++;
					break;
				}
				commands[num_commands][p][ch_index++] = *itr++;
			}

			// ignore trailing white-spaces
			while (commands[num_commands][p][ch_index] == ' ')
			{
				commands[num_commands][p][ch_index--] = '\0';
			}
		}
		num_commands++;
	}

	return num_commands;
}

char **argument_extractor(char *command)
{
	int k = 0;
	char *arg_token = strtok(command, " ");
	char **argv = NULL;
	while (arg_token != NULL)
	{
		argv = (char **)realloc(argv, (k + 1) * sizeof(char *));
		argv[k] = (char *)malloc(BUFFER_SIZE * sizeof(char));

		strcpy(argv[k], arg_token);
		arg_token = strtok(NULL, " ");
		k++;
	}
	argv = (char **)realloc(argv, (k + 1) * sizeof(char *));
	argv[k] = NULL;

	return argv;
}

FILE *file_redirector(char *user_command)
{
	char *itr = user_command, redirect_sign, *temp;
	while (*itr != '\0')
	{
		if (*itr == '<' || *itr == '>')
		{
			redirect_sign = *itr;
			temp = itr;
			*itr++ = '\0';
			while (*itr == ' ')
				itr++;

			switch (redirect_sign)
			{
			case '<':
				file_mode = 0;
				printf("file opened for reading:\t%s\n", itr);
				file = fopen(itr, "r");
				break;

			case '>':
				file_mode = 1;
				printf("file opened for writing:\t%s\n", itr);
				file = fopen(itr, "w+");
				*temp = redirect_sign;
				return file;
				break;

			default:
				break;
			}
			printf("Main command:\t%s", user_command);
			break;
		}
		itr++;
	}

	return stdout;
}
