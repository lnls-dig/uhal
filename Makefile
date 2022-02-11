BASEFLAGS = -O2 -g2 -Wall -Wextra -Ithird_party/docopt.cpp
CFLAGS = -std=c99 $(BASEFLAGS)
CXXFLAGS = -std=c++11 $(BASEFLAGS)

LDLIBS = -lpcidriver -lpthread

ALL = decode-reg
OBJ = decode-reg.o pcie.o decoders.o third_party/docopt.cpp/docopt.o

all: $(ALL)

decode-reg: $(OBJ)
	$(CXX) $(BASEFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -f $(ALL) $(OBJ)
