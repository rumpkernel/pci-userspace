RUMPTOP= ${TOPRUMP}

RUMPCOMP_USER_SRCS.rumpdev_pci=		pci_user-gnu.c experimentalUser.c mach_debugUser.c
RUMPCOMP_USER_PATH.rumpdev_pci:=	${.PARSEDIR}
RUMPCOMP_USER_CPPFLAGS.rumpdev_pci:=	-I${.PARSEDIR}
RUMPCOMP_CPPFLAGS.rumpdev_pci:=		-I${.PARSEDIR}
RUMPCOMP_MAKEFILEINC.rumpdev_pci:=	Makefile.inc

.export RUMPCOMP_USER_SRCS.rumpdev_pci
.export RUMPCOMP_USER_PATH.rumpdev_pci
.export RUMPCOMP_USER_CPPFLAGS.rumpdev_pci
.export RUMPCOMP_CPPFLAGS.rumpdev_pci
.export RUMPCOMP_MAKEFILEINC.rumpdev_pci

.include "${RUMPTOP}/dev/Makefile.rumpdevcomp"

.for pcidev in ${RUMPPCIDEVS}
SUBDIR+= ${RUMPTOP}/dev/lib/lib${pcidev}
.endfor

.include <bsd.subdir.mk>
