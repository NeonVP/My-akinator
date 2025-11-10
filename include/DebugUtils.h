#ifndef ERRORS_H
#define ERRORS_H

#include <stdio.h>
#include <stdlib.h>
#include "Colors.h"

#ifdef _DEBUG
    #define my_assert(arg, message)                                                                       \
        if ( !( arg ) ) {                                                                                     \
            fprintf( stderr, COLOR_BRIGHT_RED "Error in function `%s` %s:%d: %s \n" COLOR_RESET,                 \
                    __func__, __FILE__, __LINE__, message );                                              \
            abort();                                                                                      \
        }

    #define PRINT(str, ...) fprintf( stderr, str COLOR_RESET, ##__VA_ARGS__ );
    
    #define PRINT_EXECUTING   PRINT( GRID COLOR_BRIGHT_YELLOW "<Executing `%s`> --- ", __func__ );
    #define PRINT_STATUS_OK   PRINT(      COLOR_BRIGHT_GREEN  "OK \n" );
    #define PRINT_STATUS_FAIL PRINT(      COLOR_BRIGHT_RED    "FAIL \n" );

    #define ON_DEBUG(...) __VA_ARGS__
#else
    #define my_assert(arg, message) ((void) (arg));

    #define PRINT(str, ...)

    #define PRINT_EXECUTING   
    #define PRINT_STATUS_OK   
    #define PRINT_STATUS_FAIL 

    #define ON_DEBUG(...)
#endif //_DEBUG

#endif //ERRORS_H