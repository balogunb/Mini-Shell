/* $begin shellmain */
#include "csapp.h"
#define MAXARGS 128
#define MAXJOBS 20
#define MAXPROC 1000
/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
int handle_env_substitute(char **argv);
int addtojobslist(pid_t __pid, char *argv);
int addproctolist(pid_t __pid, int status, clock_t startT, long minFault, long maxFault, char *argv);
void sigtstpHandler(int sig);
void sigchldHandler(int sig);
void sigintHandler(int sig);

/* struct for jobs */
struct Job
{
    int process_id;
    char argument[MAXLINE];
    int status; /* 1 represents running while 0 represents stopped */
} job;

struct ProcessStat
{
    pid_t process_id;
    int status;
    clock_t startT;
    clock_t endT;
    long minFault;
    long maxFault;
    char argument[MAXLINE];
} processstat;

/* Shell Variables */
struct Job jobs[MAXJOBS];
struct ProcessStat stats[MAXPROC];
int *jobscount;
sigset_t tstpmask, prevMask;
volatile sig_atomic_t parent_proc_id;
volatile sig_atomic_t foreground_proc_id;

/* Signal Handlers */
void sigtstpHandler(int sig)
{
    //fprintf(stderr, "Parent Proccess ID: %d\n", parent_proc_id);
    //fprintf(stderr, "Foreground Proccess ID: %d\n", foreground_proc_id);
    //fprintf(stderr, "Foreground process is still running?: %d\n", kill(foreground_proc_id,0) == 0);

    kill(parent_proc_id, SIGCONT); /** continue running parent **/

    if (kill(foreground_proc_id, 0) == 0)
    { /* If child process exists */

        for (int i = 0; i < MAXJOBS; i++)
        {
            if (jobs[i].process_id == foreground_proc_id)
            {
                jobs[i].status = 2; //Status 2 represents suspended process
                break;
            }
        }

        kill(foreground_proc_id, SIGTSTP); /** suspend child process **/
    }
}

void sigchldHandler(int sig)
{
    int saved_errno = errno;

    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0)
    {
    }
    errno = saved_errno;
}

void sigintHandler(int sig)
{
    int saved_errno = errno;

    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0)
    {
    }
    errno = saved_errno;
    kill(parent_proc_id, SIGKILL);
}

int main()
{
    char cmdline[MAXLINE]; /* Command line */
    jobscount = malloc(sizeof(int));
    *jobscount = 0;
    parent_proc_id = getpid();
    //fprintf(stderr, "Parent ID: %d\n", parent_proc_id);

    if ((sigemptyset(&tstpmask) == -1) || (sigaddset(&tstpmask, SIGTSTP) == -1)) /* add SIGTSTP to mask */
    {
        perror("Failed to initialize the signal mask\n");
        return 1;
    }

    if (signal(SIGTSTP, sigtstpHandler) == SIG_ERR) /* Handles SIGTSTP for Parent  */
    {
        fprintf(stderr, "Error Catching SIGTSTP in child\n");
        //exit(0);
    }

    if (signal(SIGINT, sigintHandler) == SIG_ERR) /* Handles SIGINT for Parent  */
    {
        fprintf(stderr, "Error Catching SIGINT\n");
        //exit(0);
    }

    struct sigaction sa;
    sa.sa_handler = &sigchldHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1)
    {
        fprintf(stderr, "Error Catching SIGCHLD in child");
        perror(0);
        exit(1);
    }

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

            setpgid(getpid(), 0);        /*set job process group ID to the child's PID */
            handle_env_substitute(argv); /*substitute environment variables */
            if (execvp(argv[0], argv) < 0)
            {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
        addtojobslist(pid, cmdline); /*Handles adding new job to jobs list */

        /* Parent waits for foreground job to terminate */
        if (!bg)
        {

            //sigprocmask(SIG_BLOCK, &tstpmask, &prevMask);
            foreground_proc_id = pid;
            //sigprocmask(SIG_SETMASK, &prevMask, &tstpmask);

            int status;
            if (waitpid(pid, &status, WUNTRACED) < 0)
                unix_error("waitfg: waitpid error");
        }
        else
            fprintf(stderr, "%d %s", pid, cmdline);
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
                fprintf(stderr, "Error unsetting environment variable\n");
            }
            return 1;
        }

        //handle setting enviroment variable
        if (setenv(variableName, value, 1) != 0)
        {
            fprintf(stderr, "Insufficient space to allocate new environment\n");
        }

        return 1;
    }

    if (!strcmp(argv[0], "jobs"))
    { /* Handles the jobs built in command */
        //fprintf(stderr, "Counting Jobs\n");
        //fprintf(stderr, "Handler Jobs: %d\n", *jobscount);

        int cnt = 0;
        for (int i = 0; i < *jobscount; i++)
        {

            int status;
            int jobstate;
            jobstate = waitpid(jobs[i].process_id, &status, WNOHANG); /*returns process id of child if status changed */

            if (jobstate == -1)
                //if (jobstate == -1 || WIFEXITED(status))
                continue; /* Do not add if process already terminated */

            if (WIFSTOPPED(status))
            {
                printf("[%d] ", ++cnt);
                printf("%d  ", jobs[i].process_id);
                jobs[i].status = 0;
                printf("Stopped   ");
                printf("%s", jobs[i].argument);
            }
            else if (jobstate == 0) /** if process is still running or suspended */
            {
                if (jobs[i].status == 2) //
                {
                    printf("[%d] ", ++cnt);
                    printf("%d  ", jobs[i].process_id);
                    jobs[i].status = 2;
                    printf("Stopped   ");
                    printf("%s", jobs[i].argument);
                }
                else //If the process is running
                {
                    printf("[%d] ", ++cnt);
                    printf("%d  ", jobs[i].process_id);
                    jobs[i].status = 1;
                    printf("Running   ");
                    printf("%s", jobs[i].argument);
                }
            }
        }
        return 1;
    }

    if (!strcmp(argv[0], "bg"))
    {
        if (argv[1] == NULL)
        {
            fprintf(stderr, "Error: Input processID or JID for process to run in background\n");
            return 1;
        }

        char *id = malloc(50 * sizeof(char));
        strcpy(id, argv[1]);

        int id_;
        if (id[0] == '%')
        { /*if it is a job id */
            id_ = atoi(id+1);
            int j_cnt = 0;

            for (int i = 0; i < MAXJOBS; i++)
            {
                if (kill(jobs[i].process_id, 0) == 0 && jobs[i].process_id != 0) /* if its a job. used to count JID */
                {
                    j_cnt++;
                }

                if (id_ == j_cnt) /* if its a job. used to count JID */
                {
                    jobs[i].status = 1; //Status 1 represents running process
                    break;
                }
            }
        }
        else /* handles process ID */
        {
            id_ = atoi(id);
            fprintf(stderr, "The id is %c PID\n", id[0]);

            for (int i = 0; i < MAXJOBS; i++)
            {
                if (jobs[i].process_id == id_)
                {
                    jobs[i].status = 1; //Status 1 represents running process
                    break;
                }
            }
        }

        kill(id_, SIGCONT); /** restart child process **/
        return 1;
    }

    if (!strcmp(argv[0], "fg")) /* handles returning back to fore ground process */
    {
        if (argv[1] == NULL)
        {
            fprintf(stderr, "Error: Input processID or JID for process to run in background\n");
            return 1;
        }

        char *id = malloc(50 * sizeof(char));
        strcpy(id, argv[1]);

        int id_;
        if (id[0] == '%')
        { /*if it is a job id */
            id_ = atoi(id+1);

            int j_cnt = 0;

            for (int i = 0; i < MAXJOBS; i++)
            {
                if (kill(jobs[i].process_id, 0) == 0) /* if its a job. used to count JID */
                {
                    j_cnt++;
                }

                if (id_ == j_cnt) /* if its a job. used to count JID */
                {
                    id_ = jobs[i].process_id;
                    jobs[i].status = 1; //Status 1 represents running process
                    break;
                }
            }
        }
        else
        { /* if process id */
            id_ = atoi(id);
            for (int i = 0; i < MAXJOBS; i++)
            {
                if (jobs[i].process_id == id_)
                {
                    jobs[i].status = 1; //Status 1 represents running process
                    break;
                }
            }
        }

        int status;
        foreground_proc_id = id_;
        kill(id_, SIGCONT); /** restart child process **/
        if (waitpid(id_, &status, WUNTRACED) < 0)
        {
            unix_error("waitfg: waitpid error");
        }
        return 1;
    }

    if (!strcmp(argv[0], "jsum"))
    {
        fprintf(stdout, "PID    Status     Elapsed Time    Min Faults    Maj Faults Command\n");

        return 1;
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

int addtojobslist(pid_t __pid, char *argv)
{
    if (*jobscount == MAXJOBS)
    {
        fprintf(stderr, "Error: Job list is full\n");
        return 1;
    }

    int status;
    //int state =
    waitpid(__pid, &status, WNOHANG);
    if (WIFEXITED(status))
        return 1; /* Do not add if process already terminated */

    jobs[*jobscount].process_id = __pid;
    jobs[*jobscount].status = 1;
    strcpy(jobs[*jobscount].argument, argv);
    int prev = *jobscount;
    *jobscount = prev + 1;
    return 1;
}