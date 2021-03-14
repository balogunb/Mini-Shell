/* $begin shellmain */
#include "csapp.h"
#define MAXARGS 128
#define MAXJOBS 20

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
int handle_env_substitute(char **argv);
int addtojobslist(pid_t __pid, char *argv, int status);

/* struct for jobs */
struct Job
{
    int process_id;
    char argument[MAXLINE];
    int status; /* 1 represents running while 0 represents stopped */
} job;

/* Shell Variables */
struct Job jobs[MAXJOBS];
int *jobscount;

int main()
{
    char cmdline[MAXLINE]; /* Command line */
    jobscount = malloc(sizeof(int));
    *jobscount = 0;
    while (1)
    {
        /* Read */
        printf("lsh> ");
        Fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin))
            exit(0);

        /* Evaluate */
        eval(cmdline);
    }
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline)
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return; /* Ignore empty lines */

    if (!builtin_command(argv))
    {

        if ((pid = Fork()) == 0)
        { /* Child runs user job */

            //fprintf(stderr, "before adding to joblist\n");

            //fprintf(stderr, "adding to joblist completed\n");
            handle_env_substitute(argv); /*substitute environment variables */
            setpgid(getpid(), 0);        /*set job process group ID to the child's PID */

            if (execvp(argv[0], argv) < 0)
            {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }

        addtojobslist(pid, cmdline, bg); /*Handles adding new job to jobs list */
        //fprintf(stderr, "Jobs: %d\n", *jobscount);

        /* Parent waits for foreground job to terminate */
        if (!bg)
        {
            int status;
            if (waitpid(pid, &status, 0) < 0)
                unix_error("waitfg: waitpid error");
        }
        else
            printf("%d %s", pid, cmdline);
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
    if (!strcmp(argv[0], "quit")) /* quit command */
        exit(0);
    if (!strcmp(argv[0], "&")) /* Ignore singleton & */
        return 1;
    if (strchr(argv[0], '=') != NULL)
    { /* handle environment variables */
        char *variableName = strtok(argv[0], "=");

        if (variableName == NULL) /* Invalid enviroment variable format */
        {
            fprintf(stderr, "= :Environment variable format error \n");
            return 1;
        }

        char *value = strtok(NULL, "=");

        if (value == NULL)
        { /* Handles removing environment variable for variableName */
            if (unsetenv(variableName) == -1)
            {
                fprintf(stderr, "Error unsetting environment variable");
            }
            return 1;
        }

        //handle setting enviroment variable
        if (setenv(variableName, value, 1) != 0)
        {
            fprintf(stderr, "Insufficient space to allocate new environment");
        }

        return 1;
    }

    if (!strcmp(argv[0], "jobs"))
    { /* Handles the jobs built in command */
        //fprintf(stderr, "Counting Jobs\n");
        //fprintf(stderr, "Handler Jobs: %d\n", *jobscount);
        for (int i = 0; i < *jobscount; i++)
        {
            int status;
            jobs[i].status = waitpid(jobs[i].process_id,&status,WNOHANG) == 0;/* if waitpid is 0 then child is still running */
            printf("[%d] ", i + 1);
            printf("%d  ", jobs[i].process_id);
            if (jobs[i].status)
            {
                printf("Running   ");
            }
            else
            {
                printf("Stopped   ");
            }

            printf("%s ", jobs[i].argument);
            printf("\n");
        }

        return 1;
    }

    if (!strcmp(argv[0], "bg")){
            if(argv[1] == NULL){
                
            }

    }

    return 0; /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv)
{
    char *delim; /* Points to first space delimiter */
    int argc;    /* Number of args */
    int bg;      /* Background job? */

    buf[strlen(buf) - 1] = ' ';   /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' ')))
    {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;

    if (argc == 0) /* Ignore blank line */
        return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
        argv[--argc] = NULL;

    return bg;
}
/* $end parseline */

int handle_env_substitute(char **argv)
{
    int i = 0;

    while (argv[i] != NULL)
    {
        if (strchr(argv[i], '$') == argv[i])
        { /*checks if the environemnt needs to be substituted */

            if (getenv(argv[i] + 1) != NULL)
            {
                argv[i] = getenv(argv[i] + 1); /* Replaces value from env */
            }
        }
        i++;
    }
    return 0;
}

int addtojobslist(pid_t __pid, char *argv, int status)
{
    if (*jobscount == MAXJOBS)
    {
        fprintf(stderr, "Error: Job list is full");
        return 1;
    }

    jobs[*jobscount].process_id = __pid;
    jobs[*jobscount].status = status;
    strcpy(jobs[*jobscount].argument, argv);
    int prev = *jobscount;
    *jobscount = prev + 1;
    return 1;
}