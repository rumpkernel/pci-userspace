PROG=	example
NOMAN=	man, no man

#
# components we need
#

# wireless
LDADD+=	-lrumpdev_pci_if_iwn -lrumpnet_net80211

# PCI device access
LDADD+= -lrumpdev_pci -lrumpdev

# networking stack
LDADD+=	-lrumpnet_netinet6 -lrumpnet_netinet -lrumpnet_net -lrumpnet

# wpa_supplicant uses bpf
LDADD+=	-lrumpdev_bpf 

LDADD+=	-lrumpvfs -lrump

CPPFLAGS+= -I${.CURDIR}/../common

.include <bsd.prog.mk>
