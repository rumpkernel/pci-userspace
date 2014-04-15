#define CTRLSOCK "/tmp/wmsock"

#include "common.c"

int
main()
{

	common_bootstrap();
	common_listen();
}
