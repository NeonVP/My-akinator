#include <assert.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string.h>

#include "Akinator.h"
#include "Colors.h"
#include "DebugUtils.h"
#include "Tree.h"


const size_t MAX_LEN = 128;

static void ClearBuffer() {
    while ( getchar() != '\n' ) {}
}

static Answer_t* ParseAnswer( const char* allowed_answers ) {
    my_assert( allowed_answers, "Null pointer on `allowed_answers`" );

    char* answer = NULL;
    Answer_t* answer_struct = ( Answer_t* ) calloc ( 1, sizeof( *answer_struct ) );
    assert( answer_struct && "Memory allocation error for `answer_struct`" );

    if ( scanf( " %m[^\n]", &answer ) != 1 ) {
        fprintf( stderr, COLOR_BRIGHT_RED "Ошибка ввода!\n" );
        ClearBuffer();
        free( answer_struct );
        return NULL;
    }
    ClearBuffer();

    char* endptr = NULL;
    long num = strtol(answer, &endptr, 10);
    if (endptr != answer && *endptr == '\0') {
        answer_struct->type = INT;
        answer_struct->value.number = (int)num;
    } else {
        answer_struct->type = STR;
        answer_struct->value.string = strdup(answer);
    }

    free(answer);
    return answer_struct;
}


Node_t* AddQuestion( Tree_t* tree, Node_t* leaf, const char* new_question, char* new_object, AnswerPosition answer_for_new_question ) {
    my_assert( leaf && tree, "Null pointer on `leaf` or `tree`" );
    my_assert( new_question && new_object, "Null pointer on new data" );

    Node_t* question_node = ( Node_t* ) calloc ( 1, sizeof( *question_node ) );
    assert( question_node && "Memory allocation error" );

    // question_node->value = strdup( new_question );

    question_node->value = ( TreeData_t ) calloc ( strlen( new_question ) + 1, 1 );
    assert( question_node->value && "Memory allocation error" );

    memcpy( question_node->value, new_question, strlen( new_question ) );


    Node_t* object_node = NodeCreate( new_object, question_node );

    if ( answer_for_new_question == YES ) {
        question_node->left  = object_node;
        question_node->right = leaf;
    } else {
        question_node->right = object_node;
        question_node->left  = leaf;
    }

    question_node->parent = leaf->parent;
    leaf->parent = question_node;

    if ( question_node->parent ) {
        if ( question_node->parent->left == leaf )
            question_node->parent->left = question_node;
        else
            question_node->parent->right = question_node;
    } else {
        tree->root = question_node;
    }

    TreeDump( tree );
    return question_node;
}

static void PrintQuestion( const char* question ) {
    my_assert( question, "Null pointer on `question`" );

    char buffer[42] = {};
    snprintf( buffer, sizeof( buffer ), "%.40s?", question );
    fprintf( stderr,"\n┌─ ВОПРОС ──────────────────────────────────┐\n" );
    fprintf( stderr, "│ %-*s │\n", 57 - 8, buffer );
    fprintf( stderr, "└───────────────────────────────────────────┘\n" );
    fprintf( stderr, "Ответ[Y / N]: " );
}

Node_t* AskQuestion( Node_t* current ) {
    my_assert( current, "Null pointer on `current`" );

    if ( !current->left && !current->right ) {
        fprintf( stderr, COLOR_BRIGHT_YELLOW "Лист достигнут — здесь нет подветок.\n" );
        return NULL;
    }

    PrintQuestion(current->value);

    Answer_t* answer = ParseAnswer( "YES NO yes no Y N y n" );
    if ( !answer ) {
        fprintf( stderr, COLOR_BRIGHT_RED "Ошибка ввода ответа!\n" );
        return NULL;
    }

    Node_t* next = NULL;

    if ( answer->type == STR ) {
        char c = answer->value.string[0];
        if ( c == 'Y' || c == 'y' ) next = current->left;
        else                        next = current->right;
        free( answer->value.string );
    } else {
        fprintf( stderr, COLOR_BRIGHT_RED "Unknown type in answer!\n" );
        free( answer );
        return NULL;
    }

    free( answer );
    return next;
}

#ifdef _DEBUG
    static void PrintObjectTraits( const Node_t* leaf ) {
        my_assert( leaf, "Null pointer on `leaf`" );

        const Node_t* path[ MAX_LEN ] = {};
        size_t depth = 0;
        const Node_t* current = leaf;

        while ( current != NULL ) {
            path[ depth++ ] = current;
            current = current->parent;
        }

        fprintf( stderr, "\n\nОбъект \"%s\" имеет следующие признаки:\n", leaf->value );
        fprintf( stderr, "──────────────────────────────────────\n" );

        for ( size_t idx = depth - 1; idx > 0; idx-- ) {
            const Node_t* parent = path[ idx ];
            const Node_t* node   = path[ idx - 1 ];

            if ( parent->left == node) {
                fprintf( stderr, "✔ %s\n", parent->value );
            } else if ( parent->right == node ) {
                fprintf( stderr, "✖ не %s\n", parent->value );
            }
        }

        fprintf( stderr, "──────────────────────────────────────\n\n" );
    }
#endif

static void ShowMenu() {
    fprintf( stderr, "┌────────────────────────────────────────┐\n" );
    fprintf( stderr, "│             ГЛАВНОЕ МЕНЮ               │\n" );
    fprintf( stderr, "├────────────────────────────────────────┤\n" );
    fprintf( stderr, "│ 1. Начать игру                         │\n" );
    fprintf( stderr, "│ 2. Выход                               │\n" );
    fprintf( stderr, "└────────────────────────────────────────┘\n" );
    fprintf( stderr, "Выберите вариант[1, 2]: ");
}

static int GetMenuChoice() {
    Answer_t* menu_answer = ParseAnswer("1 2");
    if ( !menu_answer ) return -1;

    int choice = -1;
    if ( menu_answer->type == INT ) choice = menu_answer->value.number;

    if ( menu_answer->type == STR ) free(menu_answer->value.string);

    free(menu_answer);

    return choice;
}

static void HandleIncorrectGuess( Tree_t* tree, Node_t* leaf ) {
    char new_object[ MAX_LEN ] = {};
    char new_question[ MAX_LEN ] = {};

    fprintf( stderr, "Кто это был? " );
    scanf( " %127[^\n]", new_object );
    ClearBuffer();

    fprintf( stderr, "Чем \"%s\" отличается от \"%s\": он ", leaf->value, new_object );
    scanf( " %127[^\n]", new_question );
    ClearBuffer();

    fprintf( stderr, "Для \"%s\" ответ на вопрос будет 'Да' или 'Нет'? [Y/N]: ", new_object );
    Answer_t* ans_for_new_obj = ParseAnswer( "Y N y n" );
    if ( !ans_for_new_obj ) return;

    AnswerPosition position = ( ans_for_new_obj->value.string[0] == 'Y' || ans_for_new_obj->value.string[0] == 'y' ) ? YES : NO;
    free( ans_for_new_obj->value.string );
    free( ans_for_new_obj );

    AddQuestion( tree, leaf, new_question, new_object, position );
}

static void PlayRound( Tree_t* tree ) {
    my_assert( tree, "Null pointer on `tree`" );

    Node_t* current = tree->root;
    while ( current && current->left && current->right ) {
        current = AskQuestion( current );
    }

    if ( !current ) {
        fprintf( stderr, COLOR_BRIGHT_RED "Ошибка: дошли до NULL-узла!\n" );
        return;
    }

    fprintf( stderr, COLOR_BRIGHT_GREEN "Я думаю, это %s\n" COLOR_RESET, current->value );
    ON_DEBUG( PrintObjectTraits(current); )

    fprintf( stderr, "Я угадал? [Y/N]: " );
    Answer_t* correct_answer = ParseAnswer( "Y N y n" );
    if ( !correct_answer ) return;

    if ( correct_answer->type == STR && ( correct_answer->value.string[0] == 'N' || correct_answer->value.string[0] == 'n' ) ) {
        HandleIncorrectGuess( tree, current );
    }

    if ( correct_answer->type == STR ) free( correct_answer->value.string );
    free( correct_answer );
}

void Game( Tree_t* tree ) {
    my_assert( tree, "Null pointer on `tree`" );

    while (1) {
        ShowMenu();
        int choice = GetMenuChoice();

        switch ( choice ) {
            case 1:
                PlayRound(tree);
                break;

            case 2:
                fprintf( stderr, "Выход.\n" );
                return;

            default:
                fprintf( stderr, COLOR_BRIGHT_RED "Неверный выбор!\n" );
                break;
        }
    }
}

static void WriteNodeRec( const Node_t* node, FILE* stream ) {
    if ( !node ) {
        fprintf( stream, "nil" );
        return;
    }

    fprintf( stream, "( \"%s\" ", node->value ? node->value : "" );

    if ( node->left ) {
        WriteNodeRec( node->left, stream );
    } else {
        fprintf( stream, "nil" );
    }

    fprintf( stream, " " );

    if ( node->right ) {
        WriteNodeRec( node->right, stream );
    } else {
        fprintf( stream, "nil" );
    }

    fprintf( stream, " )" );
}

void TreeSaveToFile( const Tree_t* tree, const char* filename ) {
    my_assert( tree,     "Null pointer on tree" );
    my_assert( filename, "Null pointer on filename" );

    FILE* f = fopen( filename, "w" );
    my_assert( f, "Failed to open file for writing" );

    WriteNodeRec( tree->root, f );

    fprintf( f, "\n" );
    fclose( f );
}

// Tree_t* TreeReadFromFile() {
//     Tree_t* tree = ( Tree_t* ) calloc ( 1, sizeof( *tree ) );
//     assert( tree && "Memory error allocation" );

//     FILE* file = fopen( "dump/base.txt",  "r" );
//     assert( file && "File opening error" );



//     return  tree;
// }


// Node_t* NodeRead( FILE* stream ) {
//     assert( stream && "Null pointer on `stream`" );

//     Node_t* node = ( Node_t* ) calloc ( 1, sizeof( *node ) );
//     assert( node && "Memory allocation error" );

//     char* value = 0;
//     char* left  = 0;
//     char* right = 0;

//     int scanf_number = fscanf( stream, "( \"%m[^\"] %s %m[^]",  );
// }