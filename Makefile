CC=g++
CFLAGS=-Wall -O2 -Iincludes
LIBS=-lboost_system -lcurl
TARGET=smtp2telegram
SRC=src/smtp2telegram.cpp
BUILDDIR=build
BINTARGET=$(BUILDDIR)/$(TARGET)

all: $(BUILDDIR) $(BINTARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BINTARGET): $(SRC) includes/smtp2telegram.h
	$(CC) $(CFLAGS) $(SRC) -o $(BINTARGET) $(LIBS)

clean:
	rm -rf $(BUILDDIR)