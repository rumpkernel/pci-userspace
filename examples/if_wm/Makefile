PROG=	example
NOMAN=	man, no man

# components we need
LDADD+=	-lrumpdev_pci_if_wm -lrumpdev_miiphy -lrumpdev_pci -lrumpdev
LDADD+=	-lrumpvfs -lrumpnet_netinet -lrumpnet_net -lrumpnet -lrump

CPPFLAGS+=-I${.CURDIR}/../common

.include <bsd.prog.mk>
