#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "AmBinaryEditor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int HandleCommand(PARSER *ap, OPTIONS *options)
{
    return HandleAXML(ap, options);
}

int main(int argc, char *argv[])
{
    int want_usage = 0;
    int result = 0;
    OPTIONS *options = NULL;
    FILE *fp = NULL;
	char *in_buf = NULL;
	size_t in_size;
	PARSER *ap = NULL;

    if (argc < 2)
    {
        want_usage = 1;
        goto bail;
    }

    options = (OPTIONS *)malloc(sizeof(OPTIONS));
    memset(options, 0, sizeof(OPTIONS));

    if (argv[1][0] == 't')
    {
        options->command = COMMAND_TAG;
    }
    else if (argv[1][0] == 'a')
    {
        options->command = COMMAND_ATTR;
    }
    else
    {
        fprintf(stderr, "ERROR: Unknown command '%s'.\n", argv[1]);
        want_usage = 1;
        goto bail;
    }
    argc -= 2;
    argv += 2;

    if (!argc)
    {
        fprintf(stderr, "ERROR: no mode supplied.\n");
        want_usage = 1;
        goto bail;
    }

    if (argv[0][0] != '-' || argv[0][1] != '-')
    {
        fprintf(stderr, "ERROR: Unknown option '%s'.\n", argv[0]);
        want_usage = 1;
        goto bail;
    }

    if (strcmp(argv[0], "--add") == 0)
    {
        argc -= 1;
        argv += 1;
        if (!argc)
        {
            fprintf(stderr, "ERROR: No argument supplied for '--add' option.\n");
            want_usage = 1;
            goto bail;
        }
        strncpy(options->tag_name, argv[0], 128);
        options->mode = MODE_ADD;
    }
    else if (strcmp(argv[0], "--modify") == 0)
    {
        argc -= 1;
        argv += 1;
        if (!argc)
        {
            fprintf(stderr, "ERROR: No argument supplied for '--add' option.\n");
            want_usage = 1;
            goto bail;
        }
        strncpy(options->tag_name, argv[0], 128);
        options->mode = MODE_MODIFY;
    }
    else if (strcmp(argv[0], "--remove") == 0)
    {
        argc -= 1;
        argv += 1;
        if (!argc)
        {
            fprintf(stderr, "ERROR: No argument supplied for '--add' option.\n");
            want_usage = 1;
            goto bail;
        }
        strncpy(options->tag_name, argv[0], 128);
        options->mode = MODE_REMOVE;
    }
    else
    {
        fprintf(stderr, "ERROR: Unknown mode '%s'.\n", argv[1]);
        want_usage = 1;
        goto bail;
    }
    argc -= 1;
    argv += 1;

    if (!argc)
    {
        fprintf(stderr, "ERROR: no option supplied.\n");
        want_usage = 1;
        goto bail;
    }

    if (options->command == COMMAND_TAG)
    {
        while (argc && argv[0][0] == '-')
        {
            const char *cp = argv[0] + 1;
            while (*cp != '\0')
            {
                switch (*cp)
                {
                case 'd':
                    argc--;
                    argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "ERROR: No argument supplied for '-d' option.\n");
                        want_usage = 1;
                        goto bail;
                    }
                    options->deep = atoi(argv[0]);
                    break;
                case 'c':
                    argc--;
                    argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "ERROR: No argument supplied for '-l' option.\n");
                        want_usage = 1;
                        goto bail;
                    }
                    options->count = atoi(argv[0]);
                    break;
                case 'n':
                    argc--;
                    argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "ERROR: No argument supplied for '-n' option.\n");
                        want_usage = 1;
                        goto bail;
                    }
                    strncpy(options->new_tag_name, argv[0], 128);
                    break;
                case 'i':
                    argc--;
                    argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "ERROR: No argument supplied for '-i' option.\n");
                        want_usage = 1;
                        goto bail;
                    }
                    strncpy(options->input_file, argv[0], 128);
                    break;
                case 'o':
                    argc--;
                    argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "ERROR: No argument supplied for '-o' option.\n");
                        want_usage = 1;
                        goto bail;
                    }
                    strncpy(options->output_file, argv[0], 128);
                    break;
                default:
                    fprintf(stderr, "ERROR: Unknown flag '-%c'.\n", *cp);
                    want_usage = 1;
                    goto bail;
                }
                argc--;
                argv++;
                break;
            }
        }
    }
    else if (options->command == COMMAND_ATTR)
    {
        while (argc && argv[0][0] == '-')
        {
            const char *cp = argv[0] + 1;
            while (*cp != '\0')
            {
                switch (*cp)
                {
                case 'd':
                    argc--;
                    argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "ERROR: No argument supplied for '-d' option.\n");
                        want_usage = 1;
                        goto bail;
                    }
                    options->deep = atoi(argv[0]);
                    break;
                case 't':
                    argc--;
                    argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "ERROR: No argument supplied for '-t' option.\n");
                        want_usage = 1;
                        goto bail;
                    }
                    options->attr_type = atoi(argv[0]);
                    break;
                case 'n':
                    argc--;
                    argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "ERROR: No argument supplied for '-n' option.\n");
                        want_usage = 1;
                        goto bail;
                    }
                    strncpy(options->attr_name, argv[0], 128);
                    break;
                case 'v':
                    argc--;
                    argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "ERROR: No argument supplied for '-v' option.\n");
                        want_usage = 1;
                        goto bail;
                    }
                    strncpy(options->attr_value, argv[0], 128);
                    break;
                case 'r':
                    argc--;
                    argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "ERROR: No argument supplied for '-r' option.\n");
                        want_usage = 1;
                        goto bail;
                    }
                    options->resource_id = atoi(argv[0]);
                    break;
                case 'i':
                    argc--;
                    argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "ERROR: No argument supplied for '-i' option.\n");
                        want_usage = 1;
                        goto bail;
                    }
                    strncpy(options->input_file, argv[0], 128);
                    break;
                case 'o':
                    argc--;
                    argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "ERROR: No argument supplied for '-o' option.\n");
                        want_usage = 1;
                        goto bail;
                    }
                    strncpy(options->output_file, argv[0], 128);
                    break;
                default:
                    fprintf(stderr, "ERROR: Unknown flag '-%c'.\n", *cp);
                    want_usage = 1;
                    goto bail;
                }
                argc--;
                argv++;
                break;
            }
        }
    }

    if (options->input_file == NULL || options->output_file == NULL)
    {
        fprintf(stderr, "ERROR: no input_file or output_file supplied.\n");
        want_usage = 1;
        goto bail;
    }

    fp = fopen(options->input_file, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "ERROR: open %s.\n", options->input_file);
        goto bail;
    }

    fseek(fp, 0, SEEK_END);
	in_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	in_buf = (char *)malloc(in_size * sizeof(char));
	if (fread(in_buf, 1, in_size, fp) != in_size)
	{
		fprintf(stderr, "Error: read %s.\n", options->input_file);
		free(in_buf);
		fclose(fp);
		goto bail;
	}

	ap = (PARSER *)malloc(sizeof(PARSER));
	memset(ap, 0, sizeof(PARSER));
	if (ParserAxml(ap, in_buf, in_size) == -1)
	{
		fprintf(stderr, "Error: axml parser.\n");
		return -1;
	}
    fclose(fp);

    result = HandleCommand(ap, options);

bail:
    if (want_usage)
    {
        Usage();
        result = -1;
    }

    return result;
}
