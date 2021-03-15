Aim:

This minin-shell was created in order to help us understand several concepts surrounding in the OS course. These concepts include; forking, signalhandling, error handling, and environment variables. We created a new shell/pseudo shell that mimicks shell behaviour, by inheriting some default shell instructions and extending with new commands.  

Time management:

02/28/2021-03/07/2021: Theory and Research

During this period we divided concepts according to their grouping in the instructions and studied each per day. We studied the fundamantals and researched about the concept in regards to our mini shell project. E.g On the first day we researched about environments and environment variables.

03/07/2021-03/14/2021: Implementation and testing

During this period we implemented group of instructions per day. For example on the first day we implemented only Group 1. However, since we aportioned time equally to all groups without factoring increasing difficulty in further instructions, we fell short of time, e.g Group 3 took three days to implement. We intended to to testing and documentation only on the last 2 days, Saturday and Sunday.

Work Division: 

During implementation both Taku and Basit did research and read the man page, Taku did the research to debug errors and testing, and Basit wrote down the code. 

Obstacles:

Instructions were difficult to read. It was hard to differentiate between what was required and was just being provided as general info. Information of a single part was scattered in different places, e.g info about signals can be found in general infor, Group 3 and Important Notes...

Summary of each Group:

Group 1
Custom name shell to lsh by printing lsh> in a while(true), before shell receives arguments or print anything else.
Studied environment variables
Used get_env to inherit the environment of the shell in order to inherit commands such as ls
Used set_env to allow user to change environment variables and inherit commands that they desire from chosen environment
Used unset in order to remove environment variables when user types variableName=
 
Group 2
Parsed user in put in order to look for ‘$’ and changed environment, if need be, in that of $PATH in order to inherit that environment and run given command
Passed current environment to child processes after fork() so child environment also inherits shell commands
 
Group 3
Read  Computer Systems: A Programmer's Perspective
Created and struct (list) of jobs in order to track jobs 
Test
Implemented job identification of for user to call jobs and get status of that job
Check if last char is ‘&’ and run process in background if so
Test
Create method to printf the list of jobs, their id, status and initiating command
Handle signals in order to continue stoppped processes using SIGCONT, and making them back or foreground, depending on user’s choice


