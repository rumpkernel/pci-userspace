# make defs for gnu PCI component

PCIDIR:=	${.PARSEDIR}
.PATH:		${PCIDIR}

RUMPCOMP_USER_SRCS=	pci_user-gnu.c experimentalUser.c mach_debugUser.c
RUMPCOMP_USER_CPPFLAGS+=-I${PCIDIR}
RUMPCOMP_CPPFLAGS+=	-I${PCIDIR}
CPPFLAGS+=		-I${PCIDIR}
LDFLAGS+= -Wl,--no-as-needed -lpciaccess -Wl,--as-needed

experimentalUser.c:
	echo '#include <mach/experimental.defs>' \
		| ${CC} -E -x c - -o - \
		| mig -cc cat - /dev/null -subrprefix __ \
			-user experimentalUser.c \
			-server /dev/null \
			-header experimental_U.h

mach_debugUser.c:
	echo '#include <mach_debug/mach_debug.defs>' \
		| ${CC} -E -x c - -o - \
		| mig -cc cat - /dev/null -subrprefix __ \
			-user mach_debugUser.c \
			-server /dev/null \
			-header mach_debug_U.h
