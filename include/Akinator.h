#include <stdio.h>
#include <stdint.h>

#ifndef AKINATOR_H
#define AKINATOR_H

#include "Tree.h"

struct Akinator_t {
    Tree_t* tree;

    char* buffer;
    char* current_position;
};

// #define CHECK_AKINATOR

Akinator_t* AkinatorCtor();
void        AkinatorDtor( Akinator_t** akinator );

// void Game( Tree_t* tree );
void Game( Akinator_t* akinator );

#endif