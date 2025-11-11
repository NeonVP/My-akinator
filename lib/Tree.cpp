#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <sys/stat.h>

#include "Tree.h"
#include "DebugUtils.h"

const uint32_t fill_color = 0xb6b4b4;

Tree_t* TreeCtor( const TreeData_t root_value ) {
    Tree_t* new_tree = ( Tree_t* ) calloc ( 1, sizeof( *new_tree ) );
    assert( new_tree && "Mempry allocation error" );

    new_tree->root = ( Node_t* ) calloc ( 1, sizeof( * ( new_tree->root ) ) );
    assert( new_tree->root && "Memory allocation error" );

    new_tree->root->value = strdup( root_value );

    return new_tree;
}

TreeStatus_t TreeDtor( Tree_t **tree ) {
    my_assert( tree, "Null pointer on `tree`" );

    NodeDelete( ( *tree )->root );

    free( *tree );
    *tree = NULL;

    return SUCCESS;
}

Node_t* NodeCreate( const TreeData_t field, Node_t* parent ) {
    Node_t* new_node = ( Node_t* ) calloc ( 1, sizeof( *new_node ) );
    assert( new_node && "Memory err" );

    new_node->value = strdup( field );

    new_node->parent = parent;

    new_node->right = NULL;
    new_node->left  = NULL;

    return new_node;
}

TreeStatus_t NodeDelete( Node_t* node ) {
    my_assert( node, "Null pointer on `node`" );

    if ( node->left != NULL ) {
        NodeDelete( node->left );
    }    

    if ( node->right != NULL ) {
        NodeDelete( node->right );
    }

    if ( node->value ) { 
        free( node->value ); 
        node->value = 0; 
    
    }
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

void TreeDump( const Tree_t* tree ) {
    PRINT_EXECUTING;
    my_assert( tree, "Null pointer on `tree`" );

    mkdir("dump", 0755);
    
    FILE* dot_stream = fopen( "dump/image.dot", "w");
    assert( dot_stream && "Fail open file" );

    fprintf( dot_stream, "digraph {\nsplines=line;\n" );

    NodeDump( tree->root, dot_stream );

    fprintf( dot_stream, "}\n" );

    fclose( dot_stream );

    system( "dot -Tpng dump/image.dot -o dump/image.png" );

    PRINT_STATUS_OK;
}


void NodeDump( const Node_t* node, FILE* dot_stream ) {
    my_assert( node, "Null pointer on `node`" );

    fprintf( 
        dot_stream, 
        "\tnode_%lX [shape=plaintext; style=filled; color=black; fillcolor=\"#%X\"; label=< \n"
        "\t<TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\"> \n"
        "\t\t<TR> \n"
        "\t\t\t<TD PORT=\"idx\" BGCOLOR=\"#%X\">idx=0x%lX</TD> \n"
        "\t\t</TR> \n"
        "\t\t<TR> \n"
        "\t\t\t<TD PORT=\"idx\" BGCOLOR=\"#%X\">parent=0x%lX</TD> \n"
        "\t\t</TR> \n"
        "\t\t<TR> \n"
        "\t\t\t<TD PORT=\"value\" BGCOLOR=\"lightgreen\">%s%s</TD> \n"
        "\t\t</TR> \n"
        "\t\t<TR> \n"
        "\t\t\t<TD> \n"
        "\t\t\t\t<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"2\"> \n"
        "\t\t\t\t\t<TR> \n"
        "\t\t\t\t\t\t<TD PORT=\"left\" BGCOLOR=\"#%X\">%s</TD> \n"
        "\t\t\t\t\t\t<TD><FONT POINT-SIZE=\"10\">│</FONT></TD> \n"
        "\t\t\t\t\t\t<TD PORT=\"right\" BGCOLOR=\"#%X\">%s</TD> \n"
        "\t\t\t\t\t</TR> \n"
        "\t\t\t\t</TABLE> \n"
        "\t\t\t</TD> \n"
        "\t\t</TR> \n"
        "\t</TABLE> \n"
        "\t>]; \n", 
        ( uintptr_t ) node, fill_color,
        crc32_ptr( node ), ( uintptr_t ) node,
        crc32_ptr( node->parent ), ( uintptr_t ) node->parent,
        node->value, ( node->left == 0 && node->right == 0 ) ? ( "" ) : ( "?" ),
        ( node->left == 0 )  ? ( fill_color ) : ( crc32_ptr( node->left ) ),
        ( node->left == 0 )  ? ( "0" ) : ( "Да" ),
        ( node->right == 0 ) ? ( fill_color ) : ( crc32_ptr( node->right ) ),
        ( node->right == 0 ) ? ( "0" ) : ( "Нет" )

    );

    if ( node->left != nullptr ) {
        fprintf(
            dot_stream, 
            "\tnode_%lX:left:s->node_%lX:idx:n\n",
            ( uintptr_t ) node, ( uintptr_t ) node->left
        );
        NodeDump( node->left, dot_stream );
    }

    if ( node->right != nullptr ) {
        fprintf(
            dot_stream, 
            "\tnode_%lX:right:s->node_%lX:idx:n\n",
            ( uintptr_t ) node, ( uintptr_t ) node->right
        );

        NodeDump( node->right, dot_stream );
    }
}