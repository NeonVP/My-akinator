#include "Akinator.h"

int main() {
    Tree_t* tree = TreeCtor();
    tree->root = NodeCreate( "Животное", NULL );

    tree->root->left = NodeCreate( "Полторашка", tree->root );
    tree->root->right = NodeCreate( "Ведёт матан", tree->root );
    tree->root->right->left = NodeCreate( "Лукашов", tree->root->right );
    tree->root->right->right = NodeCreate( "Паша_Т", tree->root->right );

    TreeDump( tree );

    Game( tree );

    TreeDtor( &tree );
}