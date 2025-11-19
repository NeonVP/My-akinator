#include <errno.h>
#include <assert.h>

#include "UtilsRW.h"

int MakeDirectory( const char* path ) {
    if ( mkdir( path, 0700 ) == -1 ) {
        if ( errno == EEXIST ) {
            return 0;
        } else {
            return -1;
        }
    }
    return 0;
}

off_t DetermineTheFileSize( const char* file_name ) {
    struct stat file_stat;
    int check_stat = stat( file_name, &file_stat );
    assert( check_stat == 0 );

    return file_stat.st_size;
}

