#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "AmBinaryEditor.h"

#include <stdio.h>
#include <string.h>

static int CopyUint32(BUFF *buf, uint32_t value)
{
    char p[4];
	while (4 >= buf->size - buf->cur)
	{
		buf->size += 32*1024;
		buf->data = (char *)realloc(buf->data, buf->size);
		memset(buf->data + buf->cur, 0, buf->size - buf->cur);
		if (buf->data == NULL)
		{
			fprintf(stderr, "Error: realloc buffer.\n");
			return -1;
		}
	}

	p[0] = value & 0xff ;
	p[1] = (value >>8) &0xff;
	p[2] = (value >> 16) &0xff;
	p[3] = (value >> 24) &0xff;
	memcpy(buf->data + buf->cur, p, 4);
	buf->cur += 4;

	return 0;
}

static int CopyDataToBuf(BUFF *buf, unsigned char *from, size_t size)
{
	while (size >= buf->size - buf->cur)
	{
		buf->size += 32*1024;
		buf->data = (char *)realloc(buf->data, buf->size);
		memset(buf->data + buf->cur, 0, buf->size - buf->cur);
		if (buf->data == NULL)
		{
			fprintf(stderr, "Error: realloc buffer.\n");
			return -1;
		}
	}
	memcpy(buf->data + buf->cur, from, size);
	buf->cur += size;
	return 0;
}

static size_t UTFtoUTF16LE(unsigned char *to, const unsigned char *from, size_t nch)
{
	size_t total = 0;
    uint32_t c;
    uint32_t ucs4;
    size_t extra;
    size_t i;
    size_t count;
	while(nch > 0)
	{
		c = *from++;
		nch--;
	    if (c < 0x80)
	    {
	        ucs4 = c;
	        extra = 1;
	    }

	    else if (c < 0xC0 || c > 0xFD)
	    {   fprintf(stderr, "ERROR: valid UTF-8 strings\n");
	        return -1;
	    }

	    else if (c < 0xE0)
	    {
	        ucs4 = c & 0x1F;
	        extra = 2;
	    }
	    else if (c < 0xF0)
	    {
	        ucs4 = c & 0x0F;
	        extra = 3;
	    }
	    else if (c < 0xF8)
	    {
	        ucs4 = c & 7;
	        extra = 4;
	    }
	    else if (c < 0xFC)
	    {
	        ucs4 = c & 3;
	        extra = 5;
	    }
	    else
	    {
	        ucs4 = c & 1;
	        extra = 6;
	    }

	    for (i = 1; i < extra; i++)
	    {
	        c = *from++;
	        nch--;
	        if (c < 0x80 || c > 0xBF)
	        {
	        	fprintf(stderr, "ERROR: valid UTF-8 strings\n");
	            return -1;
	        }

	        ucs4 = (ucs4 << 6) + (c & 0x3F);
	    }

	    if (ucs4 <= 0xffff)
	    {
	    	count = 2;
	    }
	    else if (ucs4 <= 0xeffff)
	    {
	    	count = 4;
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
			case 4:
			to[3] = ucs4 >> 0x18;
			to[2] = ucs4 >> 0x10;
			case 2:
			to[1] = ucs4 >> 8;
			to[0] = ucs4;
			break;
			default:
			fprintf(stderr, "ERROR: valid count\n");
			return -1;
		}
		to += count;
	}
	if (to != NULL)
	{
		to[0] = '\0';
		to[1] = '\0';
	}

	return (total + 4 + 3) & (~3);
}

static int InitBuff(BUFF *buf)
{
	if (buf == NULL)
	{
		return -1;
	}

	buf->size = 32*1024;
	buf->data = (char *)malloc(buf->size);
	memset(buf->data, 0, buf->size);
	buf->cur = 0;
	return 0;
}

static int RebuildAXML(PARSER *ap, BUFF *buf)
{
    size_t i;
    XMLCONTENTCHUNK *xc;

	CopyUint32(buf, ap->header->magic_flag);
	CopyUint32(buf, ap->header->size);

	CopyUint32(buf, ap->string_chunk->chunk_type);
	CopyUint32(buf, ap->string_chunk->chunk_size);
	CopyUint32(buf, ap->string_chunk->string_count);
	CopyUint32(buf, ap->string_chunk->style_count);
	CopyUint32(buf, ap->string_chunk->unknown_field);
	CopyUint32(buf, ap->string_chunk->string_poll_offset);
	CopyUint32(buf, ap->string_chunk->style_poll_offset);
	for (i = 0; i < ap->string_chunk->string_count; i++)
	{
		CopyUint32(buf, ap->string_chunk->string_offset[i]);
	}
	for (i = 0; i < ap->string_chunk->style_count; i++)
	{
		CopyUint32(buf, ap->string_chunk->style_offset[i]);
	}
	CopyDataToBuf(buf, ap->string_chunk->string_poll_data, ap->string_chunk->string_poll_len);
	CopyDataToBuf(buf, ap->string_chunk->style_poll_data, ap->string_chunk->style_poll_len);

	CopyUint32(buf, ap->resourceid_chunk->chunk_type);
	CopyUint32(buf, ap->resourceid_chunk->chunk_size);
	for (i = 0; i < ap->resourceid_chunk->resourceids_count; i++)
	{
        CopyUint32(buf, ap->resourceid_chunk->resourceids[i]);
	}

	xc = ap->xmlcontent_chunk;
	while (xc != NULL)
	{
        ATTRIBUTE *attr;
		CopyUint32(buf, xc->chunk_type);
		CopyUint32(buf, xc->chunk_size);
		CopyUint32(buf, xc->line_number);
		CopyUint32(buf, xc->unknown_field);
		if (xc->chunk_type == CHUNK_STARTNS)
		{
			CopyUint32(buf, xc->start_ns_chunk->prefix);
			CopyUint32(buf, xc->start_ns_chunk->uri);
		}
		else if (xc->chunk_type == CHUNK_ENDNS)
		{
			CopyUint32(buf, xc->end_ns_chunk->prefix);
			CopyUint32(buf, xc->end_ns_chunk->uri);
		}
		else if (xc->chunk_type == CHUNK_STARTTAG)
		{
			CopyUint32(buf, xc->start_tag_chunk->uri);
			CopyUint32(buf, xc->start_tag_chunk->name);
			CopyUint32(buf, xc->start_tag_chunk->flag);
			CopyUint32(buf, xc->start_tag_chunk->attr_count);
			CopyUint32(buf, xc->start_tag_chunk->attr_class);
			attr = xc->start_tag_chunk->attr;
			while (attr != NULL)
			{
				CopyUint32(buf, attr->uri);
				CopyUint32(buf, attr->name);
				CopyUint32(buf, attr->string);
				CopyUint32(buf, attr->type);
				CopyUint32(buf, attr->data);
				attr = attr->next;
			}
		}
		else if (xc->chunk_type == CHUNK_ENDTAG)
		{
			CopyUint32(buf, xc->end_tag_chunk->uri);
			CopyUint32(buf, xc->end_tag_chunk->name);
		}
		else if (xc->chunk_type == CHUNK_TEXT)
		{
			CopyUint32(buf, xc->text_chunk->name);
			CopyUint32(buf, xc->text_chunk->unknown_field_1);
			CopyUint32(buf, xc->text_chunk->unknown_field_2);
		}

		xc = xc->child;
	}
	return 0;
}

static uint32_t GetStringIndex(STRING_CHUNK *string_chunk, const char *str, int flag, int32_t *extra_size)
{
	STRING_CHUNK *sc = string_chunk;
	size_t utf16le_size;
	unsigned char *out = NULL;
	size_t i;

	for (i = 0; i < sc->string_count; i++)
	{
		if (strcmp(str, (const char *)sc->strings[i]) == 0)
		{
			return i;
		}
	}

	if (!flag)
	{
        fprintf(stderr, "ERROR: str: '%s'.\n", str);
        return -1;
	}

	sc->string_count += 1;

	utf16le_size = UTFtoUTF16LE(NULL, (const unsigned char *)str, strlen(str));
	out = (unsigned char *)malloc(utf16le_size);
	((uint16_t *)out)[0] = strlen(str);
	UTFtoUTF16LE(out + 2, (const unsigned char *)str, strlen(str));

	sc->string_offset = (uint32_t *)realloc(sc->string_offset, sizeof(uint32_t) * sc->string_count);
	if (sc->string_offset == NULL)
	{
		fprintf(stderr, "ERROR: string_offset realloc failed\n");
		return -1;
	}

	sc->string_offset[sc->string_count - 1] = sc->string_poll_len;
	*extra_size += sizeof(uint32_t);

	sc->string_poll_offset += sizeof(uint32_t);

	sc->string_poll_len += utf16le_size;
	sc->string_poll_data = (unsigned char *)realloc(sc->string_poll_data, sc->string_poll_len);
	if (sc->string_poll_data == NULL)
	{
		fprintf(stderr, "ERROR: string_poll_data realloc failed\n");
		return -1;
	}

	memcpy(sc->string_poll_data + sc->string_poll_len - utf16le_size, out, utf16le_size);
	*extra_size += utf16le_size;

	sc->strings = (unsigned char **)realloc(sc->strings, sc->string_count * sizeof(unsigned char *));
	if (sc->strings == NULL)
	{
		fprintf(stderr, "ERROR: strings realloc failed\n");
		return -1;
	}

	sc->strings[sc->string_count - 1] = (unsigned char *)malloc(strlen(str) + 1);
	strcpy((char *)sc->strings[sc->string_count - 1], str);

	sc->chunk_size += (sizeof(uint32_t) + utf16le_size);

	free(out);

	return sc->string_count - 1;
}

static int HandleResourceChunk(RESOURCEID_CHUNK * resourceid_chunk, uint32_t offset, uint32_t resource_id, int32_t *extra_size)
{
    if (offset + 1 <= resourceid_chunk->resourceids_count)
    {
        resourceid_chunk->resourceids[offset] = resource_id;
    }
    else
    {
        *extra_size += (offset + 1 - resourceid_chunk->resourceids_count) *sizeof(uint32_t);

        resourceid_chunk->resourceids = (uint32_t *)realloc(resourceid_chunk->resourceids, (offset + 1) * sizeof(uint32_t));
        if (resourceid_chunk->resourceids == NULL)
        {
            fprintf(stderr, "ERROR: resourceids realloc failed\n");
            return -1;
        }
        resourceid_chunk->resourceids[offset] = resource_id;
        resourceid_chunk->resourceids_count = offset + 1;
        resourceid_chunk->chunk_size = (2 + resourceid_chunk->resourceids_count) * sizeof(uint32_t);
    }

    return 0;
}

static XMLCONTENTCHUNK *FindTagStartChunk(PARSER *ap, char *name, uint32_t deep)
{
	STRING_CHUNK *sc = ap->string_chunk;
	XMLCONTENTCHUNK *root = ap->xmlcontent_chunk;
	XMLCONTENTCHUNK *node = root;
	XMLCONTENTCHUNK *target = NULL;
	while (node)
	{
        if (node->chunk_type == CHUNK_STARTTAG)
        {
            if (strcmp((const char *)sc->strings[node->start_tag_chunk->name], name) == 0)
            {
                deep--;
                if (deep == 0)
                {
                    target = node;
                    break;
                }
            }
        }
        node = node->child;
	}

	if (deep != 0 || target == NULL)
	{
        fprintf(stderr, "ERROR: tag_name: '%s' does not exist.\n", name);
        return NULL;
	}

	return target;
}

static int InitAttribute(PARSER *ap, ATTRIBUTE *attr, const char *name, uint32_t type, char *value,
    uint32_t resource_id, int flag, int32_t *extra_size)
{
	//static float RadixTable[] = {0.00390625f, 3.051758E-005f, 1.192093E-007f, 4.656613E-010f};
	//static char *DimemsionTable[] = {"px", "dip", "sp", "pt", "in", "mm", "", ""};
	//static char *FractionTable[] = {"%", "%p", "", "", "", "", "", ""};

    float prefix_data;
	char suffix_data[32];
	float f;

	STRING_CHUNK *sc = ap->string_chunk;

	attr->uri = ap->xmlcontent_chunk->start_ns_chunk->uri;
	attr->type = type << 24;
	attr->name = GetStringIndex(sc, name, flag, extra_size);

	if (attr -> name == -1)
	{
        return -1;
	}
	attr->string = 0xffffffff;

	if (type == ATTR_STRING)
	{
		attr->string = GetStringIndex(sc, value, 1, extra_size);
		attr->data = GetStringIndex(sc, value, 1, extra_size);
	}

	else if (type == ATTR_REFERENCE)
	{
		if (memcmp(value, "@android:", 9) == 0)
		{
			attr->data = atoi(value + 9) << 24;
		}
		else if (memcmp(value, "@", 1) == 0)
		{
			attr->data = atoi(value + 1);
		}
		else
		{
			fprintf(stderr, "value: '%s' is valid\n", value);
			return -1;
		}
	}
	else if (type == ATTR_ATTRIBUTE)
	{
        if (memcmp(value, "?android:", 9) == 0)
        {
            attr->data = atoi(value + 9) << 24;
        }
        else if (memcmp(value, "?", 1) == 0)
        {
            attr->data = atoi(value + 1);
        }
        else
        {
			fprintf(stderr, "value: '%s' is valid\n", value);
			return -1;
        }
	}
	else if (type == ATTR_FLOAT)
	{
        f = atof(value);
        attr->data = ((uint32_t *)&f)[0];
	}
	else if (type == ATTR_DIMENSION)
	{
        if(sscanf(value, "%f%s", &prefix_data, suffix_data) != 2)
        {
            fprintf(stderr, "value: '%s' is valid\n", value);
            return -1;
        }
        if (suffix_data == NULL || strlen(suffix_data) <= 0)
        {
            attr->data |= (uint8_t)6;
        }
        else if (strcmp(suffix_data, "px") == 0)
        {
            attr->data |= (uint8_t)0;
        }
        else if (strcmp(suffix_data, "dip") == 0)
        {
            attr->data |= (uint8_t)1;
        }
        else if (strcmp(suffix_data, "sp") == 0)
        {
            attr->data |= (uint8_t)2;
        }
        else if (strcmp(suffix_data, "pt") == 0)
        {
            attr->data |= (uint8_t)3;
        }
        else if (strcmp(suffix_data, "in") == 0)
        {
            attr->data |= (uint8_t)4;
        }
        else if (strcmp(suffix_data, "mm") == 0)
        {
            attr->data |= (uint8_t)5;
        }
        else
        {
            fprintf(stderr, "ERROR: value: '%s' is valid\n", value);
            return -1;
        }

        fprintf(stderr, "ERROR: Sorry, I don't know how to convert '%s'\n", value);
        return -1;
	}
	else if (type == ATTR_FRACTION)
	{
        fprintf(stderr, "ERROR: Sorry, I don't know how to convert '%s'\n", value);
        return -1;
	}
	else if (type == ATTR_HEX)
	{
        if (sscanf(value, "%x", &attr->data) != 1)
        {
            fprintf(stderr, "value: '%s' is valid\n", value);
            return -1;
        }
	}
	else if (type == ATTR_BOOLEAN)
	{
        if (strcmp(value, "true") == 0)
        {
            attr->data = 1;
        }
        else if (strcmp(value, "false") == 0)
        {
            attr->data = 0;
        }
        else
        {
            fprintf(stderr, "value: '%s' is valid\n", value);
            return -1;
        }
	}
	else if (type >= ATTR_FIRSTCOLOR && type <= ATTR_LASTCOLOR)
	{
        if (memcmp(value, "#", 1) == 0)
        {
            attr->data = atoi(value + 1);
        }
        else
        {
            fprintf(stderr, "value: '%s' is valid\n", value);
            return -1;
        }
	}
	else if (type >= ATTR_FIRSTINT && type <= ATTR_LASTINT)
	{
        attr->data = atoi(value);
	}
	else
	{
        fprintf(stderr, "ERROR: Illegal attr_type: '%d'.\n", type);
        return -1;
	}

	if (resource_id != 0)
	{
        return HandleResourceChunk(ap->resourceid_chunk, attr->name, resource_id, extra_size);
	}

	return 0;
}

static int AddAttribute(PARSER *ap, char *tag_name, uint32_t deep, uint32_t attr_type, const char *attr_name,
    char *attr_value, uint32_t resource_id, int32_t *extra_size)
{
    ATTRIBUTE *attr = NULL;
    XMLCONTENTCHUNK *target = NULL;
    ATTRIBUTE *list = NULL;
    if (tag_name == NULL || deep < 0 || attr_name == NULL || attr_value == NULL || strlen(attr_name) <= 0)
    {
        fprintf(stderr, "ERROR: Illegal parameters.\n");
        return -1;
    }

    attr = (ATTRIBUTE *)malloc(sizeof(ATTRIBUTE));
    memset(attr, 0, sizeof(ATTRIBUTE));
    if (InitAttribute(ap, attr, attr_name, attr_type, attr_value, resource_id, 1, extra_size) == -1)
    {
        free(attr);
        return -1;
    }

    target = FindTagStartChunk(ap, tag_name, deep);
    if (target == NULL)
    {
        return -1;
    }
    list = target->start_tag_chunk->attr;
    if (list == NULL)
    {
        list = attr;
    }
    else
    {
        while(1)
        {
            if (list->next == NULL)
            {
                break;
            }
            list = list->next;
        }
        list->next = attr;
        attr->prev = list;
    }
    target->chunk_size += 5 * sizeof(uint32_t);
    target->start_tag_chunk->attr_count += 1;

    *extra_size += 5 * sizeof(uint32_t);

    return 0;
}

static int ModifyAttribute(PARSER *ap, char *tag_name, uint32_t deep, uint32_t attr_type, const char *attr_name,
    char *attr_value, uint32_t resource_id, int32_t *extra_size)
{
    ATTRIBUTE *attr = NULL;
    XMLCONTENTCHUNK *target = NULL;
    ATTRIBUTE *list = NULL;
    STRING_CHUNK *sc = ap->string_chunk;
    if (tag_name == NULL || deep < 0 || attr_name == NULL || attr_value == NULL || strlen(attr_name) <= 0)
    {
        fprintf(stderr, "ERROR: Illegal parameters.\n");
        return -1;
    }

    attr = (ATTRIBUTE *)malloc(sizeof(ATTRIBUTE));
    memset(attr, 0, sizeof(ATTRIBUTE));
    if (InitAttribute(ap, attr, attr_name, attr_type, attr_value, resource_id, 0, extra_size) == -1)
    {
        free(attr);
        return -1;
    }

    target = FindTagStartChunk(ap, tag_name, deep);
    if (target == NULL)
    {
        return -1;
    }
    list = target->start_tag_chunk->attr;
    if (list == NULL)
    {
        fprintf(stderr, "ERROR: tag_name: '%s' attr is NULL.\n", tag_name);
        return -1;
    }
    while(list)
    {
        if (strcmp((const char *)sc->strings[list->name], attr_name) == 0)
        {
            list->string = attr->string ? attr->string : list->string;
            list->type = attr->type ? attr->type : list->type;
            list->data = attr->data ? attr->data : list->data;
        }
        list = list->next;
    }

    free(attr);

    return 0;
}

static int RemoveAttribute(PARSER *ap, char *tag_name, uint32_t deep, uint32_t attr_type, const char *attr_name,
    char *attr_value, uint32_t resource_id, int32_t *extra_size)
{
    XMLCONTENTCHUNK *target = NULL;
    STRING_CHUNK *sc = ap->string_chunk;
    ATTRIBUTE *list = NULL;
    if (tag_name == NULL || deep < 0 || attr_name == NULL || strlen(attr_name) <= 0)
    {
        fprintf(stderr, "ERROR: Illegal parameters.\n");
        return -1;
    }

    target = FindTagStartChunk(ap, tag_name, deep);
    if (target == NULL)
    {
        return -1;
    }
    list = target->start_tag_chunk->attr;
    while (list)
    {
        if (strcmp((const char *)sc->strings[list->name], attr_name) == 0)
        {
            if (list->prev == NULL)
            {
                target->start_tag_chunk->attr = list->next;
                target->start_tag_chunk->attr->prev = NULL;
            }
            else if (list->next == NULL)
            {
                list->prev->next = NULL;
            }
            else
            {
                list->prev->next = list->next;
                list->next->prev = list->prev;
            }
            free(list);
            target->start_tag_chunk->attr_count -= 1;
            target->chunk_size -= 5 * sizeof(uint32_t);
            break;
        }
        list = list->next;
    }

    *extra_size -= 5 * sizeof(uint32_t);

    return 0;
}

static int HandleAttribute(PARSER *ap, OPTIONS *options, int32_t *extra_size)
{
    switch (options->mode)
    {
    case MODE_ADD:
        return AddAttribute(ap, options->tag_name, options->deep, options->attr_type, options->attr_name,
        options->attr_value, options->resource_id, extra_size);
    case MODE_MODIFY:
        return ModifyAttribute(ap, options->tag_name, options->deep, options->attr_type, options->attr_name,
        options->attr_value, options->resource_id, extra_size);
    case MODE_REMOVE:
        return RemoveAttribute(ap, options->tag_name, options->deep, options->attr_type, options->attr_name,
        options->attr_value, options->resource_id, extra_size);
    default:
        return -1;
    }
}

static int AddTagChunk(PARSER *ap, char *tag_name, uint32_t deep, uint32_t count, int32_t *extra_size)
{
    STRING_CHUNK *sc = ap->string_chunk;
	XMLCONTENTCHUNK *root = ap->xmlcontent_chunk;
	XMLCONTENTCHUNK *node = root;
	XMLCONTENTCHUNK *parent = NULL;

	XMLCONTENTCHUNK *target_start = NULL;
	XMLCONTENTCHUNK *target_end = NULL;

    if (tag_name == NULL || deep <= 0)
    {
        fprintf(stderr, "ERROR: Illegal parameters.\n");
        return -1;
    }

	while (node && deep)
	{
        if (node->chunk_type == CHUNK_STARTTAG)
        {
            parent = node;
            deep--;
        }
        node = node->child;
	}
	if (deep != 0 || parent == NULL)
	{
        fprintf(stderr, "deep is valid.\n");
        return -1;
	}

	target_start = (XMLCONTENTCHUNK *)malloc(sizeof(XMLCONTENTCHUNK));
	target_start->chunk_type = CHUNK_STARTTAG;
	target_start->chunk_size = 9 * sizeof(uint32_t);
	target_start->line_number = 0;
	target_start->unknown_field = 0xffffffff;
	target_start->start_tag_chunk = (START_TAG_CHUNK *)malloc(sizeof(START_TAG_CHUNK));
	target_start->start_tag_chunk->uri = 0xffffffff;
	target_start->start_tag_chunk->name = GetStringIndex(sc, tag_name, 1, extra_size);
    target_start->start_tag_chunk->flag = 0x00140014;
    target_start->start_tag_chunk->attr_count = 0;
    target_start->start_tag_chunk->attr_class = 0;
    target_start->start_tag_chunk->attr = NULL;

	target_start->child = parent->child;
	parent->child->parent = target_start;
	parent->child = target_start;
	target_start->parent = parent;

	target_end = (XMLCONTENTCHUNK *)malloc(sizeof(XMLCONTENTCHUNK));
	target_end->chunk_type = CHUNK_ENDTAG;
	target_end->chunk_size = 6 * sizeof(uint32_t);
	target_end->line_number = 0;
	target_end->unknown_field = 0xffffffff;
	target_end->end_tag_chunk = (END_TAG_CHUNK *)malloc(sizeof(END_TAG_CHUNK));
	target_end->end_tag_chunk->uri = 0xffffffff;
	target_end->end_tag_chunk->name = GetStringIndex(sc, tag_name, 1, extra_size);

	parent = target_start;
	while (count--)
	{
        parent = parent->child;
        if (parent->chunk_type == CHUNK_ENDNS || parent == NULL)
        {
            fprintf(stderr, "ERROR: count is valid.\n");
            return -1;
        }
	}
	target_end->child = parent->child;
	parent->child->parent = target_end;
	parent->child = target_end;
	target_end->parent = parent;

	*extra_size += (target_start->chunk_size + target_end->chunk_size);

	return 0;
}

static int ModifyTagChunk(PARSER *ap, char *tag_name, uint32_t deep, const char *new_tag_name, int32_t *extra_size)
{
    XMLCONTENTCHUNK *target_start = NULL;
    uint32_t ori_name;
    STRING_CHUNK *sc = ap->string_chunk;
    XMLCONTENTCHUNK *node = NULL;
    XMLCONTENTCHUNK *target_end = NULL;
    int degree = 0;
    if (tag_name == NULL || deep < 0 || new_tag_name == NULL)
    {
        fprintf(stderr, "ERROR: Illegal parameters.\n");
        return -1;
    }

	target_start = FindTagStartChunk(ap, tag_name, deep);
	if (target_start == NULL)
	{
        return -1;
	}

	ori_name = target_start->start_tag_chunk->name;
	target_start->start_tag_chunk->name = GetStringIndex(ap->string_chunk, new_tag_name, 1, extra_size);

	node = target_start;
	while (node)
	{
        if (node->chunk_type == CHUNK_STARTTAG)
        {
            degree++;
        }
        if (node->chunk_type == CHUNK_ENDTAG)
        {
            degree--;
            if (strcmp((const char *)sc->strings[node->end_tag_chunk->name], "manifest") == 0)
            {
                ;
            }
            else if (node->end_tag_chunk->name == ori_name && degree == 0)
            {
                target_end = node;
                break;
            }
        }
        node = node->child;
	}

	target_end->end_tag_chunk->name = GetStringIndex(ap->string_chunk, new_tag_name, 1, extra_size);

    return 0;
}

static int RemoveTagChunk(PARSER *ap, char *tag_name, uint32_t deep, int32_t *extra_size)
{
    XMLCONTENTCHUNK *target_start = NULL;
    uint32_t name;
    STRING_CHUNK *sc = ap->string_chunk;
    XMLCONTENTCHUNK *node = NULL;
    XMLCONTENTCHUNK *target_end = NULL;
    ATTRIBUTE *list = NULL;
    ATTRIBUTE *attr = NULL;
    int degree = 0;

    if (tag_name == NULL || deep < 0)
    {
        fprintf(stderr, "ERROR: Illegal parameters.\n");
        return -1;
    }

    target_start = FindTagStartChunk(ap, tag_name, deep);
    if (target_start == NULL)
    {
        return -1;
    }

    name = target_start->start_tag_chunk->name;

	node = target_start;
	while (node)
	{
        if (node->chunk_type == CHUNK_STARTTAG)
        {
            degree++;
        }
        if (node->chunk_type == CHUNK_ENDTAG)
        {
            degree--;
            if (strcmp((const char *)sc->strings[node->end_tag_chunk->name], "manifest") == 0)
            {
                ;
            }
            else if (node->end_tag_chunk->name == name && degree == 0)
            {
                target_end = node;
                break;
            }
        }
        node = node->child;
	}

	if (target_end == NULL)
	{
        fprintf(stderr, "ERROR: tag_name: '%s' does not exist.\n", tag_name);
        return -1;
	}

	target_start->parent->child = target_start->child;
	target_start->child->parent = target_start->parent;
	target_end->parent->child = target_end->child;
	target_end->child->parent = target_end->parent;

	*extra_size -= (target_start->chunk_size + target_end->chunk_size);

	list = target_start->start_tag_chunk->attr;
	while (list)
	{
        attr = list;
        list = list->next;
        free(attr);
	}
	free(target_start->start_tag_chunk);
	free(target_start);
	free(target_end->end_tag_chunk);
	free(target_end);

    return 0;
}

static int HandleTagChunk(PARSER *ap, OPTIONS *options, int32_t *extra_size)
{
    switch (options->mode)
    {
    case MODE_ADD:
        return AddTagChunk(ap, options->tag_name, options->deep, options->count, extra_size);
    case MODE_MODIFY:
        return ModifyTagChunk(ap, options->tag_name, options->deep, options->new_tag_name, extra_size);
    case MODE_REMOVE:
        return RemoveTagChunk(ap, options->tag_name, options->deep, extra_size);
    default:
        return -1;
    }
}

int HandleAXML(PARSER *ap, OPTIONS *options)
{
    int32_t extra_size = 0;
    int result = 0;
    BUFF buf;
    FILE *fp = NULL;
    int ret;

	if (ap == NULL || ap->string_chunk == NULL || ap->resourceid_chunk == NULL || ap->xmlcontent_chunk == NULL || options == NULL)
	{
        result = -1;
		goto bail;
	}

	switch (options->command)
	{
	case COMMAND_TAG:
        result = HandleTagChunk(ap, options, &extra_size);
        break;
	case COMMAND_ATTR:
        result = HandleAttribute(ap, options, &extra_size);
        break;
	}
	if (result != 0)
	{
        goto bail;
	}
	ap->header->size += extra_size;

	if (InitBuff(&buf) != 0)
	{
		result = -1;
		goto bail;
	}

	RebuildAXML(ap, &buf);
	//printf("buf size: %x\n", buf.cur);
	//printf("ap size: %x\n", ap->header->size);

    fp = fopen(options->output_file, "wb");
	if (fp == NULL)
	{
		fprintf(stderr, "Error: open output file failed.\n");
		result = -1;
		goto bail;
	}


	ret = fwrite(buf.data, 1, buf.cur, fp);
	if (ret != buf.cur)
	{
		fprintf(stderr, "Error: fwrite outbuf error.\n");
		result = -1;
	}

    if (fp != NULL)
    {
        fclose(fp);
    }

bail:
    if (buf.size > 0)
    {
        free(buf.data);
    }
	return result;
}
