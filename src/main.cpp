#include "Akinator.h"

int main() {
    Akinator_t* akinator = AkinatorCtor();

    Game( akinator );

    AkinatorDtor( &akinator );
}