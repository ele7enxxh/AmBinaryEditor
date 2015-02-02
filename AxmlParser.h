#ifndef INCLUDE_AXMLPARSER_H
#define INCLUDE_AXMLPARSER_H

#include <stdint.h>
#include <stdlib.h>

#endif

enum{
	CHUNK_HEAD	    = 0x00080003,

	CHUNK_STRING	= 0x001c0001,
	CHUNK_RESOURCE 	= 0x00080180,

	CHUNK_STARTNS	= 0x00100100,
	CHUNK_ENDNS	    = 0x00100101,
	CHUNK_STARTTAG	= 0x00100102,
	CHUNK_ENDTAG	= 0x00100103,
	CHUNK_TEXT	    = 0x00100104,
};

/* attribute structure within tag */
typedef struct _ATTRIBUTE
{
	uint32_t uri;		/* uri of its namespace */
	uint32_t name;
	uint32_t string;	/* attribute value if type == ATTR_STRING */
	uint32_t type;		/* attribute type, == ATTR_ */
	uint32_t data;		/* attribute value, encoded on type */

	struct _ATTRIBUTE *prev;
	struct _ATTRIBUTE *next;
} ATTRIBUTE;

typedef struct _HEADER
{
	uint32_t magic_flag;
	uint32_t size;
}HEADER;

/* StringChunk info */
typedef struct _STRING_CHUNK
{
	uint32_t chunk_type;
	uint32_t chunk_size;
	uint32_t string_count;
	uint32_t style_count;
	uint32_t unknown_field;
	uint32_t string_poll_offset;
	uint32_t style_poll_offset;

	uint32_t *string_offset;
	uint32_t *style_offset;

	unsigned char *string_poll_data;	/* raw data block, contains all strings encoded by UTF-16LE */
	size_t string_poll_len;	/* length of raw data block */

	unsigned char *style_poll_data;
	size_t style_poll_len;

	unsigned char **strings;	/* string table, point to strings encoded by UTF-8 */
} STRING_CHUNK;

typedef struct _RESOURCEID_CHUNK
{
	uint32_t chunk_type;
	uint32_t chunk_size;

	uint32_t *resourceids;
	size_t resourceids_count;
} RESOURCEID_CHUNK;

typedef struct _START_NAMESPACE_CHUNK
{
	uint32_t prefix;
	uint32_t uri;
}START_NAMESPACE_CHUNK;

typedef struct _END_NAMESPACE_CHUNK
{
	uint32_t prefix;
	uint32_t uri;
}END_NAMESPACE_CHUNK;

typedef struct _START_TAG_CHUNK
{
	uint32_t uri;
	uint32_t name;
	uint32_t flag;
	uint32_t attr_count;
	uint32_t attr_class;
	struct _ATTRIBUTE *attr;
} START_TAG_CHUNK;

typedef struct _END_TAG_CHUNK
{
	uint32_t uri;
	uint32_t name;
}END_TAG_CHUNK;

typedef struct _TEXT_CHUNK
{
	uint32_t name;
	uint32_t unknown_field_1;
	uint32_t unknown_field_2;
}TEXT_CHUNK;

typedef struct _XMLCONTENTCHUNK
{
	uint32_t chunk_type;
	uint32_t chunk_size;
	uint32_t line_number;
	uint32_t unknown_field;

	struct _START_NAMESPACE_CHUNK *start_ns_chunk;
	struct _END_NAMESPACE_CHUNK *end_ns_chunk;
	struct _START_TAG_CHUNK *start_tag_chunk;
	struct _END_TAG_CHUNK *end_tag_chunk;
	struct _TEXT_CHUNK *text_chunk;

	struct _XMLCONTENTCHUNK *child;	/* parent->childs link */
    struct _XMLCONTENTCHUNK *parent;	/* child->parent link */
    struct _XMLCONTENTCHUNK *last;
} XMLCONTENTCHUNK;

/* a parser, also a axml parser handle for user */
typedef struct
{
	unsigned char *buf;	/* origin raw data, to be parsed */
	size_t size;		/* size of raw data */
	size_t cur;		/* current parsing position in raw data */

	HEADER *header;
	STRING_CHUNK *string_chunk;
	RESOURCEID_CHUNK *resourceid_chunk;
	XMLCONTENTCHUNK *xmlcontent_chunk;
} PARSER;

int ParserAxml(PARSER *ap, char *in_buf, size_t in_size);
