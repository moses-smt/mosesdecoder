/*
 * Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*  This file is ALSO:
 *  Copyright 2001-2004 David Abrahams.
 *  Copyright 2005 Rene Rivera.
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * fileunix.c - manipulate file names and scan directories on UNIX/AmigaOS
 *
 * External routines:
 *  file_mkdir()                    - create a directory
 *  file_supported_fmt_resolution() - file modification timestamp resolution
 *
 * External routines called only via routines in filesys.c:
 *  file_collect_dir_content_() - collects directory content information
 *  file_dirscan_()             - OS specific file_dirscan() implementation
 *  file_query_()               - query information about a path from the OS
 */

#include "jam.h"
#ifdef USE_FILEUNIX
#include "filesys.h"

#include "object.h"
#include "pathsys.h"
#include "strings.h"

#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>  /* needed for mkdir() */

#if defined( sun ) || defined( __sun ) || defined( linux )
# include <unistd.h>  /* needed for read and close prototype */
#endif

#if defined( OS_SEQUENT ) || \
    defined( OS_DGUX ) || \
    defined( OS_SCO ) || \
    defined( OS_ISC )
# define PORTAR 1
#endif

#if defined( OS_RHAPSODY ) || defined( OS_MACOSX ) || defined( OS_NEXT )
# include <sys/dir.h>
# include <unistd.h>  /* need unistd for rhapsody's proper lseek */
# define STRUCT_DIRENT struct direct
#else
# include <dirent.h>
# define STRUCT_DIRENT struct dirent
#endif

/*
 * file_collect_dir_content_() - collects directory content information
 */

int file_collect_dir_content_( file_info_t * const d )
{
    LIST * files = L0;
    PATHNAME f;
    DIR * dd;
    STRUCT_DIRENT * dirent;
    string path[ 1 ];
    char const * dirstr;

    assert( d );
    assert( d->is_dir );
    assert( list_empty( d->files ) );

    dirstr = object_str( d->name );

    memset( (char *)&f, '\0', sizeof( f ) );
    f.f_dir.ptr = dirstr;
    f.f_dir.len = strlen( dirstr );

    if ( !*dirstr ) dirstr = ".";

    if ( !( dd = opendir( dirstr ) ) )
        return -1;

    string_new( path );
    while ( ( dirent = readdir( dd ) ) )
    {
        OBJECT * name;
        f.f_base.ptr = dirent->d_name
        #ifdef old_sinix
            - 2  /* Broken structure definition on sinix. */
        #endif
            ;
        f.f_base.len = strlen( f.f_base.ptr );

        string_truncate( path, 0 );
        path_build( &f, path );
        name = object_new( path->value );
        /* Immediately stat the file to preserve invariants. */
        if ( file_query( name ) )
            files = list_push_back( files, name );
        else
            object_free( name );
    }
    string_free( path );

    closedir( dd );

    d->files = files;
    return 0;
}


/*
 * file_dirscan_() - OS specific file_dirscan() implementation
 */

void file_dirscan_( file_info_t * const d, scanback func, void * closure )
{
    assert( d );
    assert( d->is_dir );

    /* Special case / : enter it */
    if ( !strcmp( object_str( d->name ), "/" ) )
        (*func)( closure, d->name, 1 /* stat()'ed */, &d->time );
}


/*
 * file_mkdir() - create a directory
 */

int file_mkdir( char const * const path )
{
#if defined(__MINGW32__)
    /* MinGW's mkdir() takes only one argument: the path. */
    mkdir(path);
#else
    /* Explicit cast to remove const modifiers and avoid related compiler
     * warnings displayed when using the intel compiler.
     */
    return mkdir( (char *)path, 0777 );
#endif
}


/*
 * file_query_() - query information about a path from the OS
 */

void file_query_( file_info_t * const info )
{
    file_query_posix_( info );
}


/*
 * file_supported_fmt_resolution() - file modification timestamp resolution
 *
 * Returns the minimum file modification timestamp resolution supported by this
 * Boost Jam implementation. File modification timestamp changes of less than
 * the returned value might not be recognized.
 *
 * Does not take into consideration any OS or file system related restrictions.
 *
 * Return value 0 indicates that any value supported by the OS is also supported
 * here.
 */

void file_supported_fmt_resolution( timestamp * const t )
{
    /* The current implementation does not support file modification timestamp
     * resolution of less than one second.
     */
    timestamp_init( t, 1, 0 );
}

#endif  /* USE_FILEUNIX */
