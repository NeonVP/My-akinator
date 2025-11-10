#ifndef AKINATOR_H
#define AKINATOR_H

#include "Tree.h"


enum Answer_t {
    YES = 1,
    NO  = 0
};

Node_t* AskQuestion( Node_t* current );
Node_t* NextNode( Node_t* current, Answer_t answer );

Node_t* AddQuestuion( Node_t* node, char* question, char* new_object, const Answer_t answer_for_new_question );

void    SaveTree( Tree_t* tree );
Node_t* LoadTree();




#endif