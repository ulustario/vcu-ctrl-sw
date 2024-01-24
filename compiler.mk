# compiler quirks

COMPILER_NAME:=$(shell $(CC) --version | head -n 1 | cut -d ' ' -f 1)

ifeq ($(findstring gcc,$(COMPILER_NAME)),gcc)

GCC_MAJOR_VER:=$(shell $(CC) -dumpversion | sed 's/\..*//')
GCC_IS_OLD:=$(shell test $(GCC_MAJOR_VER) -lt 5 && echo 1 || echo 0)

ifneq ($(GCC_IS_OLD),0)
# To be removed when GCC is fixed (see bug #59124)
CFLAGS+=-Wno-array-bounds
CFLAGS+=-Wno-missing-braces
# incompatible pointer type bug when 2d array constness is involved (gcc 4.9)
# no disable flag before 5.0
$(BIN)/lib_parsing/I_PictMngr.c.o: CFLAGS+=-w
# To be removed when GCC is fixed (see bug #36750, fixed from gcc 5.x)
CFLAGS+=-Wno-missing-field-initializers
endif

endif
