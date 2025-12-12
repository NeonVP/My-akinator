#include <sys/stat.h>

#ifndef UTILSRW_H
#define UTILSRW_H

int MakeDirectory( const char* path );
off_t DetermineTheFileSize( const char* file_name );

#endif