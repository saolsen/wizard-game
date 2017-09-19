#ifndef _wizard_h
#define _wizard_h

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

#ifndef __clang__
#include <malloc.h>
#endif

#include <stdio.h> // @TODO: drop this, it's for printf

#define LEN(e) (sizeof(e)/sizeof(e[0]))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#endif //_wizard_h