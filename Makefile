CC := g++

SRCDIR := src
BINDIR := bin
TARGET := bchain

CPPFLAGS := --std=c++17 -Og -Wall
LIBFLAGS := -pthread -lssl -lcrypto
INCDIRS := -I include

.PHONY: all compile run clean cclean

all: compile run clean

compile:
	@mkdir -p $(BINDIR)
	@$(CC) $(CPPFLAGS) $(LIBFLAGS) $(INCDIRS) $(SRCDIR)/Main.cpp -o $(BINDIR)/$(TARGET)

run:
	@./$(BINDIR)/$(TARGET)

cclean:
	@find . -name "*.o" -type f -delete
	@find . -name "*.gch" -type f -delete

clean: cclean
	@rm -f -r $(BINDIR)
	@find . -name "*.out" -type f -delete
