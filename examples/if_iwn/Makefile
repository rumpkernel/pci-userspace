PROG=	example
NOMAN=	man, no man

# wireless
LDADD+=	-lrumpdev_pci_if_iwn -lrumpnet_net80211

# wpa_supplicant reads /dev/urandom
LDADD+=	-lrumpdev_rnd

.include "${.CURDIR}/../Makefile.inc"

.include <bsd.prog.mk>
