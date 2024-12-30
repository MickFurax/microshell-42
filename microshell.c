#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void err(char *msg, char *precision)
{
    while (msg && *msg)
        write(2, msg++, 1);
    precision ? err(precision, NULL) : write(2, "\n", 1);
}

int cd(char **args)
{
    if (args[0] && !args[1])
    {
        if (chdir(args[0]) == -1)
        {
            err("error: cd: cannot change directory to ", args[0]);
            return 1;
        }
    }
    else
        err("error: cd: bad arguments", NULL);
    return 0;
}

void execute_pipe(char **cmds, char **env)
{
    int i = 0;
    while (cmds[i] && strcmp(cmds[i], "|"))
        i++;
    char *stock = cmds[i];
    cmds[i] = NULL;
    int fd[2];
    if (pipe(fd) == -1)
    {
        err("error: fatal", NULL);
        exit(1);
    }
    if (!strcmp(cmds[0], "cd"))
        cd(cmds + 1);
    else
    {
        if (!fork())
        {
            close(fd[0]);
            if (stock)
                dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            if (execve(cmds[0], cmds, env) == -1)
            {
                err("error: cannot execute ", cmds[0]);
                exit(1);
            }
            exit(0);
        }
        else
        {
            close(fd[1]);
            if (stock && cmds[i + 1])
                dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            waitpid(-1, NULL, 0);
            if (stock && cmds[i + 1])
                execute_pipe(cmds + i + 1, env);
        }
    }
    cmds[i] = stock;
}

void microshell(char **cmds, char **env)
{
    if (cmds[0] && !strcmp(cmds[0], ";"))
    {
        if (cmds[1])
        {
            microshell(cmds + 1, env);
            return;
        }
        else
            exit(0);
    }
    int i = 0;
    while (cmds[i] && strcmp(cmds[i], ";"))
        i++;
    char *stock = cmds[i];
    cmds[i] = NULL;
    execute_pipe(cmds, env);
    cmds[i] = stock;
    if (cmds[i] && cmds[i + 1])
        microshell(cmds + i + 1, env);
}

int main(int argc, char **argv, char **env)
{
    if (argc > 1)
        microshell(argv + 1, env);
    return 0;
}
