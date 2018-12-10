#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

char **split_line(char *line);
int execute(char **args);

int main(int argc, char **argv) {
	
	int s;
	srand(time(NULL));

	char *line;
	char oper[] = "./Three --total=100000 --block=10000 --play=load=weights.bin save=weights.bin alpha=0.1"; //--evil=seed=";
	char *seed;
    char **args;
    int status;

	do {
		line = malloc(100 * sizeof(char));
		seed = malloc(5 * sizeof(char));
		s = (rand() % (9999-1000+1)) + 1000;
		sprintf(seed, "%d", s);
		strcat(line, oper);
		//strcat(line, seed);

		args = split_line(line);
        status = execute(args);

        free(args);
    	args = NULL;
		free(line);
		line = NULL;
		free(seed);
		seed = NULL;
	} while(status);

	return 0;
}

char **split_line(char *line) {

    char* delim = " \n\t\a";
    int position = 0;
    char **tokens = malloc(30 * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "memory: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, delim);
    while (token != NULL) {
		tokens[position] = token;
        position++;
        token = strtok(NULL, delim);
    }

    tokens[position] = NULL;

    return tokens;
}

int execute(char **args) {
    if (args[0] == NULL) {
      return 1;
    }

    pid_t pid;
    int status;

    pid = fork();
    if (pid > 0) {
        waitpid(pid, &status, 0);
    } else if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
    } else {
        perror("fork");
    }

    return 1;
}

void free_memory (char *line, char **args) {
    free(line);
    line = NULL;
    free(args);
    args = NULL;
}
