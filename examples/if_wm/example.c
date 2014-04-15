#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>

#include <rump/rump.h>

int
main()
{

	/* make sure we see dmesg */
	setenv("RUMP_VERBOSE", "1", 1);

	/* bootstrap rump kernel */
	rump_init();

	/* make sure old control suckets are not there */
	unlink("/tmp/wmtest");

	/* start listening to remote requests */
	rump_init_server("unix:///tmp/wmtest");

	/*
	 * we are (most likely) running as root, just make sure
	 * non-root clients can access us.
	 */
	chmod("/tmp/wmtest", 0666);

	/* wait for remote clients commands until the bitter end */
	pause();
}
