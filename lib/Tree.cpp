#include <cstdarg>
#include <cstdio>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>

#include "Tree.h"
#include "DebugUtils.h"

const uint32_t fill_color = 0xb6b4b4;


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

Tree_t* TreeCtor() {
    Tree_t* new_tree = ( Tree_t* ) calloc ( 1, sizeof( *new_tree ) );
    assert( new_tree && "Mempry allocation error" );

    new_tree->root = ( Node_t* ) calloc ( 1, sizeof( *( new_tree->root ) ) );
    assert( new_tree->root && "Memory allocation error" );

    #ifdef _DEBUG
        new_tree->image_number = 0;
        new_tree->logging.log_path = strdup( "dump" );

        char buffer[ MAX_LEN_PATH ] = {};

        snprintf( buffer, MAX_LEN_PATH, "%s/images", new_tree->logging.log_path );
        new_tree->logging.img_log_path = strdup( buffer );

        int mkdir_result = MakeDirectory( new_tree->logging.log_path );
        assert( !mkdir_result );
        mkdir_result = MakeDirectory( new_tree->logging.img_log_path );
        assert( !mkdir_result );

        snprintf( buffer, MAX_LEN_PATH, "%s/index.html", new_tree->logging.log_path );
        new_tree->logging.log_file = fopen( buffer, "w" );
        assert( new_tree && "Error opening file" );
    #endif

    return new_tree;
}

TreeStatus_t TreeDtor( Tree_t **tree, void  ( *clean_function ) ( void* value ) ) {
    my_assert( tree, "Null pointer on `tree`" );

    NodeDelete( ( *tree )->root, clean_function );

    free( *tree );
    *tree = NULL;

    return SUCCESS;
}

Node_t* NodeCreate( const TreeData_t field, Node_t* parent ) {
    Node_t* new_node = ( Node_t* ) calloc ( 1, sizeof( *new_node ) );
    assert( new_node && "Memory allocation error" );

    new_node->value  = field;
    new_node->parent = parent;

    return new_node;
}

TreeStatus_t NodeDelete( Node_t* node, void ( *clean_function ) ( void* value ) ) {
    my_assert( node, "Null pointer on `node`" );

    if ( node->left != NULL ) {
        NodeDelete( node->left, clean_function );
    }    

    if ( node->right != NULL ) {
        NodeDelete( node->right, clean_function );
    }

    clean_function( node->value );

    free( node );

    return SUCCESS;
}

static uint32_t crc32_ptr( const void *ptr ) {
    uintptr_t val = ( uintptr_t ) ptr;
    uint32_t  crc = 0xFFFFFFFF;

    for ( size_t i = 0; i < sizeof(val); i++ ) {
        uint8_t byte = ( val >> ( i * 8 ) ) & 0xFF;
        crc ^= byte;
        for ( int j = 0; j < 8; j++ ) {
            if ( crc & 1 )
                crc = ( crc >> 1 ) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return crc ^ 0xFFFFFFFF;
}

void TreeDump( Tree_t* tree, const char* format_string, ... ) {
    my_assert( tree, "Null pointer on `tree`" );

    #define PRINT_HTML( format, ... ) fprintf( tree->logging.log_file, format, ##__VA_ARGS__ );

    PRINT_HTML( "<h3> DUMP" );
    va_list args = {};
    va_start( args, format_string );
    vfprintf( tree->logging.log_file, format_string, args );
    va_end( args );
    PRINT_HTML( "</H3>\n" );

    NodeGraphicDump(
        tree->root, "%s/image%lu.dot", 
        tree->logging.img_log_path, tree->image_number 
    );

    PRINT_HTML( 
        "<img src=\"images/image%lu.dot.svg\" height=\"200px\">\n", 
        tree->image_number++
    );

    #undef PRINT_HTML
}


static void NodeDumpRecursively( const Node_t* node, FILE* dot_stream ) {
    if ( node == NULL ) {
        return;
    }

    #define DOT_PRINT( format, ... ) fprintf( dot_stream, format, ##__VA_ARGS__ );

    #ifdef _DEBUG
        DOT_PRINT( "\tnode_%lX [shape=plaintext; style=filled; color=black; fillcolor=\"#%X\"; label=< \n",
                ( uintptr_t ) node, fill_color );

        DOT_PRINT( "\t<TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\"> \n" );

        DOT_PRINT( "\t\t<TR> \n");
        DOT_PRINT( "\t\t\t<TD PORT=\"idx\" BGCOLOR=\"#%X\">idx=0x%lX</TD> \n",
                crc32_ptr( node ), ( uintptr_t ) node );
        DOT_PRINT( "\t\t</TR> \n" );

        DOT_PRINT( "\t\t<TR> \n" );
        DOT_PRINT( "\t\t\t<TD PORT=\"idx\" BGCOLOR=\"#%X\">parent=0x%lX</TD> \n",
                crc32_ptr( node->parent ), ( uintptr_t ) node->parent );
        DOT_PRINT( "\t\t</TR> \n" );

        DOT_PRINT( "\t\t<TR> \n" );
        DOT_PRINT( "\t\t\t<TD PORT=\"value\" BGCOLOR=\"lightgreen\">%s%s</TD> \n",
                node->value, ( node->left == 0 && node->right == 0 ) ? "" : "?" );
        DOT_PRINT( "\t\t</TR> \n" );

        DOT_PRINT( "\t\t<TR> \n" );
        DOT_PRINT( "\t\t\t<TD> \n" );

        DOT_PRINT( "\t\t\t\t<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"2\"> \n" );
        DOT_PRINT( "\t\t\t\t\t<TR> \n" );

        DOT_PRINT( "\t\t\t\t\t\t<TD PORT=\"left\" BGCOLOR=\"#%X\">%s - %lX</TD> \n",
                ( node->left == 0 ? fill_color : crc32_ptr( node->left ) ),
                ( node->left == 0 ? "0" : "Да" ),
                ( uintptr_t )node->right );

        DOT_PRINT( "\t\t\t\t\t\t<TD><FONT POINT-SIZE=\"10\">│</FONT></TD> \n" );

        DOT_PRINT( "\t\t\t\t\t\t<TD PORT=\"right\" BGCOLOR=\"#%X\">%s - %lX</TD> \n",
                ( node->right == 0 ? fill_color : crc32_ptr( node->right ) ),
                ( node->right == 0 ? "0" : "Нет" ),
                ( uintptr_t )node->left );

        DOT_PRINT( "\t\t\t\t\t</TR> \n" );
        DOT_PRINT( "\t\t\t\t</TABLE> \n" );

        DOT_PRINT( "\t\t\t</TD> \n" );
        DOT_PRINT( "\t\t</TR> \n" );
        DOT_PRINT( "\t</TABLE> \n" );
        DOT_PRINT( "\t>]; \n" );
    #else
        DOT_PRINT( "\tnode_%lX [shape=plaintext; label=<\n", ( uintptr_t ) node );

        DOT_PRINT( "\t<TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\" BGCOLOR=\"#f8f9fa\" COLOR=\"#343a40\">\n" );

        DOT_PRINT( "\t\t<TR>\n" );
        DOT_PRINT("\t\t\t<TD COLSPAN=\"2\" BGCOLOR=\"#4c6ef5\">"
                "<FONT COLOR=\"white\"><B>%s</B></FONT></TD>\n",
                node->value );
        DOT_PRINT("\t\t</TR>\n" );

        DOT_PRINT( "\t\t<TR>\n" );
        DOT_PRINT( "\t\t\t<TD PORT=\"left\"  BGCOLOR=\"#d8f5a2\"><B>Да</B></TD>\n" );
        DOT_PRINT( "\t\t\t<TD PORT=\"right\" BGCOLOR=\"#f5a8a8\"><B>Нет</B></TD>\n" );
        DOT_PRINT( "\t\t</TR>\n" );

        DOT_PRINT( "\t</TABLE>\n" );
        DOT_PRINT( "\t>];\n" );
    #endif

    if ( node->left != NULL ) {
        DOT_PRINT( "\tnode_%lX:left->node_%lX\n",
                  ( uintptr_t ) node, ( uintptr_t ) node->left );
        NodeDumpRecursively( node->left, dot_stream );
    }

    if ( node->right != NULL ) {
        DOT_PRINT( "\tnode_%lX:right->node_%lX\n",
                  ( uintptr_t )node, ( uintptr_t ) node->right );
        NodeDumpRecursively( node->right, dot_stream );
    }

    #undef DOT_PRINT
}

void NodeGraphicDump( const Node_t* node, const char* image_path_name, ... ) {
    if ( !node || !image_path_name )
        return;

    char dot_path[ MAX_LEN_PATH ] = {};
    char svg_path[ MAX_LEN_PATH ] = {};

    va_list args;
    va_start(  args, image_path_name );
    vsnprintf( dot_path, MAX_LEN_PATH, image_path_name, args );
    va_end( args );

    snprintf( svg_path, MAX_LEN_PATH, "%s.svg", dot_path );

    FILE* dot_stream = fopen( dot_path, "w" );
    assert(dot_stream && "File opening error");

    fprintf( dot_stream, "digraph {\n\tsplines=line;\n" );
    NodeDumpRecursively( node, dot_stream );
    fprintf( dot_stream, "}\n" );

    fclose( dot_stream );

    char cmd[ MAX_LEN_PATH * 2 ] = {};
    snprintf( cmd, sizeof(cmd), "dot -Tsvg %s -o %s", dot_path, svg_path );
    system( cmd );
}
