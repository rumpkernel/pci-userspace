#
# components all examples need
#

# PCI device access
LDADD+= -lrumpdev_pci -lrumpdev

# networking stack
LDADD+=	-lrumpnet_netinet6 -lrumpnet_netinet -lrumpnet_net -lrumpnet

# bpf for dhcpclient and wpa_supplicant
LDADD+=	-lrumpdev_bpf 

# for /dev/bpf access
LDADD+=	-lrumpvfs

# remote syscalls
LDADD+=	-lrumpkern_sysproxy

# rump kernel core
LDADD+=	-lrump

CPPFLAGS+= -I${.CURDIR}/../common
