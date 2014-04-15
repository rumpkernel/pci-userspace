#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rump.h>

#define CTRLURL "unix://" CTRLSOCK

static void
common_bootstrap(void)
{

	/* make sure we see dmesg */
	setenv("RUMP_VERBOSE", "1", 1);

	/* bootstrap rump kernel */
	if (rump_init() != 0)
		errx(1, "rump kernel bootstrap");
}

static void
common_listen(void)
{

	/* make sure old control suckets are not there */
	unlink(CTRLSOCK);

	/* start listening to remote requests */
	if (rump_init_server(CTRLURL) != 0)
		errx(1, "rump_init_server failed!");

	/*
	 * we are (most likely) running as root, just make sure
	 * non-root clients can access us.
	 */
	chmod(CTRLSOCK, 0666);

	printf("\nlistening for remote clients at: %s\n", CTRLURL);

	/* wait for remote clients commands until the bitter end */
	pause();
}
