SHELL := /bin/bash
CPP := g++

DIRS := lib bin

CFLAGS := -Wall -O3 -Isrc
CGFLAGS := -Wall -g -Isrc

ROOT_CFLAGS := $(shell root-config --cflags)
ROOT_LIBS   := $(shell root-config --libs)

MAGIC_CFLAGS := $(shell Magick++-config --cxxflags)
MAGIC_LIBS   := $(shell Magick++-config --ldflags --libs)

.PHONY: all clean

EXE := bin/test1 bin/test2 bin/test_diffsqrgb

all: $(DIRS) $(EXE)

# directories rule
$(DIRS):
	@mkdir -p $@

# class object rules
lib/img_reader.o: lib/%.o: src/%.cc src/%.h
	@echo -e "Compiling \E[0;49;96m"$@"\E[0;0m ... "
	@$(CPP) $(CFLAGS) $(MAGIC_CFLAGS) -c $(filter %.cc,$^) -o $@

# main object rules
lib/test1.o: lib/%.o: test/%.cc
	@echo -e "Compiling \E[0;49;94m"$@"\E[0;0m ... "
	@$(CPP) $(CFLAGS) $(MAGIC_CFLAGS) -c $(filter %.cc,$^) -o $@

lib/test2.o: lib/%.o: test/%.cc
	@echo -e "Compiling \E[0;49;94m"$@"\E[0;0m ... "
	@$(CPP) $(CFLAGS) -c $(filter %.cc,$^) -o $@

lib/test_diffsqrgb.o: lib/%.o: test/%.cc
	@echo -e "Compiling \E[0;49;94m"$@"\E[0;0m ... "
	@$(CPP) $(CFLAGS) $(ROOT_CFLAGS) -c $(filter %.cc,$^) -o $@

# executable rules
bin/test1 bin/test2: bin/%: lib/%.o
	@echo -e "Linking \E[0;49;92m"$@"\E[0;0m ... "
	@$(CPP) $(filter %.o,$^) -o $@ $(MAGIC_LIBS)

bin/test_diffsqrgb: bin/%: lib/%.o
	@echo -e "Linking \E[0;49;92m"$@"\E[0;0m ... "
	@$(CPP) $(filter %.o,$^) -o $@ $(MAGIC_LIBS) $(ROOT_LIBS)

# OBJ dependencies

# EXE_OBJ dependencies
lib/test2.o : src/img_reader.h

# EXE dependencies
bin/test2   : lib/img_reader.o
bin/test_diffsqrgb : lib/img_reader.o

clean:
	rm -rf bin lib

