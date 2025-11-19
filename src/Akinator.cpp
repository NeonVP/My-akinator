#include <assert.h>
#include <cstddef>
#include <stdarg.h>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#include "Akinator.h"
#include "Colors.h"
#include "DebugUtils.h"
#include "Tree.h"

#ifdef _DEBUG
    #include <errno.h>
#endif

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

static void TreeSaveToFile( const Tree_t* tree, const char* filename );
static void TreeReadFromFile( Akinator_t* akinator );

static void ClearBuffer();
static int HasExtraInput();

ON_DEBUG( static void AkinatorDump( const Akinator_t* akinator, const Node_t* current_element, const char* format_string, ... ); )

static int MakeDirectory( const char* path ) {
    if ( mkdir( path, 0755 ) == -1 ) {
        if ( errno == EEXIST ) {
            return 0;
        } else {
            return -1;
        }
    }
    return 0;
}

Akinator_t* AkinatorCtor() {
    Akinator_t* akinator = ( Akinator_t* ) calloc ( 1, sizeof( *akinator ) );
    assert( akinator && "Memory allocation error" );

    akinator->tree = TreeCtor();

    int mkdir_result = MakeDirectory( "dump" ); 
    assert( !mkdir_result );

    TreeReadFromFile( akinator );

    return akinator;
}

static void TreeCleanFunction( void* stream ) {
    // TODO: add if for check "is it buffer?"

    free( ( char* ) stream );
}

void AkinatorDtor( Akinator_t** akinator ) {
    my_assert( akinator, "Null pointeron `akinator`" );

    TreeDtor( &( ( *akinator )->tree), TreeCleanFunction );

    free( *akinator );
    *akinator = NULL;
}

static Answer_t YesOrNoAnswer() {
    char answer[4] = {};
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
            fprintf( stderr, "Некорректный ответ, попробуйте ещё раз. \n [Y/N]: " );
        }
    }
}

void Game( Akinator_t* akinator ) {
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
            fprintf( stderr, "Некорректный ввод. Повторите ещё раз: " );
        }

        switch ( choice ) {
            case PlayGame:
                PlayRound( akinator->tree );
                break;
            case GiveDefinition:
                PrintObjectTraits( akinator->tree );
                break;
            // case Compare2Definitions:
            //     PrintTwoObjectDifference( tree );
            //     break;
            case QuitSave:
                TreeSaveToFile( akinator->tree, "base.txt" );
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
    fprintf( stderr, "│ 3. Сравнить два объекта (в разработке) │\n" );
    fprintf( stderr, "│ 4. Выход c сохранением базы данных     │\n" );
    fprintf( stderr, "│ 5. Выход без сохранения базы данных    │\n" );
    fprintf( stderr, "│                                        │\n" );
    fprintf( stderr, "│ 0. Выдать базу                         │\n" );
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

static void PrintQuestion(const char* question) {
    my_assert( question, "Null pointer on `question`" );

    fprintf(stderr, COLOR_BRIGHT_YELLOW "[ВОПРОС]\n" COLOR_RESET);
    fprintf(stderr, "%s?\n", question);
    fprintf(stderr, "---------------------------------------------\n");
    fprintf(stderr, "Ответ [Y/N]: ");
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

    return SearchObjectRecursively( tree->root, lower_name );
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

static void ClearBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static off_t DetermineTheFileSize( const char* file_name ) {
    struct stat file_stat;
    int check_stat = stat( file_name, &file_stat );
    assert( check_stat == 0 );

    return file_stat.st_size;
}

static void CleanSpace( char** position ) {
    while ( isspace( **position ) ) 
        ( *position )++;
}

static Node_t* NodeRead(Akinator_t* akinator){
    CleanSpace( &(akinator->current_position ) );

    if (*(akinator->current_position) == '(') {
        akinator->current_position++;

        CleanSpace( &(akinator->current_position ) );

        char* value_ptr = NULL;

        // TODO: scanf...
        if ( *(akinator->current_position) == '\"')
        {
            akinator->current_position++;
            value_ptr = akinator->current_position;

            while (*(akinator->current_position) &&
                   *(akinator->current_position) != '\"')
            {
                akinator->current_position++;
            }

            *(akinator->current_position) = '\0';
            akinator->current_position++;
        }

        Node_t* node = NodeCreate(value_ptr ? value_ptr : ( char* ) "", NULL);

        node->left = NodeRead(akinator);
        if (node->left) node->left->parent = node;

        node->right = NodeRead(akinator);
        if (node->right) node->right->parent = node;

        while (isspace(*(akinator->current_position)))
            akinator->current_position++;

        if (*(akinator->current_position) == ')')
            akinator->current_position++;

        return node;
    }

    if (strncmp(akinator->current_position, "nil", 3) == 0) {
        akinator->current_position += 3;
        return NULL;
    }

    return NULL;
}

static void TreeReadFromFile( Akinator_t* akinator ) {
    my_assert( akinator, "Null pointer on `akinator`" );

    akinator->tree = TreeCtor();

    off_t size = DetermineTheFileSize( "base.txt" );

    FILE* file = fopen( "base.txt",  "r" );
    assert( file && "File opening error" );

    akinator->buffer = ( char* ) calloc ( ( size_t ) ( size + 1 ), sizeof( *( akinator->buffer ) ) );
    assert( akinator->buffer && "Memory allocation error" );

    size_t result_of_read = fread( akinator->buffer, sizeof( char ), ( size_t ) size, file );
    assert( result_of_read != 0 );

    akinator->buffer[ result_of_read ] = '\0';

    int result_of_fclose = fclose( file );
    assert( !result_of_fclose );

    bool error = false;
    akinator->current_position = akinator->buffer;
    akinator->tree->root = NodeRead( akinator );

    if ( error ) {
        fprintf( stderr, "Pizdez, не распарсилось\n" );
    }
    else {
        fprintf( stderr, "Все норм, распарсилось\n" );
    }

    AkinatorDump( akinator, akinator->tree->root, "After full reading the data base" );
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

    if ( *( akinator->current_position ) != '\0' ) {
        PRINT_HTML( "<h2>Текст в буфере с текущей позиции</h2>\n"
                    "<pre style=\"background:#f0f0f0; padding:10px;\">\n" );
        PRINT_HTML( "%s\n", akinator->current_position );
        PRINT_HTML("</pre>\n<hr>\n");
    }

    PRINT_HTML("<h2>Графическое дерево (SVG)</h2>\n"
               "<div style=\"border:1px solid #999; padding:10px; background:white;\">\n");

    // NodeGraphicDump( current_element, "%s/image%lu.dot", akinator->tree->logging.img_log_path, akinator->tree->image_number );

    TreeDump( akinator->tree, "" );

    PRINT_HTML("</div>\n<hr>\n");



    fflush( akinator->tree->logging.log_file );
}
#endif
