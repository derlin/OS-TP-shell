This is a simple shell, which normally handles common commands and does not crash upon errors. The skeleton was provided. 

Context:        Techniques avancées de programmation, 
                Systèmes d'exploitation 2, I3
Organization:   EIA-FR
Date:           19 Dec. 2014
Author:         Lucy Linder

Purpose: 
This TP should help us understand the logic behind a shell and learn the basics of system calls in Linux using the C language.

----------------------------------------------------------------------------

# Tasks
A skeleton is provided. Students have to:
- implement the invoke command (execute external commands), allowing pipes and redirects
- add support for variable expansion
- implement history and the commands h1-h10
- add support for redirect in built-in commands
- implement the cd command

----------------------------------------------------------------------------

# Notes

I added a comment at the top of each created/modified file, which explains the purpose, modifications, todos and/or bugs that could persist. Please, refer to them if you have a doubt.

The makefile provided two commands: all and clean.
The .o and .d are created in the bin/ directory. If you run it on different systems, don't forget to make clean before trying to compile again.

I noticed some bugs in the provided implementation and also improved the way it coped with user errors. For more details, see the file headers (mostly my_shell.c).

