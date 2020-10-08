#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 100
#define PATH_SIZE 256
#define PIPE_SIZE 1000000
#define PIPE_READ 0
#define PIPE_WRITE 1
#define FILE_WRITE 0
#define FILE_APPEND 1
#define FILE_READ -1

int commands_stored = 0;
char **last_ten_commands;
typedef struct
{
	int file_mode;
	FILE *file_stream;
} redir_file;

void run_shell();
void handle_signal(int);
void handle_sigint();
void handle_sigquit();
char *read_command();
void add_command(char *command);
void parse_and_execute(char *user_command, redir_file rfiles);
int command_extractor(char commands[][3][BUFFER_SIZE], char *user_command);
char **argument_extractor(char *command);
char *path_to_executable(char *agrv_0);
redir_file file_redirector(char *user_command);

int main()
{
	last_ten_commands = (char **)malloc(10 * sizeof(char *));
	printf("\n");

	signal(SIGQUIT, handle_signal);
	signal(SIGINT, handle_signal);

	run_shell();
	return 0;
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
	{
		// fputc('\n', stdin);
		return;
	}
	raise(SIGKILL);
}

void handle_sigint()
{
	printf("\n");
	for (int i = 0; i < commands_stored; i++)
	{
		printf("%s\n", last_ten_commands[i]);
		free(last_ten_commands[i]);
	}
	free(last_ten_commands);
	raise(SIGKILL);
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
		redir_file rfile = file_redirector(user_command);

		/*
		 * parse user_command 
		 * check for new operators - || and |||
		 * convert to corresponding list of bash commands
		 * execute all
		 * 
		 */
		parse_and_execute(user_command, rfile);
		free(user_command);
		if (rfile.file_stream != NULL)
		{
			fclose(rfile.file_stream);
		}
	}
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
	if (strlen(user_command) > 0) //	Add a non-null command to history
		add_command(user_command);

	return user_command;
}

// add commands to last ten commands
void add_command(char *command)
{
	char *command_record = (char *)malloc((strlen(command) + 1) * sizeof(char));
	strcpy(command_record, command);
	if (commands_stored < 10)
	{
		last_ten_commands[commands_stored] = command_record;
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
		last_ten_commands[9] = command_record;
	}
}

/*
 * parse single line command into possibly
 * multiple line bash equivalents
 * execute each one
*/
void parse_and_execute(char *user_command, redir_file rfile)
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
		strcpy(input, output);
		if (i == 0 && rfile.file_mode == FILE_READ && rfile.file_stream != NULL)
		{
			fseek(rfile.file_stream, 0, SEEK_END);
			size_t bytes_to_read = ftell(rfile.file_stream);
			fseek(rfile.file_stream, 0, SEEK_SET);
			fread(input, sizeof(char), bytes_to_read, rfile.file_stream);
		}

		for (int j = 0; j < 3 && strlen(commands[i][j]) != 0; j++)
		{
			char curr_command[BUFFER_SIZE];
			strcpy(curr_command, commands[i][j]);
			char **argv = argument_extractor(curr_command);

			//	Pipes established to and from child
			int from_child[2];
			int to_child[2];
			pipe(from_child);
			pipe(to_child);

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
				dup2(to_child[PIPE_READ], STDIN_FILENO);		 //stdin of string from parent process
				close(from_child[PIPE_WRITE]);
				close(to_child[PIPE_READ]);

				char *path;
				if (strchr(argv[0], '/') == NULL)
					path = path_to_executable(argv[0]);
				else
					path = argv[0];

				if (path == NULL || access(path, X_OK) != 0)
				{
					perror("Executable not found");
					exit(1);
				}

				wait(NULL);
				execv(path, argv);
				perror("execv failed");
				exit(1);
				break;

			default:
				close(to_child[PIPE_READ]);
				close(from_child[PIPE_WRITE]);
				printf("Process id:\t%d\n", p);

				write(to_child[PIPE_WRITE], input, strlen(input));
				close(to_child[PIPE_WRITE]);
				// printf("written to pipe:-----\n%s\n------\n", input);
				wait(&status);
				size_t nbytes = read(from_child[PIPE_READ], output, PIPE_SIZE);
				output[nbytes] = '\0';
				break;
			}

			/*
			 * Final output of the command, if multiple commands
			 * on the final pipe, multiple outputs to stdout or redirected file.
			 */
			if (i == (num_commands - 1))
			{
				if (rfile.file_mode == FILE_READ)
				{
					printf("%s\n", output);
				}
				else
				{
					fwrite(output, sizeof(char), strlen(output), rfile.file_stream);
				}
			}

			// Deallocate the argument vector
			for (int index = 0; argv[index] != NULL; index++)
			{
				free(argv[index]);
			}
			free(argv);
		}
	}

	free(input);
	free(output);
	printf("command execution ended\n");
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

/* 
 * Finds full path to the executable file in PATH directories,
 * returns NULL if not found.
 */
char *path_to_executable(char *argv_0)
{
	char *path_list = getenv("PATH"), *exe_path = (char *)calloc(PATH_SIZE, sizeof(char));
	for (char *path_token = strtok(path_list, ":"); path_token != NULL; path_token = strtok(NULL, ":"))
	{
		strcat(exe_path, path_token);
		strcat(exe_path, "/");
		strcat(exe_path, argv_0);

		if (access(exe_path, X_OK) == 0)
		{
			return exe_path;
		}
		exe_path = (char *)memset(exe_path, '\0', PATH_SIZE);
	}

	free(exe_path);
	return NULL;
}

/* 
 * Extract the file name from user_command
 * and determine read or write mode for
 * the file
 */
redir_file file_redirector(char *user_command)
{
	redir_file rfile;
	rfile.file_mode = FILE_READ;
	rfile.file_stream = NULL;

	char *itr = user_command;
	while (*itr != '\0')
	{
		if (*itr == '<')
		{
			*itr = '\0';
			while (*++itr == ' ')
				;

			rfile.file_mode = FILE_READ;
			rfile.file_stream = fopen(itr, "r");
			return rfile;
		}
		else if (*itr == '>')
		{
			*itr = '\0';
			if (*(itr + 1) == '>')
			{
				printf("file opened for appending\n");
				itr++;
				while (*++itr == ' ')
					;

				printf("%s\n", itr);
				rfile.file_mode = FILE_APPEND;
				rfile.file_stream = fopen(itr, "a");
				return rfile;
			}

			while (*++itr == ' ')
				;

			rfile.file_mode = FILE_WRITE;
			rfile.file_stream = fopen(itr, "w");
			return rfile;
		}
		itr++;
	}

	return rfile;
}
