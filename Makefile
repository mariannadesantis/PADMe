# ==============================================================================
# Makefile for PADMe algorithm, ideal-point-checking variant
#
# Builds main.cpp against the Gurobi C++ API. Requires a working Gurobi
# installation; set GUROBI_HOME to point at it, e.g.:
#
#   make GUROBI_HOME=/Library/gurobi1103/macos_universal2      (macOS)
#   make GUROBI_HOME=/opt/gurobi1103/linux64                   (Linux)
#
# or export it once for the shell session:
#
#   export GUROBI_HOME=/opt/gurobi1103/linux64
#   make
#
# Targets:
#   make            build the optimized binary (default)
#   make debug      build with -g -O0 (no optimization, for gdb/lldb)
#   make run ARGS="instance_name 1"   build (if needed) and run
#   make clean      remove build artifacts
#   make print-%    debug helper, e.g. `make print-GRB_LIBS`
# ==============================================================================

CXX      := g++
CXXSTD   := -std=c++17
OPT      := -O2
WARN     := -Wall -Wextra

# ---- Gurobi configuration ---------------------------------------------------
# GUROBI_HOME must contain include/ and lib/ (the standard layout of a
# Gurobi install). No default is assumed -- set it explicitly, e.g.:
#   make GUROBI_HOME=/opt/gurobi1201/linux64
# or once per shell session:
#   export GUROBI_HOME=/opt/gurobi1201/linux64


ifndef GUROBI_HOME
$(error GUROBI_HOME is not set. Point it at your Gurobi install, e.g. \
  `make GUROBI_HOME=/opt/gurobi1103/linux64` or `export GUROBI_HOME=...`)
endif

GRB_INC := -I$(GUROBI_HOME)/include
GRB_LIB := -L$(GUROBI_HOME)/lib

# Auto-detect the versioned Gurobi shared library (libgurobiXXX.so[.a.b.c]
# or libgurobiXXX.dylib) so the exact version doesn't need to be hardcoded.
# Uses a shell one-liner (rather than make's own $(wildcard)) so it also
# matches versioned filenames like libgurobi120.so.12.0.1, which is how
# many Linux installs ship the library. Override with GRB_VERLIB=gurobiXXX
# if detection picks the wrong one (e.g. multiple versions installed
# side by side).
GRB_VERLIB ?= $(shell ls $(GUROBI_HOME)/lib/libgurobi[0-9]*.so* \
                          $(GUROBI_HOME)/lib/libgurobi[0-9]*.dylib 2>/dev/null \
                      | head -n1 | xargs -r basename \
                      | sed -E 's/^lib//; s/\.(so|dylib).*$$//')

ifeq ($(strip $(GRB_VERLIB)),)
$(error Could not auto-detect a Gurobi shared library under $(GUROBI_HOME)/lib. \
  Run `ls $(GUROBI_HOME)/lib` to see what is actually there, then set it \
  explicitly, e.g. `make GUROBI_HOME=... GRB_VERLIB=gurobi120`)
endif

# ---- Platform-specific bits --------------------------------------------------
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  SYS_LIBS  :=
  RPATH_FLAG :=
else
  SYS_LIBS   := -lpthread -lm
  # Bake the Gurobi lib dir into the binary's rpath so it's found at
  # runtime without needing to set LD_LIBRARY_PATH.
  RPATH_FLAG := -Wl,-rpath,$(GUROBI_HOME)/lib
endif

GRB_LIBS := -lgurobi_c++ -l$(GRB_VERLIB) $(SYS_LIBS)

# ---- Project files ------------------------------------------------------------
TARGET  := padme
SRC     := main.cpp
HEADERS := Header.h GurobiMPS.h dichotomic_search.hpp BolpDichotomic.h

CXXFLAGS := $(CXXSTD) $(OPT) $(WARN) $(GRB_INC)
LDFLAGS  := $(GRB_LIB) $(RPATH_FLAG)
LDLIBS   := $(GRB_LIBS)

.PHONY: all debug run clean print-%

all: $(TARGET)

# Single translation unit: main.cpp includes Header.h, GurobiMPS.h,
# dichotomic_search.hpp and BolpDichotomic.h directly, so a rebuild is
# triggered whenever any of them changes.
$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ $(LDFLAGS) $(LDLIBS)

debug: CXXFLAGS := $(CXXSTD) -g -O0 -DDEBUG $(WARN) $(GRB_INC)
debug: $(TARGET)

# Usage: make run ARGS="my_instance 1"
run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	rm -f $(TARGET) *.o

# Prints the value of any Makefile variable, e.g. `make print-GRB_LIBS`
print-%:
	@echo '$*=$($*)'
