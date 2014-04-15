#include <sys/stat.h>

/* now, obviously you need the right firmware file for _your_ device */
#define FIRMWARE "./iwlwifi-5000-2.ucode"

#define CTRLSOCK "/tmp/iwnsock"
#define CTRLURL "unix://" CTRLSOCK

#include "common.c"

#define WPA_ETFS "/wpa-etfs.conf"

int
main()
{
	struct stat sb;
	int rv;

	if (stat(FIRMWARE, &sb) == -1)
		err(1, "need firmware file %s", FIRMWARE);

	common_bootstrap();

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

	/*
	 * If ./wpa.conf exists, expose it to the rump kernel.
	 * You can of course copy other wpa.conf's into the rump
	 * kernel file systems, but this avoids having to do it
	 * every time a testing loop.
	 */
	rv = rump_pub_etfs_register(WPA_ETFS, "./wpa.conf", RUMP_ETFS_REG);
	if (rv == 0)
		printf("\netfs registered ./wpa.conf at " WPA_ETFS "\n");

	common_listen();
}
