#ifndef AKINATOR_H
#define AKINATOR_H

#include "Tree.h"


enum AnswerData_t {
    STR = 0,
    INT = 1
};

union union_for_value{
    int   number;
    char* string;
};

struct Answer_t {
    AnswerData_t type;
    union union_for_value value;
};

enum AnswerPosition {
    YES = 1,
    NO  = 0
};

void Game( Tree_t* tree );

Node_t* AskQuestion ( Node_t* current );
// Node_t* AddQuestuion( Node_t* node, char* question, char* new_object, const AnswerPosition answer_for_new_question );
Node_t* AddQuestion( Tree_t* tree, Node_t* leaf, const char* new_question, char* new_object, AnswerPosition answer_for_new_question );

void TreeSaveToFile( const Tree_t* tree, const char* filename );

#endif