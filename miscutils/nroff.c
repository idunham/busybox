/* mini groff implementation for busybox
* Copyright (C) 2008 by Ivana Varekova <varekova@redhat.com>
* Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
*/


//applet:IF_NROFF(APPLET(nroff, BB_DIR_USR_BIN, BB_SUID_DROP))

//kbuild:lib-$(CONFIG_NROFF)       += nroff.o

//config:config NROFF
//config:	bool "nroff"
//config:	default n
//config:	help
//config:	nroff formats troff documents (used by man applet).
//config:	Currently this has very bad formatting.

//usage:#define nroff_trivial_usage 
//usage:    "[OPTION] [FILE]"
//usage:#define nroff_full_usage "\n\n"
//usage:     "\nOptions:\n"
//usage:     "\n      -man        The traditional output format\n"
//usage:     "\n      -mandoc     The general output format\n"
//usage:     "\n      -T<dev>    Accepted but not implemented yet\n"

#include "libbb.h"

#define V(a, b) ((a) * 256 + (b))

enum {
	IDEFAULT = 1,
	CH_NUM = 78,
};

struct text_format {
	int ind1;	/* indentation of the first row of the paragraph */
	int ind2;	/* indentation of the next rows of the paragraph */
	int alt1;
	int alt2;
	int flags;
};

enum {
	FLAG_bold = 1,          /* bold */
	FLAG_it = 2,            /* italic */
	FLAG_break = 4,         /* break */
	FLAG_skip = 8,          /* insert tabulator */
	FLAG_ind_reset = 16,    /* reset indentation */
	FLAG_cont = 32,         /* continue on the next row if the present
	                         * is empty - after the format character */
	FLAG_alt = 64,          /* alterating mode */
	FLAG_nl = 128,							
};

/* go to the next line (and do proper indentation) */
static void print_indent(int i, int *cn, int flags)
{
	bb_putchar('\n');
	*cn = 0;
	if (flags) {
		i *= 7;
		*cn = i;
		printf("%*s", i, "");
	}
}

/* read format tag and set format field */
static int scan_format(char *line, struct text_format *f)
{
	int counter;

	f->flags = 0;

	/* read format tag */
	switch (V(line[1], line[2])) {
		case V('\\', '"'):	/* comment - just skip it */
		case V('d', 'e'):	/* some macros we ignore	*/
		case V('n', 'e'):
		case V('n', 'f'):
		case V('f', 't'):
		case V('d', 's'):
			return strlen(line);
			break;
		/* case V('d', 'e') */
		/* section names */
		case V('S', 'H'):
			f->flags |= (FLAG_break | FLAG_bold | FLAG_ind_reset | FLAG_skip | FLAG_cont | FLAG_nl);
			f->ind1 = 0;
			f->ind2 = IDEFAULT;
			break;
		/* indentation */
		case V('b', 'r'):
		case V('T', 'P'):
			f->flags |= (FLAG_break | FLAG_ind_reset);
			f->ind1 = IDEFAULT;
			f->ind2 = IDEFAULT + 1;
			break;
		case V('L', 'P'):
		case V('P', 'P'):
		case V('P', ' '):
			f->flags |= (FLAG_break | FLAG_ind_reset);
			f->ind1 = IDEFAULT;
			f->ind2 = IDEFAULT;
			break;
		case V('s', 'p'):
			f->flags |= (FLAG_ind_reset | FLAG_bold);
			f->ind1 = IDEFAULT;
			f->ind2 = IDEFAULT;
			break;
		/* alternating modes */
		case V('I', 'R'):
			f->flags |= FLAG_alt;
			f->alt1 = 2;
			f->alt2 = 0;
			break;
		case V('R', 'I'):
			f->flags |= FLAG_alt;
			f->alt1 = 0;
			f->alt2 = 2;
			break;
		case V('I', 'B'):
			f->flags |= FLAG_alt;
			f->alt1 = 2;
			f->alt2 = 1;
			break;
		case V('B', 'I'):
			f->flags |= FLAG_alt;
			f->alt1 = 1;
			f->alt2 = 2;
			break;
		case V('B', 'R'):
			f->flags |= FLAG_alt;
			f->alt1 = 1;
			f->alt2 = 0;			
			break;
		case V('R', 'B'):
			f->flags |= FLAG_alt;
			f->alt1 = 1;
			f->alt2 = 0;
			break;
		/* bold and italic modes */
		case V('B', ' '):
			f->flags |= FLAG_bold;
			break;
		case V('I', ' '):
			f->flags |= FLAG_it;
			break;
	}

	counter = 0;
	while ((line[counter] | ' ') != ' ')
		counter++;
		
	if (line[counter]) {
		counter++;
	}
	return counter;
}

/* read and display input file */
static void convert_file(FILE *fd, int opt)
{
	char *line;
	int alt;
	int count;	// counter of the input characters
	int cn;		// the number of output characters (maximum is CH_NUM characters per row)
	struct text_format *f;	// format structure
	char nl;	// save whether to make a newline at EOL

	f = xzalloc(sizeof(*f));
	f->ind2 = IDEFAULT;
	nl = 0;		 

	while ((line = xmalloc_fgetline(fd)) != NULL) {
		count = 0;
		f->flags = 0;

		/* format tag starts here */
		if (line[0] == '.') {
			count = scan_format(line, f);
		}

		if (f->flags & FLAG_break) {
			print_indent(f->ind1, &cn, opt);
		}

		if ((f->flags & FLAG_ind_reset) && opt) {
			print_indent(f->ind2, &cn, opt);
		}

		if ((f->flags & FLAG_cont) && (count == strlen(line))) {
			free(line);
			line = xmalloc_fgetline(fd);
			if (line == NULL)
				return;
		} 
		if (f->flags & FLAG_nl) {
				nl = 1;		// Save state so we break after .SH TEXT
		}
		
		alt = 1;
		while (count < strlen(line)) {

			/* read character is the metasymbol */
			if (line[count] == '\\') {
				count++;
				switch (line[count]) {
				/* special characters */
				case '-':
					printf("-\b-");
					cn++;
					count++;
					break;
				case '\\':
					bb_putchar(line[count]);
					cn++;
					count++;
					break;
				case '^':
					count++;
					break;
				/* format change */
				case 'f':
					count++;
					switch (line[count]) {
					case 'I':
						f->flags |= FLAG_it;
						count++;
						break;
					case 'B':
						f->flags |= FLAG_bold;
						count++;
						break;
					case 'R':
					case 'P':
						f->flags &= ~(FLAG_it | FLAG_bold);
						count++;
						break;
					}
					break;
				}
			}

			/* print the character */
			else {

				if (line[count] != '"') {
					if (cn > CH_NUM) {
						print_indent(f->ind2, &cn, opt);
					}

					if ((f->flags & FLAG_it)
					 || ((f->flags & FLAG_alt)
					     &&	(((alt == 1) && (f->alt1 == 2)) || ((alt == 0) && (f->alt2 == 2))))
					) {
						/* italic */
						
						if (opt) {
							printf("_\b%c", line[count]);
							cn++;
						}
					} else if ((f->flags & FLAG_bold)
					 || ((f->flags & FLAG_alt)
					     &&	(((alt == 1) && (f->alt1 == 1)) || ((alt == 0) && (f->alt2 == 1))))
					) {
						/* bold */
						if (opt) {
							printf("%c\b%c", line[count], line[count]);
							cn++;
						}
					} else if (!(f->flags & FLAG_alt) || opt) {
						/* normal */
						bb_putchar(line[count]);
						cn++;
					}

					if ((f->flags & FLAG_alt) && (line[count] == ' ')) {
						alt = !alt ;
						cn++;
					}

				}
				count++;
			}

		}
		
		/* the change of indentation */
		if ((f->flags & FLAG_ind_reset) && (f->flags & FLAG_skip) && opt) {
			f->ind1 = IDEFAULT;
			f->ind2 = IDEFAULT;
			print_indent(f->ind1, &cn, opt);
		}
		
		if (opt) {
			bb_putchar(' ');
			cn++;
		}

		if ((nl & 1) && (count == strlen(line))) {
			printf("\n");	/* We need to break after headers */
			nl = 0;
		}

		free(line);
	}
	bb_putchar('\n');
}


int nroff_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int nroff_main(int argc UNUSED_PARAM, char **argv)
{
	int opt;

	/* -T<param> is ignored, -m<param> is interpreted as if was
	 * -man or -mandoc (we do not check actual <param>) */
	opt = (1 & getopt32(argv, "m:T:", NULL, NULL));
	argv += optind;

	if (argv[0] == NULL) {
		*--argv = (char*)"-";
	}

	do {
		FILE *file = fopen_or_warn_stdin(*argv);
		if (!file) {
			continue;
		}
		convert_file(file, opt);
		fclose_if_not_stdin(file);
	} while (*++argv);

	return 0;
}
