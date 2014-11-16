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

EXE := bin/test_variables_threaded

all: $(DIRS) $(EXE)

# directories rule
$(DIRS):
	@mkdir -p $@

# class object rules
lib/running_stat.o: lib/%.o: src/%.cc src/%.h
	@echo -e "Compiling \E[0;49;96m"$@"\E[0;0m ... "
	@$(CPP) $(CFLAGS) -c $(filter %.cc,$^) -o $@

lib/hist_wrap.o: lib/%.o: src/%.cc src/%.h
	@echo -e "Compiling \E[0;49;96m"$@"\E[0;0m ... "
	@$(CPP) $(CGFLAGS) $(ROOT_CFLAGS) -c $(filter %.cc,$^) -o $@ -std=c++11

# main object rules
lib/test_variables_threaded.o: lib/%.o: test/%.cc
	@echo -e "Compiling \E[0;49;94m"$@"\E[0;0m ... "
	@$(CPP) $(CFLAGS) $(ROOT_CFLAGS) $(MAGIC_CFLAGS) -c $(filter %.cc,$^) -o $@ -std=c++11

# executable rules
bin/test_variables_threaded: bin/%: lib/%.o
	@echo -e "Linking \E[0;49;92m"$@"\E[0;0m ... "
	@$(CPP) -Wl,--no-as-needed $(filter %.o,$^) -o $@ $(MAGIC_LIBS) $(ROOT_LIBS) -lpthread -lboost_regex

# OBJ dependencies

# EXE_OBJ dependencies
lib/test_variables_threaded.o: src/running_stat.h src/hist_wrap.h

# EXE dependencies
bin/test_variables_threaded: lib/running_stat.o lib/hist_wrap.o

clean:
	rm -rf bin lib

