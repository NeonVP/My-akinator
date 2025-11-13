#include <assert.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#include "Akinator.h"
#include "Colors.h"
#include "DebugUtils.h"
#include "Tree.h"

// добавить чтение из файла
// все что сказал (сделать красивую систему ввода и проверки ввода)
// maybe добавить режимы игры: дать определение, сравнить два слова, выход с сохранеием и без, выдать базу

enum GameMode {
    PlayGame            = 1,
    GiveDefinition      = 2,
    Compare2Definitions = 3,
    QuitSave            = 4,
    QuitNotSave         = 5
};

enum Answer_t {
    YES = 1,
    NO  = 0
};

const size_t MAX_LEN = 128;

 
static void     ShowMenu();
static void     PlayRound( Tree_t* tree );
static Node_t*  AskQuestion( Node_t* current );
static void     PrintQuestion( const char* question );
static void     HandleIncorrectGuess( Tree_t* tree, Node_t* leaf ); 
static Answer_t YesOrNoAnswer();

static void    PrintObjectTraits( const Tree_t* tree );
// static void    PrintTwoObjectDifference( const Tree_t* tree );
static Node_t* SearchObject(const Tree_t* tree, const char name_of_object[ MAX_LEN ] );

static Node_t* AddQuestion( Tree_t* tree, Node_t* leaf, const char* new_question, char* new_object, Answer_t answer_for_new_object );

static void    TreeSaveToFile( const Tree_t* tree, const char* filename );
// static Tree_t* TreeReadFromFile();

static void ClearBuffer();
static int HasExtraInput();

static Answer_t YesOrNoAnswer() {
    char answer[ 4 ] = {};
    int result = 0;

    while (1) {
        result = scanf( "%3s", answer );
        ClearBuffer();
        
        if ( result != 1 ) continue;

        if ( strncmp( answer, "Y", 1 ) == 0 ) {
            return YES;
        }
        else if ( strncmp( answer, "N", 1 ) == 0 ) {
            return NO;
        }
        else {
            fprintf( stderr, "Некорректный ответ, попробуйте ещё раз. \n [Y/N]" );
        }
    }
}

void Game( Tree_t* tree ) {
    my_assert( tree, "Null pointer on `tree`" );

    while (1) {
        ShowMenu();

        int choice = 0;
        while ( ( scanf( "%d", &choice ) ) != 1 || !HasExtraInput() ) {
            ClearBuffer();

            fprintf( stderr, "Неправильный ввод. Повторите еще раз. " );
        }
        ClearBuffer();

        switch ( choice ) {
            case PlayGame:
                PlayRound(tree);
                break;
            case GiveDefinition:
                PrintObjectTraits( tree );
                break;
            // case Compare2Definitions:
            //     PrintTwoObjectDifference( tree );

            case QuitSave:
                TreeSaveToFile( tree, "base.txt" );
                return;
            case QuitNotSave:
                fprintf( stderr, "Выход." );
                return;
            default:
                fprintf( stderr, COLOR_BRIGHT_RED "Неверный выбор!\n" COLOR_RESET );
                break;
        }
    }
}

static void ShowMenu() {
    fprintf( stderr, "┌────────────────────────────────────────┐\n" );
    fprintf( stderr, "│             ГЛАВНОЕ МЕНЮ               │\n" );
    fprintf( stderr, "├────────────────────────────────────────┤\n" );
    fprintf( stderr, "│ 1. Начать игру                         │\n" );
    fprintf( stderr, "│ 2. Дать определение объекту            │\n" );
    fprintf( stderr, "│ 3. Сравнить два объекта                │\n" );
    fprintf( stderr, "│ 4. Выход с сохранением базы данных     │\n" );
    fprintf( stderr, "│ 5. Выход без сохранения базы данных    │\n" );
    fprintf( stderr, "│                                        │\n" );
    fprintf( stderr, "│ 0. Загрузить базу данных из файла      │\n" );
    fprintf( stderr, "└────────────────────────────────────────┘\n" );
    fprintf( stderr, "Выберите вариант[1, 2, 3, 4, 5, 0]: ");
}

static void PlayRound( Tree_t* tree ) {
    my_assert( tree, "Null pointer on `tree`" );

    Node_t* current = tree->root;
    while ( current && current->left && current->right ) {
        current = AskQuestion( current );
    }

    if ( !current ) {
        fprintf( stderr, COLOR_BRIGHT_RED "Ошибка: NULL-узел\n" );
        return;
    }

    fprintf( stderr, COLOR_BRIGHT_GREEN "Я думаю, это %s\n" COLOR_RESET, current->value );

    fprintf( stderr, "Я угадал? [Y/N]: " );
    Answer_t answer = YesOrNoAnswer();

    if ( answer == YES ) {
        return;
    }
    else {
        HandleIncorrectGuess( tree, current );
    }
}

static void HandleIncorrectGuess( Tree_t* tree, Node_t* leaf ) {
    my_assert( tree, "Null pointer on `tree`" );
    my_assert( leaf, "Null pointer on `leaf`" );

    fprintf( stderr, "Хотите добавить новый объект? [Y/N] " );
    Answer_t answer = YesOrNoAnswer();
    if ( answer == NO ) {
        return;
    }

    char new_object[ MAX_LEN ]   = {};
    char new_question[ MAX_LEN ] = {};

    fprintf( stderr, "Кто это был? " );
    scanf( " %127[^\n]", new_object );
    ClearBuffer();

    fprintf( stderr, "Чем \"%s\" отличается от \"%s\": он ", leaf->value, new_object );
    scanf( " %127[^\n]", new_question );
    ClearBuffer();

    fprintf( stderr, "Для \"%s\" ответ на вопрос будет 'Да' или 'Нет'? [Y/N]: ", new_object );
    Answer_t ans_for_new_obj = YesOrNoAnswer();

    AddQuestion( tree, leaf, new_question, new_object, ans_for_new_obj );
}


static Node_t* AddQuestion( Tree_t* tree, Node_t* leaf, const char* new_question, char* new_object, Answer_t answer_for_new_object ) {
    my_assert( leaf && tree, "Null pointer on `leaf` or `tree`" );
    my_assert( new_question && new_object, "Null pointer on new data" );

    Node_t* question_node = ( Node_t* ) calloc ( 1, sizeof( *question_node ) );
    assert( question_node && "Memory allocation error" );

    // question_node->value = strdup( new_question );

    question_node->value = ( TreeData_t ) calloc ( strlen( new_question ) + 1, 1 );
    assert( question_node->value && "Memory allocation error" );

    memcpy( question_node->value, new_question, strlen( new_question ) );


    Node_t* object_node = NodeCreate( new_object, question_node );

    if ( answer_for_new_object == YES ) {
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

    return question_node;
}

static Node_t* AskQuestion( Node_t* current ) {
    my_assert( current, "Null pointer on `current`" );

    if ( !current->left && !current->right ) {
        fprintf( stderr, COLOR_BRIGHT_YELLOW "Лист достигнут — здесь нет подветок.\n" );
        return NULL;
    }

    PrintQuestion(current->value);

    Node_t* next = NULL;

    Answer_t answer = YesOrNoAnswer();
    if ( answer == YES ) {
        next = current->left;
    }
    else {
        next = current->right;
    }
    
    return next;
}

static void PrintQuestion( const char* question ) {
    my_assert( question, "Null pointer on `question`" );

    char buffer[42] = {};       // very bad
    snprintf( buffer, sizeof( buffer ), "%.40s?", question );
    fprintf( stderr,"\n┌─ ВОПРОС ──────────────────────────────────┐\n" );
    fprintf( stderr, "│ %-*s │\n", 57 - 8, buffer );
    fprintf( stderr, "└───────────────────────────────────────────┘\n" );
    fprintf( stderr, "Ответ[Y / N]: " );
}

static void PrintObjectTraits( const Tree_t* tree ) {
    my_assert( tree, "Null pointer on `tree`" );

    fprintf( stderr, "Введите имя искомого объекта: " );

    char name_of_object[ MAX_LEN ] = {};

    while ( ( scanf( "%127[^\n]", name_of_object ) ) != 1 ) {
        ClearBuffer();

        fprintf( stderr, "Неправильный ввод. Повторите еще раз. " );
    }

    const Node_t* current = SearchObject( tree, name_of_object );
    
    if ( current == NULL ) {
        fprintf( stderr, "Объекта с именем \"%s\" не существует. \n", name_of_object );
        return;
    }

    const Node_t* path[ MAX_LEN ] = {};
    size_t depth = 0;
    while ( current != NULL ) {
        path[ depth++ ] = current;
        current = current->parent;
    }

    fprintf( stderr, "\n\nОбъект \"%s\" имеет следующие признаки:\n", name_of_object );
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

static Node_t* SearchObjectRecursively( Node_t* node, const char name_of_object[ MAX_LEN ] ) {
    if ( node == NULL ) return NULL;

    char node_name[ MAX_LEN ] = {};
    for ( size_t idx = 0; node->value[ idx ] != '\0'; idx++ )
        node_name[ idx ] = ( char ) tolower( node->value[ idx ] );

    if ( node->left == NULL && node->right == NULL && 
            strncmp( node_name, name_of_object, strlen( name_of_object ) ) == 0 )
        return node;

    Node_t* found = SearchObjectRecursively( node->left, name_of_object );
    if ( found != NULL )
        return found;

    return SearchObjectRecursively( node->right, name_of_object );
}

static Node_t* SearchObject( const Tree_t* tree, const char name_of_object[ MAX_LEN ] ) {
    my_assert( tree,           "Null pointer on `tree`" );
    my_assert( name_of_object, "Null pointer on `name_of_object`" );

    char lower_name[ MAX_LEN ] = "";

    for ( size_t idx = 0; name_of_object[ idx ] != '\0'; idx++ )
        lower_name[ idx ] = ( char ) tolower( name_of_object[ idx ] );

    return SearchObjectRecursively( tree->root, name_of_object );
}

static void WriteNode( const Node_t* node, FILE* stream ) {
    if ( !node ) {
        fprintf( stream, " nil" );
        return;
    }

    fprintf( stream, "( \"%s\" ", node->value ? node->value : "" );

    WriteNode( node->left,  stream );
    WriteNode( node->right, stream );

    fprintf( stream, " )" );
}

static void TreeSaveToFile( const Tree_t* tree, const char* filename ) {
    my_assert( tree,     "Null pointer on tree" );
    my_assert( filename, "Null pointer on filename" );

    FILE* file_with_base = fopen( filename, "w" );
    my_assert( file_with_base, "Failed to open file for writing" );

    WriteNode( tree->root, file_with_base );

    int result = fclose( file_with_base );
    assert( !result && "Error while closing file with base" );
}

// static off_t determining_the_file_size( const char* file_name ) {
//     struct stat file_stat;
//     int check_stat = stat( file_name, &file_stat );
//     assert( check_stat == 0 );

//     return file_stat.st_size;
// }

// static Node_t* NodeRead( char* current_position ) {
//     my_assert( current_position, "Null pointer on `current_position`" );

//     if ( *current_position == '(' ) {
//         current_position++;

        
//         int read_bytes = 0;
//         sscanf( current_position, " \"%*[^\"]%n\"", &read_bytes );
//         *current_position = '\0';


//     }



// }

static void ClearBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static int HasExtraInput() {
    int c;
    int has_extra = 0;

    while ((c = getchar()) != '\n' && c != EOF) {
        if (!isspace(c)) has_extra = 1;
    }

    return has_extra;
}
// static Tree_t* TreeReadFromFile() {
//     Tree_t* tree = TreeCtor();
//     assert( tree && "Memory error allocation" );

//     off_t size = determining_the_file_size( "base.txt" );

//     FILE* file = fopen( "base.txt",  "r" );
//     assert( file && "File opening error" );

//     char* buffer = ( char* ) calloc ( ( size_t ) size, sizeof( *buffer ) );
//     size_t result_of_read = fread( buffer, sizeof( char ), ( size_t ) size, file );
//     assert( result_of_read   != 0 );

//     int result_of_fclose = fclose( file );
//     assert( !result_of_fclose );

//     char* current_position = NULL;
//     tree->root = NodeRead( current_position );

//     return tree;
// }