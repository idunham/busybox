/* vi: set sw=4 ts=4: */
/*
 * guess_fs based on a version by "jamesbond"
 *
 * Copyright 2013, ????
 *
 * Licensed under GPLv2, see file LICENSE in this source tree
 */

//applet:IF_GUESS_FS(APPLET(guess_fs, BB_DIR_BIN, BB_SUID_MAYBE))

//kbuild:lib-$(CONFIG_GUESS_FS) += guess_fs.o

//config:config GUESS_FS
//config:	bool "guess_fs"
//config:	default n
//config:	help
//config:	  Guess filesystem type of device.
//config:	  (As in Puppy guess_fstype)

//usage:#define guess_fs_trivial_usage
//usage:	"[BLOCKDEV]..."
//usage:#define guess_fs_full_usage "\n\n"
//usage:	"[BLOCKDEV]..."

#include "libbb.h" 
#include "volume_id/volume_id_internal.h"
#include <fcntl.h>
#include <sys/stat.h>

#define PSTR(x) (x), strlen(x)

int guess_fs_main (int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int guess_fs_main (int argc, char **argv)
{	
	struct volume_id *id=0;
	int retcode=1; //assume failure
	int fd, i;
	
	for (i=0;++i < argc;) {
		// identify
		fd = open (argv[i], O_RDONLY);
		if (fd >= 0) {
			id = volume_id_open_node (fd);
			if (id) {
				retcode = volume_id_probe_all (id, 0);
				if (!retcode && id->type) {
					write (1, PSTR(id->type));
					write (1, PSTR("\n"));
				} else retcode=1;
				free_volume_id (id);
			}
		}
	}
	//failure result code
	if (retcode) write (1, PSTR("unknown\n"));

	return retcode;
}
