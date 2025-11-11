#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>

#include "Akinator.h"
#include "Colors.h"
#include "DebugUtils.h"


const size_t MAX_LEN = 128;

static void ClearBuffer() {
    while (getchar() != '\n') {}
}

static Answer_t* ParseAnswer(const char* string_with_answers) {
    my_assert(string_with_answers, "Null pointer on `string_with_answers`");

    char* answer = NULL;
    Answer_t* answer_struct = (Answer_t*) calloc(1, sizeof(*answer_struct));
    assert(answer_struct && "Memory allocation error for `answer_struct`");

    // Считать строку целиком (с пробелами, до перевода строки)
    if (scanf(" %m[^\n]", &answer) != 1) {
        fprintf(stderr, COLOR_BRIGHT_RED "Ошибка ввода!\n");
        ClearBuffer();
        free(answer_struct);
        return NULL;
    }
    ClearBuffer(); // очистить '\n' из буфера

    // Попробуем интерпретировать как число
    char* endptr = NULL;
    long num = strtol(answer, &endptr, 10);

    if (endptr != answer && *endptr == '\0') {
        // строка — это целое число
        answer_struct->type = INT;
        answer_struct->value.number = (int)num;
    } else {
        // строка — это текст
        answer_struct->type = STR;
        answer_struct->value.string = strdup(answer);
    }

    free(answer);
    return answer_struct;
}

Node_t* AddQuestion(Tree_t* tree, Node_t* leaf, const char* new_question, char* new_object, AnswerPosition answer_for_new_question) {
    my_assert(leaf && tree, "Null pointer on `leaf` or `tree`");
    my_assert(new_question && new_object, "Null pointer on new data");

    Node_t* question_node = (Node_t*) calloc(1, sizeof(*question_node));
    question_node->value = strdup(new_question);

    Node_t* object_node = NodeCreate(new_object, question_node);

    if (answer_for_new_question == YES) {
        question_node->left  = object_node;
        question_node->right = leaf;
    } else {
        question_node->right = object_node;
        question_node->left  = leaf;
    }

    question_node->parent = leaf->parent;
    leaf->parent = question_node;

    if (question_node->parent) {
        if (question_node->parent->left == leaf)
            question_node->parent->left = question_node;
        else
            question_node->parent->right = question_node;
    } else {
        tree->root = question_node; // если leaf был корнем, новый вопрос становится корнем
    }

    return question_node;
}




static void print_question(const char* question) {
    my_assert( question, "Null pointer on `question`" );

    char buffer[42] = {};
    snprintf( buffer, sizeof( buffer ), "%.40s?", question );
    fprintf( stderr,"\n┌─ ВОПРОС ──────────────────────────────────┐\n" );
    fprintf( stderr, "│ %-*s │\n", 57 - 8, buffer );
    fprintf( stderr, "└───────────────────────────────────────────┘\n" );
    fprintf( stderr, "Ответ[Y / N]: " );
}

Node_t* AskQuestion(Node_t* current) {
    my_assert(current, "Null pointer on `current`");

    if (!current->left && !current->right) {
        fprintf(stderr, COLOR_BRIGHT_YELLOW "Лист достигнут — здесь нет подветок.\n");
        return NULL;
    }

    print_question(current->value);

    Answer_t* answer = ParseAnswer("YES NO yes no Y N y n");
    if (!answer) {
        fprintf(stderr, COLOR_BRIGHT_RED "Ошибка ввода ответа!\n");
        return NULL;
    }

    Node_t* next = NULL;

    if (answer->type == STR) {
        char c = answer->value.string[0];
        if (c == 'Y' || c == 'y') next = current->left;
        else                      next = current->right;
        free(answer->value.string);
    } else {
        fprintf(stderr, COLOR_BRIGHT_RED "Unknown type in answer!\n");
        free(answer);
        return NULL;
    }

    free(answer);
    return next;
}

static void print_menu() {
    fprintf( stderr, "┌────────────────────────────────────────┐\n");
    fprintf( stderr, "│             ГЛАВНОЕ МЕНЮ               │\n");
    fprintf( stderr, "├────────────────────────────────────────┤\n");
    fprintf( stderr, "│ 1. Начать игру                         │\n");
    fprintf( stderr, "│ 2. Выход                               │\n");
    fprintf( stderr, "└────────────────────────────────────────┘\n");
    fprintf( stderr, "Выберите вариант[1, 2]: ");
}

static void PrintObjectTraits( const Node_t* leaf ) {
    my_assert(leaf, "Null pointer on `leaf`");

    const Node_t* path[ MAX_LEN ] = {};
    size_t depth = 0;
    const Node_t* current = leaf;

    while ( current != NULL ) {
        PRINT( "TRAIT - %s \n", current->value );
        path[ depth++ ] = current;
        current = current->parent;
    }

    fprintf(stderr, "\n\nОбъект \"%s\" имеет следующие признаки:\n", leaf->value);
    fprintf(stderr, "──────────────────────────────────────\n");

    for ( size_t idx = depth - 1; idx > 0; idx--) {
        const Node_t* parent = path[ idx ];
        const Node_t* node   = path[ idx - 1 ];

        if (parent->left == node) {
            fprintf(stderr, "✔ %s\n", parent->value);
        } else if (parent->right == node) {
            fprintf(stderr, "✖ не %s\n", parent->value);
        }
    }

    fprintf(stderr, "──────────────────────────────────────\n\n");
}



void Game(Tree_t* tree) {
    my_assert(tree, "Null pointer on `tree`");

    while ( 1 ) {
        print_menu();
        Answer_t* menu_answer = ParseAnswer("1 2");
        if ( !menu_answer ) continue;

        if (menu_answer->type == INT) {
            switch ( menu_answer->value.number ) {
                case 1: {
                    Node_t* current = tree->root;
                    while ( current != 0 && current->left != 0 && current->right != 0 ) {
                        current = AskQuestion( current );
                    }

                    if ( current != 0 ) {
                        fprintf(stderr, COLOR_BRIGHT_GREEN "Я думаю, это %s\n" COLOR_RESET, current->value);
                        PrintObjectTraits(current);

                        fprintf(stderr, "Я угадал? [Y/N]: ");
                        Answer_t* correct_answer = ParseAnswer("Y N y n");
                        if ( !correct_answer ) break;

                        if ( correct_answer->type == STR && ( correct_answer->value.string[0] == 'N' || correct_answer->value.string[0] == 'n' ) ) {

                            char new_object[ MAX_LEN ];
                            char new_question[ MAX_LEN ];

                            fprintf(stderr, "Кто это был? ");
                            scanf(" %127[^\n]", new_object);
                            ClearBuffer();

                            fprintf(stderr, "Чем \"%s\" отличается от \"%s\": он ", current->value, new_object );
                            scanf(" %127[^\n]", new_question);
                            ClearBuffer();

                            fprintf(stderr, "Для \"%s\" ответ на вопрос будет 'Да' или 'Нет'? [Y/N]: ", new_object);
                            Answer_t* ans_for_new_obj = ParseAnswer( "Y N y n" );
                            if ( !ans_for_new_obj ) break;

                            AnswerPosition position = ( ans_for_new_obj->value.string[0] == 'Y' || ans_for_new_obj->value.string[0] == 'y' ) ? YES : NO;
                            free( ans_for_new_obj->value.string );
                            free( ans_for_new_obj );

                            AddQuestion(tree, current, new_question, new_object, position);
                        }

                        if (correct_answer->type == STR) free(correct_answer->value.string);
                        free(correct_answer);
                    } else {
                        fprintf(stderr, COLOR_BRIGHT_RED "Ошибка: дошли до NULL-узла!\n");
                    }
                    break;
                }

                case 2:
                    fprintf(stderr, "Выход.\n");
                    if (menu_answer->type == STR) free(menu_answer->value.string);
                    free(menu_answer);
                    return;

                default:
                    fprintf(stderr, COLOR_BRIGHT_RED "Неверный выбор!\n");
                    break;
            }
        }

        if (menu_answer->type == STR) free(menu_answer->value.string);
        free(menu_answer);
    }
}
