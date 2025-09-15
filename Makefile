CC=g++
CFLAGS=-Wall -O2
LIBS=-lboost_system -lcurl
TARGET=smtp2telegram
SRC=smtp2telegram.cpp
BUILDDIR=build
BINTARGET=$(BUILDDIR)/$(TARGET)

all: $(BUILDDIR) $(BINTARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BINTARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BINTARGET) $(LIBS)

clean:
	rm -rf $(BUILDDIR)