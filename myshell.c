#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUF_MAX 1024
#define DIR_MAX 200
#define CMD_MAX 100
#define TKN_MAX 100
#define TKN_LEN 100

typedef struct cmd_struct {
	char ***cmds;
	int index;
} cmd_struct;

int run_command(char***);
int lstrip(char*, int);
void free_mem(char***);
void ch_dir(char*);
void get_cdir(char*);
cmd_struct parse_command(char*);


int main()
{
    char input[BUF_MAX], cdir[DIR_MAX];
    char *pos;
	int i, ret = 0;
	cmd_struct cmd;

	while(1)
    {
        get_cdir(cdir);
		printf("%s $ ", cdir);
        if ((fgets(input, BUF_MAX, stdin) == NULL) || (strlen(input) == 1))
            continue;

        if ((pos = strchr(input, '\n')) != NULL) 
            *pos = '\0';
        else 
        {
            printf("buffer overflow... max char: %d\n", BUF_MAX); 
            continue;
        }

        i = lstrip(input, 0);
        if (strncmp(input + i, "exit", 4) == 0)
        {
            i = lstrip(input, i + 4);
            if (*(input + i) == '\n' || *(input + i) == '\0') 
                exit(EXIT_SUCCESS);
            else 
            {
                printf("command: \"%s\" not found\n", input); 
                continue;
            }
        }
        else if (strncmp(input + i, "cd", 2) == 0)
        {
            i = lstrip(input, i + 2);
            ch_dir(input + i);
        }
        else 
        {
            cmd = parse_command(input);
            ret = run_command(cmd.cmds);

            if(ret <= 0)
                printf("invalid command\n");

            free_mem(cmd.cmds);
        }
    }

    return (0);
}

cmd_struct parse_command(char *buf){
	cmd_struct cmd;
	char ***commands;
    char **tokens = NULL;
	char *p = buf, *token;
    int cmd_num = 0,
        tkn_num = 0,
        tkn_len = 0;

	if ((commands = malloc(CMD_MAX * sizeof(char**))) == NULL)
    {
		fprintf(stderr, "malloc() failed in line %d\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	while(1)
    {
        while (*p == ' ') p++; /* strip trailing whitespace */

		if(*p == '|' || *p == '\0')
        {
			if(tkn_num == 0)
            {
				printf("invalid command\n");
				cmd_struct tup_ret;
				tup_ret.index = -1;
				return tup_ret;
			}

			*(tokens + tkn_num) = NULL;
			*(commands + cmd_num) = tokens;
			cmd_num ++;
			
			if(*p == '\0')
				break;
			else
            {
				tkn_num = 0;
				p++;
				continue;
			}
		}
		
		if(tkn_num ==0)
        {
            if ((tokens = malloc(TKN_MAX * sizeof(char *))) == NULL)
            {
                fprintf(stderr, "malloc() failed in line %d\n", __LINE__);
                exit(EXIT_FAILURE);
            }

		}
		tkn_num++;
		
        if ((token = malloc(TKN_LEN * sizeof(char))) == NULL)
        {
            fprintf(stderr, "malloc() failed in line %d\n", __LINE__);
            exit(EXIT_FAILURE);
        }
		tkn_len = 0;
		
		while(*p != '\0'&& *p != ' ' && *p!= '|')
        {
			*(token + tkn_len) = *p;
			p++;
			tkn_len++;
		}
		*(token + tkn_len) = '\0';
		*(tokens + tkn_num - 1) = token;
	}

	commands[cmd_num] = NULL;	
	cmd.cmds = commands;

	return (cmd);
}


int run_command(char ***command)
{
	char ***ptr = command;
    char **cmd;
	int *pipe_fd, *opipe_fd, *pipe_list, *p;
	int status, pipe_id, ret, i, 
        cmd_num = 0;

	for(ptr = command; *ptr != NULL; ptr ++)
		cmd_num++;

	if ((pipe_list = malloc(cmd_num * 2 * sizeof(int))) == NULL)
    {
        fprintf(stderr, "malloc() failed in line %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }
	
    cmd_num = 0;
	for(ptr = command, pipe_fd = pipe_list; *ptr != NULL; ptr ++, 
        pipe_fd += 2)
    {
		cmd_num++; 	
		if(cmd_num > 1)
			opipe_fd = pipe_fd - 2;
		
        if ((pipe_id = pipe(pipe_fd)) < 0)
        {
			perror(NULL); 
			free_mem(command);
			exit(EXIT_FAILURE);
		}
        
        if((ret = fork()) == 0)
        {
			if(*(ptr + 1) != NULL)
            {
				if(dup2(pipe_fd[1], 1) < 0)
                {
					perror(NULL);
					exit(EXIT_FAILURE);
				}
			}
            
            if(cmd_num > 1)
            {
				if(dup2(opipe_fd[0], 0) < 0 )
                {
					perror(NULL);
					exit(EXIT_FAILURE);
				}	
			}

            p = NULL;
			for(p = pipe_fd; p >= pipe_list; p-=2)
            {
                close(p[0]);
                close(p[1]);
			}

			cmd = *ptr; 
            if ((ret = execvp(cmd[0], cmd)) == -1)
            {
                perror(NULL);
                exit(EXIT_FAILURE);
            }
		}
	}
	
	for(i = 0; i < cmd_num; i++)
    {	
        close(*(pipe_list + 2 * i));
        close(*(pipe_list + 2 * i + 1));
        wait(&status);
        
        if(WEXITSTATUS(status));
	}
	free(pipe_list);

	return (cmd_num);
}

void free_mem(char ***mem)
{
	char ***ptr;
	char **ptr_tkn;

	for(ptr = mem; *ptr != NULL; ptr++)
    {
        for(ptr_tkn = *ptr; *ptr_tkn != NULL; ptr_tkn++)
        {
			if(*ptr_tkn)
				free(*ptr_tkn);
		}
		if(*ptr)
			free(*ptr);
	}
	if(mem)
		free(mem);
}

void ch_dir(char *dir)
{
    char *pos;

    if ((pos = strchr(dir, ' ')) != NULL)
        *pos = '\0';
    if (chdir(dir) == -1)
        printf("cd : \"%s\" no such directory\n", dir);
}

int lstrip(char *str, int index)
{
    while (*(str + index) == ' ') index++;

    return (index);
}

void get_cdir(char *str)
{
    if (getcwd(str, DIR_MAX) == NULL)
        perror(NULL);
}

