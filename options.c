#include "options.h"

#include <stdio.h>
#include <stdlib.h>

static const char* prog_name = "ameditor";

void Usage(void)
{
	fprintf(stderr, "\t------------------------------------------\n");
	fprintf(stderr, "\t|==== Android Manifest Binary Editor ====|\n");
	fprintf(stderr, "\t---------------- by ele7enxxh -------------\n");
	fprintf(stderr, "\t------------------------------------------\n");
	fprintf(stderr, "Usage:\n");
    fprintf(stderr,
	"%s t[ag] [--values] tag_name -d[eep] -c[ount] -n[ame] -i[nput] -o[utput]\n"
	"   add          add a tag\n"
	"   modify       modify a tag\n"
	"   remove       remove a tag\n\n", prog_name);
    fprintf(stderr,
	"%s a[ttr] [--values] tag_name -d[eep] -t[ype] -n[ame] -v[alue] -r[esourceid] -i[nput] -o[utput]\n"
	"   add          add a attr\n"
	"   modify       modify a attr\n"
	"   remove       remove a attr\n\n", prog_name);
}
