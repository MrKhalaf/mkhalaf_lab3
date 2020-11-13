//
//  tokens.h
//  mysh
//
//  Created by Dorian Arnold on 10/22/20.
//  Copyright © 2020 Dorian Arnold. All rights reserved.
//

#ifndef tokens_h
#define tokens_h

// tokens struct carries with it the numTokens
typedef struct {
    char ** tokens;
    int numTokens;
}Tokens;


//get_tokens:
//  parses line into individual (space-separated) tokens
//  input: null-terminated string
//  output: array of null-terminated strings
//          output array terminated by a NULL pointer
Tokens get_tokens( const char * line );

void free_tokens( char ** tokens );

#endif /* tokens_h */
