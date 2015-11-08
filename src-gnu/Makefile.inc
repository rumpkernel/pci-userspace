LDFLAGS += -lpciaccess

experimentalUser.c:
	echo '#include <mach/experimental.defs>' \
		| ${CC} -E -x c - -o - \
		| mig -cc cat - /dev/null -subrprefix __ \
			-user experimentalUser.c \
			-server /dev/null \
			-header experimental_U.h
