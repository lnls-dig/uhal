CFLAGS = -O2 -std=c99 -g2
LDLIBS = -lpcidriver -lpthread

all: decode-reg

decode-reg: decode-reg.c pcie.c decoder.c
