#pragma codeseg ("LIBCODE")
/*
 * strxfrm.c
 *
 * Ullrich von Bassewitz, 11.12.1998
 */



#include <string.h>



size_t strxfrm (char* dest, const char* src, size_t count)
{
    strncpy (dest, src, count);
    return strlen (src);
}



