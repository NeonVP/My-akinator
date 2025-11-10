#include <assert.h>
#include <string.h>

#include "Akinator.h"
#include "DebugUtils.h"

static Answer_t AnswerParsing() {
    char* answer = 0;

    scanf( "%ms", &answer );
    assert( answer && "Error in answer agter %ms" );

    if ( strncmp( answer, "YES", 3 ) == 0 || strncmp( answer, "Y", 1 ) ) {
        return YES;
    }
    else {
        return NO;
    }
}

// Node_t* AddQuestuion( Node_t* node, char* question, char* new_object, const Answer_t answer_for_new_question ) {
//     my_assert( node,                    "Null pointer on `node`" );
//     my_assert( question,                "Null pointer on `question`" );
//     my_assert( new_object,              "Null pointer on `new_object`" );
//     my_assert( answer_for_new_question, "Null pointer on `answer_for_new_question`" );

//     Node_t* old_node = node;


// }