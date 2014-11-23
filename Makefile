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

EXE := bin/calc_vars bin/draw_vars bin/resize

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
	@$(CPP) -std=c++11 $(CFLAGS) $(ROOT_CFLAGS) -c $(filter %.cc,$^) -o $@

# main object rules
lib/calc_vars.o: lib/%.o: src/%.cc
	@echo -e "Compiling \E[0;49;94m"$@"\E[0;0m ... "
	@$(CPP) -std=c++11 $(CFLAGS) $(ROOT_CFLAGS) $(MAGIC_CFLAGS) -c $(filter %.cc,$^) -o $@

lib/draw_vars.o: lib/%.o: src/%.cc
	@echo -e "Compiling \E[0;49;94m"$@"\E[0;0m ... "
	@$(CPP) -std=c++11 $(CFLAGS) $(ROOT_CFLAGS) -c $(filter %.cc,$^) -o $@

lib/resize.o: lib/%.o: src/%.cc
	@echo -e "Compiling \E[0;49;94m"$@"\E[0;0m ... "
	@$(CPP) -std=c++11 $(CFLAGS) $(MAGIC_CFLAGS) -c $(filter %.cc,$^) -o $@

# executable rules
bin/calc_vars: bin/%: lib/%.o
	@echo -e "Linking \E[0;49;92m"$@"\E[0;0m ... "
	@$(CPP) -Wl,--no-as-needed $(filter %.o,$^) -o $@ $(MAGIC_LIBS) $(ROOT_LIBS) -lpthread

bin/draw_vars: bin/%: lib/%.o
	@echo -e "Linking \E[0;49;92m"$@"\E[0;0m ... "
	@$(CPP) $(filter %.o,$^) -o $@ $(ROOT_LIBS) -lboost_program_options -lboost_regex

bin/resize: bin/%: lib/%.o
	@echo -e "Linking \E[0;49;92m"$@"\E[0;0m ... "
	@$(CPP) $(filter %.o,$^) -o $@ $(MAGIC_LIBS) -lpthread

# OBJ dependencies

# EXE_OBJ dependencies
lib/calc_vars.o   : src/running_stat.h
lib/draw_vars.o   : src/hist_wrap.h

# EXE dependencies
bin/calc_vars     : lib/running_stat.o
bin/draw_vars     : lib/hist_wrap.o

clean:
	rm -rf bin lib

