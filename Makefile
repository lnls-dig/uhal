CFLAGS = -O2 -std=c99 -g2
LDLIBS = -lpcidriver -lpthread

all: readRegs

readRegs: readRegs.c pcie.c
