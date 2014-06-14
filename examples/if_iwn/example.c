#include <sys/stat.h>

/* now, obviously you need the right firmware file for _your_ device */
#define FIRMWARE "iwlwifi-5000-2.ucode"

#define CTRLSOCK "/tmp/iwnsock"
#define CTRLURL "unix://" CTRLSOCK

#include "common.c"

#define WPA_ETFS "/wpa-etfs.conf"

static struct rump_boot_etfs eb = {
	.eb_key = "/libdata/firmware/if_iwm/" FIRMWARE,
	.eb_hostpath = "./" FIRMWARE,
	.eb_type = RUMP_ETFS_REG,
	.eb_begin = 0,
	.eb_size = RUMP_ETFS_SIZE_ENDOFF,
};

int
main()
{
	struct stat sb;
	int rv;

	if (stat(FIRMWARE, &sb) == -1)
		err(1, "need firmware file %s", FIRMWARE);
	rump_boot_etfs_register(&eb);

	common_bootstrap();

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
