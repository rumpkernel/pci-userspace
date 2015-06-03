RUMPTOP= ${TOPRUMP}

RUMPCOMP_USER_SRCS.rumpdev_pci=		pci_user-uio_linux.c
RUMPCOMP_USER_PATH.rumpdev_pci:=	${.PARSEDIR}
RUMPCOMP_USER_CPPFLAGS.rumpdev_pci:=	-I${.PARSEDIR}
RUMPCOMP_CPPFLAGS.rumpdev_pci:=		-I${.PARSEDIR}

.export RUMPCOMP_USER_SRCS.rumpdev_pci
.export RUMPCOMP_USER_PATH.rumpdev_pci
.export RUMPCOMP_USER_CPPFLAGS.rumpdev_pci
.export RUMPCOMP_CPPFLAGS.rumpdev_pci

.include "${RUMPTOP}/dev/Makefile.rumpdevcomp"

.for pcidev in ${RUMPPCIDEVS}
SUBDIR+= ${RUMPTOP}/dev/lib/lib${pcidev}
.endfor

.include <bsd.subdir.mk>
