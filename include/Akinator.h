#ifndef AKINATOR_H
#define AKINATOR_H

#include "Tree.h"

struct Akinator_t {
    Tree_t* tree;

    char* base_path;
};

Akinator_t* AkinatorCtor();
void        AkinatorDtor( Akinator_t** akinator );

void AkinatorGame( Akinator_t* akinator );

#endif