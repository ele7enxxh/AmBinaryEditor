#include "AxmlParser.h"

#include <stdio.h>
#include <string.h>

/* get a 4-byte integer, and mark as parsed */
/* uses byte oprations to avoid little or big-endian conflict */
static uint32_t GetInt32(PARSER *ap)
{
	uint32_t value = 0;
	unsigned char *p = ap->buf + ap->cur;
	value = p[0] | p[1]<<8 | p[2]<<16 | p[3]<<24;
	ap->cur += 4;
	return value;
}

/* skip some uknown of useless fields, don't parse them */
/*static void SkipInt32(PARSER *ap, size_t num)
{
	ap->cur += 4 * num;
	return;
}*/

/* if no more byte need to be parsed */
static int NoMoreData(PARSER *ap)
{
	return ap->cur >= ap->size;
}

static void CopyData(PARSER *ap, unsigned char * to, size_t size)
{
	memcpy(to, ap->buf + ap->cur, size);
	ap->cur += size;
	return;
}

/** \brief Convert UTF-16LE string into UTF-8 string
 *
 *  You must call this function with to=NULL firstly to get UTF-8 size;
 *  then you should alloc enough memory to the string;
 *  at last call this function again to convert actually.
 *  \param to Pointer to target UTF-8 string
 *  \param from Pointer to source UTF-16LE string
 *  \param nch Count of UTF-16LE characters, including terminal zero
 *  \retval -1 Converting error.
 *  \retval positive Bytes of UTF-8 string, including terminal zero.
 */
static size_t UTF16LEtoUTF8(unsigned char *to, unsigned char *from, size_t nch)
{
	size_t total = 0;
    uint32_t ucs4;
    size_t count;
	while(nch > 0)
	{
		/* utf-16le -> ucs-4, defined in RFC 2781 */
		ucs4 = from[0] + (from[1]<<8);
		from += 2;
		nch--;
		if (ucs4 < 0xd800 || ucs4 > 0xdfff)
		{
			;
		}
		else if (ucs4 >= 0xd800 && ucs4 <= 0xdbff)
		{
			unsigned int ext;
			if (nch <= 0)
			{
				return -1;
			}
			ext = from[0] + (from[1]<<8);
			from += 2;
			nch--;
			if (ext < 0xdc00 || ext >0xdfff)
			{
				return -1;
			}
			ucs4 = ((ucs4 & 0x3ff)<<10) + (ext & 0x3ff) + 0x10000;
		}
		else
		{
			return -1;
		}

		/* ucs-4 -> utf-8, defined in RFC 2279 */
		if (ucs4 < 0x80)
		{
			count = 1;
		}
		else if (ucs4 < 0x800)
		{
			count = 2;
		}
		else if (ucs4 < 0x10000)
		{
			count = 3;
		}
		else if (ucs4 < 0x200000)
		{
			count = 4;
		}
		else if (ucs4 < 0x4000000)
		{
			count = 5;
		}
		else if (ucs4 < 0x80000000)
		{
			count = 6;
		}
		else
		{
			return 0;
		}

		total += count;
		if (to == NULL)
		{
			continue;
		}

		switch(count)
		{
			case 6: to[5] = 0x80 | (ucs4 & 0x3f); ucs4 >>= 6; ucs4 |= 0x4000000;
			case 5:	to[4] = 0x80 | (ucs4 & 0x3f); ucs4 >>= 6; ucs4 |= 0x200000;
			case 4: to[3] = 0x80 | (ucs4 & 0x3f); ucs4 >>= 6; ucs4 |= 0x10000;
			case 3: to[2] = 0x80 | (ucs4 & 0x3f); ucs4 >>= 6; ucs4 |= 0x800;
			case 2: to[1] = 0x80 | (ucs4 & 0x3f); ucs4 >>= 6; ucs4 |= 0xc0;
			case 1: to[0] = ucs4; break;
		}
		to += count;
	}
	if (to != NULL)
	{
		to[0] = '\0';
	}
	return total + 1;
}

static int SetStringTable(PARSER *ap)
{
	size_t i;
	unsigned char *offset;
	uint16_t character_num;
	size_t size;

	ap->string_chunk->strings = (unsigned char **)malloc(ap->string_chunk->string_count * sizeof(unsigned char *));

	for (i = 0; i < ap->string_chunk->string_count; i++)
	{
		/* point to string's poll data */
		offset = ap->string_chunk->string_poll_data + ap->string_chunk->string_offset[i];

		/* its first 2 bytes is string's characters count */
		character_num = *(uint16_t *)offset;

		size = UTF16LEtoUTF8(NULL, offset + 2, (size_t)character_num);

		ap->string_chunk->strings[i] = (unsigned char *)malloc(size);
		if (size == (size_t)-1)
		{
			((unsigned char *)(ap->string_chunk->strings[i]))[0] = '\0';
			// printf("%ld %s\n", i, ap->st->strings[i]);
		}
		UTF16LEtoUTF8(ap->string_chunk->strings[i], offset + 2, (size_t)character_num);
		// printf("%ld %s\n", i, ap->string_chunk->strings[i]);
	}

	return 0;
}

static int ParseHeadChunk(PARSER *ap)
{
	/* file magic */
	ap->header->magic_flag = GetInt32(ap);
	if (ap->header->magic_flag != CHUNK_HEAD)
	{
		fprintf(stderr, "Error: not valid AXML file.\n");
		return -1;
	}

	/* file size */
	ap->header->size = GetInt32(ap);
	if (ap->header->size != ap->size)
	{
		fprintf(stderr, "Error: not complete file.\n");
		return -1;
	}

	return 0;
}

static int ParseStringChunk(PARSER *ap)
{
	size_t i;

	/* chunk type */
	ap->string_chunk->chunk_type = GetInt32(ap);
	if (ap->string_chunk->chunk_type != CHUNK_STRING)
	{
		fprintf(stderr, "Error: not valid string chunk.\n");
		return -1;
	}

	/* chunk size */
	ap->string_chunk->chunk_size = GetInt32(ap);

	/* count of strings */
	ap->string_chunk->string_count = GetInt32(ap);

	/* count of styles */
	ap->string_chunk->style_count = GetInt32(ap);

	/* unknown field */
	ap->string_chunk->unknown_field = GetInt32(ap);

	/* offset of string poll data in chunk */
	ap->string_chunk->string_poll_offset = GetInt32(ap);

	/* offset of style poll data in chunk */
	ap->string_chunk->style_poll_offset = GetInt32(ap);

	/* strings' offsets table */
	ap->string_chunk->string_offset = (uint32_t *)malloc(ap->string_chunk->string_count * sizeof(uint32_t));
	for (i = 0; i < ap->string_chunk->string_count; i++)
	{
		ap->string_chunk->string_offset[i] = GetInt32(ap);
	}

	/* styles' offsets table */
	if (ap->string_chunk->style_count != 0)
	{
        ap->string_chunk->style_offset = (uint32_t *)malloc(ap->string_chunk->style_count * sizeof(uint32_t));
        for (i = 0; i < ap->string_chunk->style_count; i++)
        {
            ap->string_chunk->style_offset[i] = GetInt32(ap);
        }
		//SkipInt32(ap, ap->string_chunk->style_count);
	}

	/* save string poll data */
	ap->string_chunk->string_poll_len = (ap->string_chunk->style_poll_offset ? ap->string_chunk->style_poll_offset : ap->string_chunk->chunk_size) - ap->string_chunk->string_poll_offset;
	ap->string_chunk->string_poll_data = (unsigned char *)malloc(ap->string_chunk->string_poll_len);
	CopyData(ap, ap->string_chunk->string_poll_data, ap->string_chunk->string_poll_len);
	if (SetStringTable(ap) != 0)
	{
		fprintf(stderr, "Error: setstringtable return failed.\n");
		free(ap->string_chunk->string_offset);
		ap->string_chunk->string_offset = NULL;
		free(ap->string_chunk->string_poll_data);
		ap->string_chunk->string_poll_data = NULL;

		free(ap->string_chunk->string_offset);
		free(ap->string_chunk->string_poll_data);
		return -1;
	}

	/*save style poll data */
	if (ap->string_chunk->style_poll_offset != 0 && ap->string_chunk->style_count != 0)
	{
        ap->string_chunk->style_poll_len = ap->string_chunk->chunk_size - ap->string_chunk->style_poll_offset;
        ap->string_chunk->style_poll_data = (unsigned char *)malloc(ap->string_chunk->style_poll_len);
        CopyData(ap, ap->string_chunk->style_poll_data, ap->string_chunk->style_poll_len);
		//SkipInt32(ap, (ap->string_chunk->chunk_size - ap->string_chunk->style_poll_offset) / 4);
	}

	return 0;
}

static int ParseResourceChunk(PARSER *ap)
{
    size_t i;
	/* chunk type */
	ap->resourceid_chunk->chunk_type = GetInt32(ap);
	if (ap->resourceid_chunk->chunk_type != CHUNK_RESOURCE)
	{
		fprintf(stderr, "Error: not valid resource chunk.\n");
		return -1;
	}

	ap->resourceid_chunk->chunk_size = GetInt32(ap);
	if (ap->resourceid_chunk->chunk_size % sizeof(uint32_t) != 0)
	{
		fprintf(stderr, "Error: not valid resource chunk.\n");
		return -1;
	}

	ap->resourceid_chunk->resourceids_count =  ap->resourceid_chunk->chunk_size / 4 - 2;
	ap->resourceid_chunk->resourceids = (uint32_t *)malloc(ap->resourceid_chunk->resourceids_count * sizeof(uint32_t));
	for (i = 0; i < ap->resourceid_chunk->resourceids_count; i++)
	{
        ap->resourceid_chunk->resourceids[i] = GetInt32(ap);
	}

	return 0;
}

static void AddElement(PARSER *ap, XMLCONTENTCHUNK *content)
{
	if (ap->xmlcontent_chunk == NULL)
	{
		ap->xmlcontent_chunk = content;
		ap->xmlcontent_chunk->child = NULL;
	}

	else
	{
		//content->parent = *target_parent;
		//(*target_parent)->child = content;
		//content->child = NULL;
		content->parent = ap->xmlcontent_chunk->last;
		ap->xmlcontent_chunk->last->child = content;
		content->child = NULL;
	}

	//*target_parent = content;
	ap->xmlcontent_chunk->last = content;
}

static int ParserXmlContentChunk(PARSER *ap)
{
	int ret = 0;
	XMLCONTENTCHUNK *content = NULL;
	//XMLCONTENTCHUNK target_parent;
	//XMLCONTENTCHUNK **target_parent = (XMLCONTENTCHUNK **)malloc(4);

	while(1)
	{
        ATTRIBUTE *bro = NULL;
        uint32_t attr_count;
        ATTRIBUTE *attr;
		/* when buffer ends */
		if (NoMoreData(ap))
		{
			ret = 0;
			break;
		}
		content = (XMLCONTENTCHUNK *)malloc(sizeof(XMLCONTENTCHUNK));
		content->chunk_type = GetInt32(ap);
		content->chunk_size = GetInt32(ap);
		content->line_number = GetInt32(ap);
		content->unknown_field = GetInt32(ap);

		if (content->chunk_type == CHUNK_STARTNS)
		{
			content->start_ns_chunk = (START_NAMESPACE_CHUNK *)malloc(sizeof(START_NAMESPACE_CHUNK));
			content->start_ns_chunk->prefix = GetInt32(ap);
			content->start_ns_chunk->uri = GetInt32(ap);
		}
		else if (content->chunk_type == CHUNK_ENDNS)
		{
			content->end_ns_chunk = (END_NAMESPACE_CHUNK *)malloc(sizeof(END_NAMESPACE_CHUNK));
			content->end_ns_chunk->prefix = GetInt32(ap);
			content->end_ns_chunk->uri = GetInt32(ap);
		}
		else if (content->chunk_type == CHUNK_STARTTAG)
		{
			content->start_tag_chunk = (START_TAG_CHUNK *)malloc(sizeof(START_TAG_CHUNK));
			memset(content->start_tag_chunk, 0, sizeof(START_TAG_CHUNK));
			content->start_tag_chunk->uri = GetInt32(ap);
			content->start_tag_chunk->name = GetInt32(ap);
			content->start_tag_chunk->flag = GetInt32(ap);
			content->start_tag_chunk->attr_count = GetInt32(ap);
			content->start_tag_chunk->attr_class = GetInt32(ap);
			attr_count = content->start_tag_chunk->attr_count;
			while (attr_count != 0)
			{
				attr = (ATTRIBUTE *)malloc(sizeof(ATTRIBUTE));
				attr->uri = GetInt32(ap);
				attr->name = GetInt32(ap);
				attr->string = GetInt32(ap);
				attr->type = GetInt32(ap);
				attr->data = GetInt32(ap);

				if (content->start_tag_chunk->attr == NULL)
				{
					content->start_tag_chunk->attr = attr;
					content->start_tag_chunk->attr->next = NULL;
				}
				else
				{
					attr->prev = bro;
					bro->next = attr;
					attr->next = NULL;
				}
				bro = attr;
				attr_count--;
			}
		}
		else if (content->chunk_type == CHUNK_ENDTAG)
		{
			content->end_tag_chunk = (END_TAG_CHUNK *)malloc(sizeof(END_TAG_CHUNK));
			content->end_tag_chunk->uri = GetInt32(ap);
			content->end_tag_chunk->name = GetInt32(ap);
		}
		else if (content->chunk_type == CHUNK_TEXT)
		{
			content->text_chunk = (TEXT_CHUNK *)malloc(sizeof(TEXT_CHUNK));
			content->text_chunk->name = GetInt32(ap);
			content->text_chunk->unknown_field_1 = GetInt32(ap);
			content->text_chunk->unknown_field_2 = GetInt32(ap);
		}

		AddElement(ap, content);
	}

	return ret;
}

int ParserAxml(PARSER *ap, char *in_buf, size_t in_size)
{
	ap->buf = (unsigned char *)in_buf;
	ap->size = in_size;
	ap->cur = 0;

	ap->header = (HEADER *)malloc(sizeof(HEADER));
	memset(ap->header, 0, sizeof(HEADER));

	ap->string_chunk = (STRING_CHUNK *)malloc(sizeof(STRING_CHUNK));
	memset(ap->string_chunk, 0, sizeof(STRING_CHUNK));

	ap->resourceid_chunk = (RESOURCEID_CHUNK *)malloc(sizeof(RESOURCEID_CHUNK));
	memset(ap->resourceid_chunk, 0, sizeof(RESOURCEID_CHUNK));

	if ( ParseHeadChunk(ap) != 0 || ParseStringChunk(ap) != 0 || ParseResourceChunk(ap) != 0 || ParserXmlContentChunk(ap) != 0)
	{
		return -1;
	}

	return 0;
}
