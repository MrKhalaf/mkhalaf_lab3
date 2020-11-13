#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

/* ------- GLOBAL ------- */
char lineBuffer[1024];

typedef struct {
    // stores information about each command
    // store operators
    bool background;
    bool ch_stdin; // change stdin, stdout
    bool ch_stdout;
    bool pipe;
    bool stdoutTrunc;
    bool exec_set;
    char * stdin;
    char * stdo;
    char * exec; // execute this
    size_t argc;
    char ** argv;
    // need something to track the args?
} Cmd;

typedef struct {
    size_t numCmds;
    // Collection of individual commands
    Cmd * commands;
    bool malformed;
} CmdLine;

typedef struct {
    char ** tokens;
    int numTokens;
}Tokens;

Tokens get_tokens( const char * line ) {
    Tokens t;
    t.tokens=NULL;
    t.numTokens = 0;
    char * line_copy;
    const char * delim = " \t\n";
    char * cur_token;

    t.tokens = (char**) malloc( sizeof(char*) );
    t.tokens[0] = NULL;

    if( line == NULL )
        return t;

    line_copy = strdup( line );
    cur_token = strtok( line_copy, delim );
    if( cur_token == NULL )
        return t;

    do {
        t.numTokens++;
        t.tokens = ( char ** ) realloc( t.tokens, (t.numTokens+1) * sizeof( char * ) );
        t.tokens[ t.numTokens - 1 ] = strdup(cur_token);
        t.tokens[ t.numTokens ] = NULL;
    } while( (cur_token = strtok( NULL, delim )) );
    free(line_copy);

    return t;
}

void free_tokens( char ** tokens ) {
    if( tokens == NULL )
        return;

    for( int i=0; tokens[i]; i++ )
        free(tokens[i]);
    free(tokens);
}

CmdLine parse_cmdline(int argc, char *argv[]) {
    // for each command, we want to separate it out
    CmdLine cmdLine;
    cmdLine.commands = (Cmd *) malloc(sizeof(Cmd)); // may need to be add NULL as first later check after
    cmdLine.numCmds = 0; // check will need to be run.
    Cmd cmd;
    cmd.argc = 0;
    int i = 0;
    while ( i < argc) {

        if (cmd.ch_stdin == true) {
            // if cmd has been piped into already leave stdin as true.
            cmd.background = cmd.ch_stdout = cmd.pipe = cmd.stdoutTrunc = cmd.exec_set = false;
        }else {
            cmd.background = cmd.ch_stdin = cmd.ch_stdout = cmd.pipe = cmd.stdoutTrunc = cmd.exec_set = false;
        }

        cmd.argv = (char **) malloc(sizeof(char *));
        cmd.argv[0] = NULL;

        while (i < argc) {
            char *arg = argv[i];
            if (strcmp(arg, "&") == 0) {
                // this would need to be the last character of the cmdLine
                if (i != argc - 1) {
                    // if it isn't then we're in the wrong ehre
                    fprintf(stderr, "Error: \"&\" must be last token on command line\n");

                } else {
                    cmd.background = true;
                }
            } else if (strcmp(arg, "<") == 0) {
                // changing stdin
                if (cmd.ch_stdin) {
                    // if we already have a stdin change here, error
                    fprintf(stderr, "Error: Ambiguous input redirection.\n");
                    break;
                }
                cmd.ch_stdin = true;
                i++; // skip over this next iteration as we're taking the file
                cmd.stdin = argv[i];
            } else if (strcmp(arg, ">") == 0) {
                if (cmd.pipe || cmd.ch_stdout || cmd.stdoutTrunc) {
                    fprintf(stderr, "Error: Ambiguous output redirection.\n");
                }
                cmd.ch_stdout = true;
                i++;
                cmd.stdo = argv[i];
            } else if (strcmp(arg, ">>") == 0) {
                if (cmd.pipe || cmd.ch_stdout || cmd.stdoutTrunc) {
                    fprintf(stderr, "Error: Ambiguous output redirection.\n");
                }
                cmd.stdoutTrunc = true;
                i++;
                cmd.stdo = argv[i];
            } else if (strcmp(arg, "|") == 0) {
                if (cmd.pipe || cmd.ch_stdout || cmd.stdoutTrunc) {
                    fprintf(stderr, "Error: Ambiguous output redirection.\n");
                }
                cmd.pipe = true;
                // we know there is another command now.
                cmdLine.numCmds++;
                cmdLine.commands[cmdLine.numCmds - 1] = cmd; // append cmd to the line
                cmdLine.commands = (Cmd *) realloc(cmdLine.commands, cmdLine.numCmds * sizeof(Cmd));
                // attempt to clear a Cmd:
                Cmd clear;
                clear.ch_stdin = true;
                clear.stdin = cmd.exec; // set the output of this as the input of this * Watch for uninit execs coming into here.
                cmd = clear;
                i++;
                break; // break out of this while loop to define defaults for next Cmd
            } else if (strcmp(arg, "exit") == 0) {
                exit(0);
            } else {
                // what we probably have is the command otherwise throw it in args
                if (!cmd.exec_set) {
                    cmd.exec = arg;
                    cmd.exec_set = true;
                } else {
                    cmd.argc++; // add an additional arg
                    cmd.argv = (char **) realloc(cmd.argv, (cmd.argc + 1) * sizeof(char *));
                    cmd.argv[cmd.argc - 1] = strdup(arg);
                    cmd.argv[cmd.argc] = NULL;
                }
            }
            i++;
        }

    }
    // need to add the last Cmd
    cmdLine.numCmds++;
    cmdLine.commands = (Cmd *) realloc(cmdLine.commands, cmdLine.numCmds * sizeof(Cmd));
    cmdLine.commands[cmdLine.numCmds - 1] = cmd; // append cmd to the line

    return cmdLine;
}



int execute_line(CmdLine line) {
    // read through each command
    int i = 0;
    int nCmds = line.numCmds;
    for (i = 0; i < nCmds; i++) {
        Cmd cmd = line.commands[i];
        int inFD = 0; // input fd
        int outFD = 1;
        char cmdBuf[1024];
        if (i == 0) {
            // if we're at the first command
            if (cmd.ch_stdin) {
                inFD = open(cmd.stdin, O_RDONLY);
                if (inFD == -1) {
                    fprintf(stderr, "Error: open(\"%s\"): %s\n", cmd.stdo, strerror(errno));
                    return -1;
                }
                dup2(inFD, 0); // replace stdin with input file
                close(inFD);
                // that may be it?
            }
            if (cmd.ch_stdout) {
                outFD = open(cmd.stdo, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP |S_IROTH);
                if (outFD == -1) {
                    fprintf(stderr, "Error: open(\"%s\"): %s\n", cmd.stdo, strerror(errno));
                    fflush(stdout);
                    return -1;
                }
                dup2(outFD, 1); // redir stdout to the file.
                close(outFD);

            }
            // ignore piping now, just a output redirection


            if (i == nCmds - 1 && cmd.background) {
                // if this is the only command, check for background

            }

            // here we fork and make the child run the process while the parent waits for foreground process
            int status, wpid;
            int pid = fork();
            if ( pid == 0){
                if (cmd.exec_set) {
                    if (execvp(cmd.exec, cmd.argv) == -1){
                        perror("execvp");
                        return -1;
                    }
                }
            } else {
                // parent: wait if foreground, don't wait if it's a background process
                if (!cmd.background) {
                    wpid = wait(&status);
                    if (wpid == -1) {
                        perror("wait");
                        return -1;
                    }
                }
                // we may be good otherwise
            }

        } else if (i == nCmds - 1) {
            // final command
            int id = fork();
            if (id == 0) {
                // child does work here
            } else {
                // parent
//                if (!cmd.background)
//                    waitpid(id)
                int e = 3;
            }

        } else {
            // somewhere in the middle
            int b = 11;
        }
    }
    return 1;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main() {

    Tokens t;
    CmdLine cmdLine;
    while(1) {
        // a struct defined in tokens.h
        printf("mysh:");
        fflush(stdout);

        // ERROR Handle later
        fgets(lineBuffer, 1024, stdin);
        t = get_tokens(lineBuffer);
        cmdLine = parse_cmdline(t.numTokens, t.tokens);

        fflush(stdout);

        if (execute_line(cmdLine) == -1) {
            // there was an error here, but no need to do anything
            int ok = 0;
        }
        free_tokens(t.tokens);
    }
    return 0;
}
#pragma clang diagnostic pop
