RUMPTOP= ${TOPRUMP}

RUMPCOMP_MAKEFILEINC_rumpdev_pci:= ${.PARSEDIR}/Makefile.inc
.export RUMPCOMP_MAKEFILEINC_rumpdev_pci

.include "${RUMPTOP}/dev/Makefile.rumpdevcomp"

.for pcidev in ${RUMPPCIDEVS}
SUBDIR+= ${RUMPTOP}/dev/lib/lib${pcidev}
.endfor

.include <bsd.subdir.mk>
