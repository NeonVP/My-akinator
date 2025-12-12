#include <stdio.h>

#ifndef TREE_H
#define TREE_H

#ifdef _LINUX
#include <linux/limits.h>
const size_t MAX_LEN_PATH = PATH_MAX;
#else
const size_t MAX_LEN_PATH = 256;
#endif

typedef char* TreeData_t;

struct Node_t {
    TreeData_t value;

    Node_t* right;
    Node_t* left;

    Node_t* parent;
};

struct Tree_t {
    Node_t* root;

    char* buffer;
    char* current_position;
    off_t buffer_size;

    #ifdef _DEBUG
        struct Log_t {
            FILE* log_file;
            char* log_path;
            char* img_log_path;
        } logging;

        size_t image_number;
    #endif
};

enum TreeStatus_t {
    SUCCESS = 0,
    FAIL    = 1
};

enum DirectionType {
    RIGHT = 0,
    LEFT  = 1
};

Tree_t*      TreeCtor();
TreeStatus_t TreeDtor( Tree_t** tree, void ( *clean_function ) ( char* value, Tree_t* tree ) );

void TreeSaveToFile( const Tree_t* tree, const char* filename );
void TreeReadFromFile( Tree_t* tree );

Node_t* NodeCreate( const TreeData_t field, Node_t* parent );
TreeStatus_t NodeDelete( Node_t* node, Tree_t* tree, void ( *clean_function ) ( char* value, Tree_t* tree ) );

void TreeDump( Tree_t* tree, const char* format_string, ... );
void NodeGraphicDump( const Node_t* node, const char* image_path_name, ... );

#endif//TREE_H