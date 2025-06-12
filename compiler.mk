# compiler quirks
COMPILER_NAME:=$(shell $(CC) -v 2>&1 | tail -n 1)

# GCC quirks
ifeq ($(findstring gcc, $(COMPILER_NAME)), gcc)
CC_MAJOR_VER:=$(shell $(CC) -dumpversion | sed 's/\..*//')
CC_IS_OLD:=$(shell test $(CC_MAJOR_VER) -lt 5 && echo 1 || echo 0)

ifneq ($(CC_IS_OLD),0)
# To be removed when GCC is fixed (see bug #59124)
CFLAGS+=-Wno-array-bounds
CFLAGS+=-Wno-missing-braces
# To be removed when GCC is fixed (see bug #36750, fixed from gcc 5.x)
CFLAGS+=-Wno-missing-field-initializers
endif
endif
