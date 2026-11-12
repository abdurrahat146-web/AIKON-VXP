/* Minimal freestanding stub: the MRE SDK headers and gccmain.c include
   <stdio.h> only for the FILE type name (used in one fiprintf prototype)
   and gccmain.c's _calloc_r uses struct _reent by pointer only, never
   dereferenced. No real newlib exists for this bare-metal ARM target, so
   we supply just enough of the shape to type-check; nothing here is ever
   called. */
#ifndef _SHIM_STDIO_H
#define _SHIM_STDIO_H
#include <stddef.h> /* size_t, from clang's bundled freestanding header */
typedef struct __FILE FILE;
struct _reent;
#endif
