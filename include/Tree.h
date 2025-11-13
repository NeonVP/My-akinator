#ifndef TREE_H
#define TREE_H

#include <stdio.h>

typedef char* TreeData_t;

struct Node_t {
    TreeData_t value;

    Node_t* right;
    Node_t* left;

    Node_t* parent;
};

struct Tree_t {
    Node_t* root;

    char* buffer = 0;
    char* buf_current_position = 0;

    // struct VarInfo_t {
        
    // } var_info;

    // struct Logging_t {
        
    // } logging;
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
TreeStatus_t TreeDtor( Tree_t** tree );

TreeStatus_t TreeCreateNode( Node_t* node, const DirectionType direction,  const TreeData_t new_value, Node_t** new_node_ptr );

Node_t* NodeCreate( const TreeData_t field, Node_t* parent );
TreeStatus_t NodeDelete( Node_t* node );

void TreeDump( const Tree_t* tree );
void NodeDump( const Node_t* node, FILE* dot_stream );

#endif//TREE_H