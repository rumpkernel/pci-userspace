LDFLAGS += -lpciaccess

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
