#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include "Akinator.h"
#include "Colors.h"
#include "DebugUtils.h"
#include "Tree.h"
#include "UtilsRW.h"


enum GameMode {
    PlayGame            = 1,
    GiveDefinition      = 2,
    Compare2Definitions = 3,
    QuitSave            = 4,
    QuitNotSave         = 5,
    ShowTree            = 0
};

enum Answer_t {
    YES = 1,
    NO  = 0
};

const size_t MAX_LEN = 256;

 
static void     ShowMenu();
static void     PlayRound( Tree_t* tree );
static Node_t*  AskQuestion( Node_t* current );
static void     PrintQuestion( const char* question );
static void     HandleIncorrectGuess( Tree_t* tree, Node_t* leaf ); 
static Node_t*  AddQuestion( Tree_t* tree, Node_t* leaf, const char* new_question, char* new_object, Answer_t answer_for_new_object );
static Answer_t YesOrNoAnswer();

static void    PrintObjectTraits( const Tree_t* tree );
static Node_t* SearchObject(const Tree_t* tree, const char* name_of_object );

static void PrintTwoObjectDifference(const Tree_t* tree);
static size_t BuildPath(const Node_t* root, const Node_t* target, const Node_t* path[], size_t depth);

static void ShowGraphicTree( Tree_t* tree );

static void ClearBuffer();

ON_DEBUG( static void AkinatorDump( const Akinator_t* akinator, const Node_t* current_element, 
                                                                const char* format_string, ... ); )

static void Speak( const char* text );

Akinator_t* AkinatorCtor() {
    Akinator_t* akinator = ( Akinator_t* ) calloc ( 1, sizeof( *akinator ) );
    assert( akinator && "Memory allocation error" );

    akinator->tree = TreeCtor();

    int mkdir_result = MakeDirectory( "dump" );
    assert( !mkdir_result );

    akinator->base_path = strdup( "base.txt" );

    TreeReadFromFile( akinator->tree );
    AkinatorDump( akinator, akinator->tree->root, "After full reading the data base" );

    return akinator;
}

static void TreeCleanFunction( char* stream, Tree_t* tree ) {
    if ( !( tree->buffer <= stream && stream < tree->buffer + tree->buffer_size ) ) {
        free( stream );
    }
}

void AkinatorDtor( Akinator_t** akinator ) {
    my_assert( akinator, "Null pointer on `akinator`" );

    TreeDtor( &( ( *akinator )->tree), TreeCleanFunction );

    free( ( *akinator )->base_path );

    free( *akinator );
    *akinator = NULL;
}

void AkinatorGame( Akinator_t* akinator ) {
    my_assert( akinator, "Null pointer on `akinator`" );

    while (1) {
        ShowMenu();

        int  choice = 0;
        char tail   = 0;

        while (1) {
            if ( scanf( "%d", &choice ) == 1 ) {
                if ( scanf( "%c", &tail ) == 1 && tail == '\n' ) {
                    break;
                }
            }
            ClearBuffer();
            fprintf( stdout, "Некорректный ввод. Повторите ещё раз: " );
        }

        switch ( choice ) {
            case PlayGame:
                PlayRound( akinator->tree );
                break;
            case GiveDefinition:
                PrintObjectTraits( akinator->tree );
                break;
            case Compare2Definitions:
                PrintTwoObjectDifference( akinator->tree );
                break;
            case QuitSave:
                TreeSaveToFile( akinator->tree, akinator->base_path );
                fprintf( stdout, "Выход." );
                return;
            case QuitNotSave:
                fprintf( stdout, "Выход." );
                return;
            case ShowTree:
                ShowGraphicTree( akinator->tree );
                break;
            default:
                fprintf( stdout, COLOR_BRIGHT_RED "Неверный выбор!\n" COLOR_RESET );
                break;
        }
    }
}

static void ShowMenu() {
    fprintf( stdout, "┌────────────────────────────────────────┐\n" );
    fprintf( stdout, "│             ГЛАВНОЕ МЕНЮ               │\n" );
    fprintf( stdout, "├────────────────────────────────────────┤\n" );
    fprintf( stdout, "│ 1. Начать игру                         │\n" );
    fprintf( stdout, "│ 2. Дать определение объекту            │\n" );
    fprintf( stdout, "│ 3. Сравнить два объекта (в разработке) │\n" );
    fprintf( stdout, "│ 4. Выход c сохранением базы данных     │\n" );
    fprintf( stdout, "│ 5. Выход без сохранения базы данных    │\n" );
    fprintf( stdout, "│                                        │\n" );
    fprintf( stdout, "│ 0. Выдать базу                         │\n" );
    fprintf( stdout, "└────────────────────────────────────────┘\n" );
    fprintf( stdout, "Выберите вариант[1, 2, 3, 4, 5, 0]: ");
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

    char buffer[ MAX_LEN ] = {};
    snprintf( buffer, MAX_LEN, "Я думаю, это %s", current->value );
    fprintf( stdout, COLOR_BRIGHT_GREEN "%s\n" COLOR_RESET, buffer );
    Speak( buffer );

    snprintf( buffer, MAX_LEN, "Я угадал?" );
    fprintf( stdout, "%s [Y/N]: ", buffer );
    Speak( buffer );
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

    char buffer[ MAX_LEN * 3 ] = {};

    snprintf( buffer, MAX_LEN, "Хотите добавить новый объект?" );
    fprintf( stdout, "%s [Y/N] ", buffer );
    Speak( buffer );
    Answer_t answer = YesOrNoAnswer();
    if ( answer == NO ) {
        return;
    }

    char new_object[ MAX_LEN ]   = {};
    char new_question[ MAX_LEN ] = {};

    fprintf( stdout, "Кто это был? " );
    Speak( "Кто это был?" );
    scanf( " %127[^\n]", new_object );
    ClearBuffer();

    snprintf( buffer, MAX_LEN * 3, "Чем \"%s\" отличается от \"%s\"", leaf->value, new_object );
    fprintf( stdout, "%s: он ", buffer );
    Speak( buffer );
    scanf( " %127[^\n]", new_question );
    ClearBuffer();

    snprintf( buffer, MAX_LEN * 3, "Для \"%s\" ответ на вопрос будет 'Да' или 'Нет'?", new_object );
    fprintf( stdout, "%s [Y/N]: ", buffer );
    Speak( buffer );
    Answer_t ans_for_new_obj = YesOrNoAnswer();

    AddQuestion( tree, leaf, new_question, new_object, ans_for_new_obj );
}

static Answer_t YesOrNoAnswer() {
    char answer[4] = {};
    int result = 0;

    while (1) {
        result = scanf( "%3s", answer );
        ClearBuffer();
        
        if ( result != 1 ) continue;

        if ( strncmp( answer, "Y", 1 ) == 0 || strncmp( answer, "y", 1 ) == 0  ) {
            return YES;
        }
        else if ( strncmp( answer, "N", 1 ) == 0 || strncmp( answer, "n", 1 ) == 0 ) {
            return NO;
        }
        else {
            fprintf( stdout, "Некорректный ответ, попробуйте ещё раз. \n [Y/N]: " );
        }
    }
}

static Node_t* AddQuestion( Tree_t* tree, Node_t* leaf, const char* new_question, char* new_object, Answer_t answer_for_new_object ) {
    my_assert( leaf && tree, "Null pointer on `leaf` or `tree`" );
    my_assert( new_question && new_object, "Null pointer on new data" );

    Node_t* question_node = ( Node_t* ) calloc ( 1, sizeof( *question_node ) );
    assert( question_node && "Memory allocation error" );

    question_node->value = strdup( new_question );

    // question_node->value = ( TreeData_t ) calloc ( strlen( new_question ) + 1, 1 );
    // assert( question_node->value && "Memory allocation error" );
    // memcpy( question_node->value, new_question, strlen( new_question ) );

    question_node->value[0] = ( char ) toupper( question_node->value[0] );

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

    fprintf(stdout, COLOR_BRIGHT_YELLOW "[ВОПРОС]\n" COLOR_RESET);
    fprintf(stdout, "%s?\n", question);
    fprintf(stdout, "---------------------------------------------\n");
    fprintf(stdout, "Ответ [Y/N]: ");

    Speak( question );
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

    fprintf( stdout, "\n\nОбъект \"%s\" имеет следующие признаки:\n", name_of_object );
    fprintf( stdout, "──────────────────────────────────────\n" );

    for ( size_t idx = depth - 1; idx > 0; idx-- ) {
        const Node_t* parent = path[ idx ];
        const Node_t* node   = path[ idx - 1 ];

        if ( parent->left == node) {
            fprintf( stdout, "✔ %s\n", parent->value );
            
        } else if ( parent->right == node ) {
            fprintf( stdout, "✖ не %s\n", parent->value );
        }
    }

    fprintf( stderr, "──────────────────────────────────────\n\n" );
}

static Node_t* SearchObjectRecursively( Node_t* node, const char* name_of_object, size_t length ) {
    if ( node == NULL ) return NULL;

    if ( node->left == NULL && node->right == NULL && 
            strncasecmp( node->value, name_of_object, length ) == 0 )
        return node;

    Node_t* found = SearchObjectRecursively( node->left, name_of_object, length );
    if ( found != NULL )
        return found;

    return SearchObjectRecursively( node->right, name_of_object, length );
}

static Node_t* SearchObject( const Tree_t* tree, const char* name_of_object ) {
    my_assert( tree,           "Null pointer on `tree`" );
    my_assert( name_of_object, "Null pointer on `name_of_object`" );

    char lower_name[ MAX_LEN ] = "";

    for ( size_t idx = 0; name_of_object[ idx ] != '\0'; idx++ )
        lower_name[ idx ] = ( char ) tolower( name_of_object[ idx ] );

    return SearchObjectRecursively(
        tree->root, 
        lower_name, strlen( lower_name ) 
    );
}



static size_t BuildPath(const Node_t* root, const Node_t* target, const Node_t* path[], size_t depth) {
    if (!root) return 0;

    path[depth] = root;

    if (root == target)
        return depth + 1;

    size_t left  = BuildPath(root->left,  target, path, depth + 1);
    if (left) return left;

    size_t right = BuildPath(root->right, target, path, depth + 1);
    if (right) return right;

    return 0;
}

// static void PrintTwoObjectDifference( const Tree_t* tree ) {
//     my_assert(tree, "Null pointer on tree");

//     char obj1[ MAX_LEN ] = {};
//     char obj2[ MAX_LEN ] = {};

//     fprintf( stdout, "Введите имя первого объекта: " );
//     scanf( " %127[^\n]", obj1 );
//     ClearBuffer();

//     fprintf( stdout, "Введите имя второго объекта: " );
//     scanf( " %127[^\n]", obj2 );
//     ClearBuffer();

//     const Node_t* n1 = SearchObject( tree, obj1 );
//     const Node_t* n2 = SearchObject( tree, obj2 );

//     if ( !n1 || !n2 ) {
//         fprintf( stderr, COLOR_BRIGHT_RED "Одного из объектов нет в базе.\n" COLOR_RESET );
//         return;
//     }

//     const Node_t* path1[ MAX_LEN ] = {};
//     const Node_t* path2[ MAX_LEN ] = {};

//     size_t len1 = BuildPath( tree->root, n1, path1, 0 );
//     size_t len2 = BuildPath( tree->root, n2, path2, 0 );

//     size_t diverge = 0;
//     while ( diverge < len1 && diverge < len2 && path1[ diverge ] == path2[ diverge ] ) diverge++;

//     fprintf( stdout, "\n────────────────────────────────────────────\n" );
//     fprintf( stdout, COLOR_BRIGHT_GREEN "Сравнение \"%s\" и \"%s\":\n" COLOR_RESET, obj1, obj2 );
//     fprintf( stdout, "────────────────────────────────────────────\n" );

//     // Общие признаки
//     fprintf( stdout, COLOR_BRIGHT_YELLOW "\nОбщие признаки:\n" COLOR_RESET );
//     for ( size_t idx = 1; idx < diverge; idx++ ) {
//         const Node_t* parent = path1[ idx - 1 ];
//         const Node_t* node   = path1[ idx ];

//         if ( parent->left == node )
//             fprintf( stdout, "✔ %s\n", parent->value );
//         else
//             fprintf( stdout, "✖ не %s\n", parent->value );
//     }

//     // Отличия
//     fprintf( stdout, COLOR_BRIGHT_RED "\nОтличия:\n" COLOR_RESET );

//     size_t start_idx = ( diverge == 0 ? 0 : diverge - 1 );

//     fprintf( stderr, "\n%s:\n", obj1 );
//     for ( size_t idx = start_idx; idx < len1 - 1; idx++ ) {
//         const Node_t* parent = path1[ idx ];
//         const Node_t* node   = path1[ idx + 1 ];

//         if ( parent->left == node )
//             fprintf( stdout, "✔ %s\n", parent->value );
//         else
//             fprintf( stdout, "✖ не %s\n", parent->value );
//     }

//     fprintf( stdout, "\n%s:\n", obj2 );
//     for ( size_t idx = start_idx; idx < len2 - 1; idx++ ) {
//         const Node_t* parent = path2[ idx ];
//         const Node_t* node   = path2[ idx + 1 ];

//         if ( parent->left == node )
//             fprintf( stdout, "✔ %s\n", parent->value );
//         else
//             fprintf( stdout, "✖ не %s\n", parent->value );
//     }

//     fprintf( stdout, "────────────────────────────────────────────\n\n" );
// }

static int ReadTwoObjects( char* obj1, char* obj2 ) {
    fprintf(stdout, "Введите имя первого объекта: ");
    if ( scanf(" %127[^\n]", obj1) != 1 ) return 0;
    ClearBuffer();

    fprintf(stdout, "Введите имя второго объекта: ");
    if ( scanf(" %127[^\n]", obj2) != 1 ) return 0;
    ClearBuffer();

    return 1;
}

static int FindTwoNodes( const Tree_t* tree, const char* obj1, const char* obj2, const Node_t** n1, const Node_t** n2 ) {
    *n1 = SearchObject( tree, obj1 );
    *n2 = SearchObject( tree, obj2 );

    if ( !*n1 || !*n2 ) {
        fprintf( stderr, COLOR_BRIGHT_RED "Одного из объектов нет в базе.\n" COLOR_RESET );
        return 0;
    }
    return 1;
}

static void PrintCommonTraits( const Node_t** path, size_t diverge ) {
    fprintf( stdout, COLOR_BRIGHT_YELLOW "\nОбщие признаки:\n" COLOR_RESET );

    for ( size_t idx = 1; idx < diverge; idx++ ) {
        const Node_t* parent = path[ idx - 1 ];
        const Node_t* node   = path[ idx ];

        if ( parent->left == node )
            fprintf( stdout, "✔ %s\n", parent->value );
        else
            fprintf( stdout, "✖ не %s\n", parent->value );
    }
}

static void PrintDifferences( const char* obj, const Node_t** path, size_t len, size_t diverge )
{
    fprintf( stdout, "\n%s:\n", obj );

    size_t start = ( diverge == 0 ? 0 : diverge - 1 );

    for ( size_t idx = start; idx < len - 1; idx++ ) {
        const Node_t* parent = path[ idx ];
        const Node_t* node   = path[ idx + 1 ];

        if ( parent->left == node )
            fprintf( stdout, "✔ %s\n", parent->value );
        else
            fprintf( stdout, "✖ не %s\n", parent->value );
    }
}

static void PrintTwoObjectDifference( const Tree_t* tree ) {
    my_assert(tree, "Null pointer on tree");

    char obj1[ MAX_LEN ] = {};
    char obj2[ MAX_LEN ] = {};

    if ( !ReadTwoObjects(obj1, obj2) ) return;

    const Node_t* n1 = NULL;
    const Node_t* n2 = NULL;

    if ( !FindTwoNodes( tree, obj1, obj2, &n1, &n2 ) ) return;

    const Node_t* path1[ MAX_LEN ] = {};
    const Node_t* path2[ MAX_LEN ] = {};

    size_t len1 = BuildPath( tree->root, n1, path1, 0 );
    size_t len2 = BuildPath( tree->root, n2, path2, 0 );

    size_t diverge = 0;
    while (diverge < len1 && diverge < len2 && path1[diverge] == path2[diverge])
        diverge++;

    fprintf( stdout, "\n────────────────────────────────────────────\n" );
    fprintf( stdout, COLOR_BRIGHT_GREEN "Сравнение \"%s\" и \"%s\":\n" COLOR_RESET, obj1, obj2 );
    fprintf( stdout, "────────────────────────────────────────────\n" );

    PrintCommonTraits( path1, diverge);

    fprintf( stdout, COLOR_BRIGHT_RED "\nОтличия:\n" COLOR_RESET );

    PrintDifferences( obj1, path1, len1, diverge );
    PrintDifferences( obj2, path2, len2, diverge );

    fprintf( stdout, "────────────────────────────────────────────\n\n" );
}


static void ClearBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}


#ifdef _DEBUG
static void AkinatorDump( const Akinator_t* akinator, const Node_t* current_element, const char* format_string, ... ) {
    my_assert( akinator, "Null pointer on `akinator`" );

    if ( !current_element ) {
        current_element = akinator->tree->root;
    }

    #define PRINT_HTML( format, ... ) fprintf( akinator->tree->logging.log_file, format, ##__VA_ARGS__ );

    PRINT_HTML( "<h1>")

    va_list args = {};
    va_start( args, format_string );
    vfprintf( akinator->tree->logging.log_file, format_string, args );
    va_end( args );

    PRINT_HTML( "</h1>\n" );

    if ( *( akinator->tree->current_position ) != '\0' ) {
        PRINT_HTML( "<h2>Текст в буфере с текущей позиции</h2>\n"
                    "<pre style=\"background:#f0f0f0; padding:10px;\">\n" );
        PRINT_HTML( "%s\n", akinator->tree->current_position );
        PRINT_HTML("</pre>\n<hr>\n");
    }

    PRINT_HTML("<h2>Графическое дерево</h2>\n"
               "<div style=\"border:1px solid #999; padding:10px; background:white;\">\n");

    TreeDump( akinator->tree, "" );

    PRINT_HTML("</div>\n<hr>\n");



    fflush( akinator->tree->logging.log_file );
}
#endif

static void Speak( const char* text ) {
    if ( !text || text[0] == '\0' ) return;

    char cmd[ MAX_LEN ] = {};
    snprintf( cmd, sizeof(cmd), "espeak -v ru -s 100 \"%s\" 2>/dev/null", text );

    system( cmd );
}

static void OpenImage(const char* path) {
    if ( !path ) return;

    char cmd[ MAX_LEN ] = {};
    snprintf( cmd, sizeof(cmd), "xdg-open \"%s\" >/dev/null 2>&1 &", path );
    system( cmd );
}

static void ShowGraphicTree( Tree_t* tree ) {
    my_assert( tree, "Null pointer on `tree`" );

    fprintf( stdout, "Генерация графического дерева...\n" );

    TreeDump( tree, "" );

    char svg_path[256] = {};
    snprintf(
        svg_path,
        sizeof(svg_path),
        "%s/image%lu.dot.svg",
        tree->logging.img_log_path,
        tree->image_number - 1
    );

    fprintf( stdout, "Готово! SVG сохранён: %s\n", svg_path );
    fprintf( stdout, "Открываю изображение...\n" );

    OpenImage(svg_path);
}
