//
//  parse.c
//  mysh
//
//  Created by Dorian Arnold on 10/22/20.
//  Copyright © 2020 Dorian Arnold. All rights reserved.
//

#include "tokens.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
