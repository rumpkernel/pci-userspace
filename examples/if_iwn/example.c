#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>

#include <rump/rump.h>

/* now, obviously you need the right firmware file for _your_ device */
#define FIRMWARE "./iwlwifi-5000-2.ucode"

int
main()
{
	struct stat sb;

	if (stat(FIRMWARE, &sb) == -1)
		err(1, "need firmware file %s", FIRMWARE);

	/* make sure we see dmesg */
	setenv("RUMP_VERBOSE", "1", 1);

	/* bootstrap rump kernel */
	rump_init();

	/*
	 * The iwn driver needs to load a firmware before anything
	 * can happen with the device.  We assume that the user will
	 * copy the correct file into this directory, and we will
	 * expose it under the firmware directory to the rump kernel.
	 */
	if (rump_pub_etfs_register(
	    "/libdata/firmware/if_iwn/iwlwifi-5000-2.ucode",
	    FIRMWARE, RUMP_ETFS_REG) != 0)
			err(1, "etfs");

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
