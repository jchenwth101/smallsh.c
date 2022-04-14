# smallsh.c
This is a smallish.c Operating Systems project written in C language.  This should be run in a GCC compiler.


to compile:  gcc -std=c11 -Wall -Werror -g3 -O0 smallsh.c -o smallsh  and then run ./smallsh from $
to compile script: chmod +x ./p3testscript and then run ./p3testscript 2>&1 from $

In this:  A smallsh or a shell written in C language. This smallsh (small shell) will implement a subset of features of well-known shells, such as bash. This program provides:

1. Provides a prompt for running commands
2. Handles blank lines and comments, which are lines beginning with the # character
3. Provide expansion for the variable $$
4. Executes 3 commands exit, cd, and status via code built into the shell
5. Executes other commands by creating new processes using a function from the exec family of functions
6. Supports input and output redirection
7. Supports running commands in foreground and background processes
8. Implements custom handlers for 2 signals, SIGINT and SIGTSTP
