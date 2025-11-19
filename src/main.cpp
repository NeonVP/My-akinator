#include "Akinator.h"

int main() {
    Akinator_t* akinator = AkinatorCtor();

    AkinatorGame( akinator );

    AkinatorDtor( &akinator );
}