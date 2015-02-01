#ifndef INCLUDE_AMBINARYEDITOR_H
#define INCLUDE_AMBINARYEDITOR_H

#include "AxmlParser.h"
#include "options.h"

#include <stdlib.h>

#endif

enum
{
	ATTR_NULL 	    = 0,
	ATTR_REFERENCE 	= 1,
	ATTR_ATTRIBUTE 	= 2,
	ATTR_STRING 	= 3,
	ATTR_FLOAT 	    = 4,
	ATTR_DIMENSION 	= 5,
	ATTR_FRACTION 	= 6,

	ATTR_FIRSTINT 	= 16,

	ATTR_DEC 	    = 16,
	ATTR_HEX	    = 17,
	ATTR_BOOLEAN	= 18,

	ATTR_FIRSTCOLOR	= 28,
	ATTR_ARGB8 	    = 28,
	ATTR_RGB8	    = 29,
	ATTR_ARGB4	    = 30,
	ATTR_RGB4	    = 31,
	ATTR_LASTCOLOR	= 31,

	ATTR_LASTINT	= 31,
};

typedef struct _BUFF
{
	char *data;
	size_t size;
	size_t cur;
} BUFF;

int HandleAXML(PARSER *ap, OPTIONS *options);
