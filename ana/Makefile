ROOTCINT      = $(ROOTSYS)/bin/rootcint
ROOTCFLAGS    = $(shell $(ROOTSYS)/bin/root-config --cflags)
ROOTLIBS      = $(shell $(ROOTSYS)/bin/root-config --libs)
ROOTGLIBS     = $(shell $(ROOTSYS)/bin/root-config --glibs)

CXX           = g++
CXXFLAGS      = $(ROOTCFLAGS) -g -Wall -fPIC

LD            = g++
LDFLAGS       = -g
SOFLAGS       = -shared

GLIBS         = $(filter-out -lz, $(ROOTGLIBS))
GLIBS         += -lMinuit

# -- Default rules
$(addprefix obj/,%.o) : %.cc
	$(CXX) $(CXXFLAGS) -I../core/utils/ -I../util -c $< -o $@

%Dict.cc : %.hh %LinkDef.h
	$(ROOTCINT) -f $@ -c $^
	mv $(subst Dict.cc,Dict_rdict.pcm,$@) ../lib

%Dict.cc : %.hh
	$(ROOTCINT) -f $@ -c $<
	mv $(subst Dict.cc,Dict_rdict.pcm,$@) ../lib

# -- Source code
ANA = anaPHValidation.o \
      anaFullTest.o \
      anaGainPedestal.o \
      anaScurve.o

DICTFILES = ${ANA:.o=Dict.o}

# ================================================================================
ana: $(addprefix obj/,$(ANA) $(DICTFILES))
# ---------------------------------------
	$(CXX) -I ../util $(SOFLAGS) $(GLIBS) $(addprefix obj/,$(ANA) $(DICTFILES)) ../lib/libpxarutil.so -o ../lib/libana.so

# ================================================================================
#runPhFit: obj/runPhFit.o obj/aPhFit.o obj/aPhFitDict.o
# ----------------------------------------------------
#	$(LD) $(LDFLAGS)  -o ../bin/runPhFit obj/runPhFit.o $(GLIBS)

# ================================================================================
all:
# --
	@$(MAKE) cleanall
	@$(MAKE) links
	@$(MAKE) ana

cleanall:
	@$(MAKE) clean
	rm -f ../lib/libana.so

clean:
	rm -f $(addprefix obj/,$(ANA)) $(addprefix obj/,$(DICTFILES))

links:
	cd ../lib && ln -s libpxarutil.dylib libpxarutil.so && cd -
