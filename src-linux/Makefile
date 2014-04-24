RUMPTOP= ${TOPRUMP}

RUMP_PCI_USER:=	${.PARSEDIR}/pci_user-uio_linux.c
.export RUMP_PCI_USER

.include "${RUMPTOP}/dev/Makefile.rumpdevcomp"

.for pcidev in ${RUMPPCIDEVS}
SUBDIR+= ${RUMPTOP}/dev/lib/lib${pcidev}
.endfor

.include <bsd.subdir.mk>
