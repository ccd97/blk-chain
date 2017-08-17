CPPFLAGS = --std=c++17 -Og -Wall -pthread -lssl -lcrypto
INCDIRS = -I ./include
GCCBIN = g++

.PHONY: all compile run clean cclean

all: compile run clean

compile:
	@$(GCCBIN) $(CPPFLAGS) $(INCDIRS) Main.cpp -o bchain

run:
	@./bchain

cclean:
	@find . -name "*.gch" -type f -delete

clean: cclean
	@rm -f bchain 
	@find . -name "*.out" -type f -delete
