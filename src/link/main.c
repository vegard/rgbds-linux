#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "asmotor.h"

#include "link/object.h"
#include "link/output.h"
#include "link/assign.h"
#include "link/patch.h"
#include "link/mylink.h"
#include "link/mapfile.h"
#include "link/main.h"
#include "link/library.h"

enum eBlockType {
	BLOCK_COMMENT,
	BLOCK_OBJECTS,
	BLOCK_LIBRARIES,
	BLOCK_OUTPUT
};

SLONG options = 0;
SLONG fillchar = 0;
char smartlinkstartsymbol[256];

/*
 * Print the usagescreen
 *
 */

static void 
usage(void)
{
	printf("RGBLink v" LINK_VERSION " (part of ASMotor " ASMOTOR_VERSION
	    ")\n\n");
	printf("usage: rgblink [-t] [-l library] [-m mapfile] [-n symfile] [-o outfile]\n");
	printf("\t    [-s symbol] [-z pad_value] objectfile [...]\n");

	exit(1);
}

/*
 * The main routine
 *
 */

int 
main(int argc, char *argv[])
{
	int ch;
	char *ep;

	if (argc == 1)
		usage();

	while ((ch = getopt(argc, argv, "l:m:n:o:p:s:t")) != -1) {
		switch (ch) {
		case 'l':
			lib_Readfile(optarg);
			break;
		case 'm':
			SetMapfileName(optarg);
			break;
		case 'n':
			SetSymfileName(optarg);
			break;
		case 'o':
			out_Setname(optarg);
			break;
		case 'p':
			fillchar = strtoul(optarg, &ep, 0);
			if (optarg[0] == '\0' || *ep != '\0') {
				fprintf(stderr, "Invalid argument for option 'p'\n");
				exit(1);
			}
			if (fillchar < 0 || fillchar > 0xFF) {
				fprintf(stderr, "Argument for option 'p' must be between 0 and 0xFF");
				exit(1);
			}
			break;
		case 's':
			options |= OPT_SMART_C_LINK;
			strcpy(smartlinkstartsymbol, optarg);
			break;
		case 't':
			options |= OPT_SMALL;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	for (int i = 0; i < argc; ++i)
		obj_Readfile(argv[i]);

	AddNeededModules();
	AssignSections();
	CreateSymbolTable();
	Patch();
	Output();
	CloseMapfile();

	return (0);
}
