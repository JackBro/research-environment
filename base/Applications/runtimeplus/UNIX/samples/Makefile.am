
TARGETS = \
	basic.exe \
	advanced.exe

EXTRA_DIST = \
	BasicSample.cs \
	AdvancedSample.cs

DEBUGS = $(addsuffix .mdb, $(TARGETS))

assembly=$(ASSEMBLY_NAME).dll

CLEANFILES = $(TARGETS) $(DEBUGS) $(assembly)

references=-r:../sources/$(assembly) -pkg:gtk-sharp-2.0

all: $(TARGETS)

$(assembly): ../sources/$(assembly)
	cp ../sources/$(ASSEMBLY_NAME).dll $(srcdir)

basic.exe: $(assembly) $(srcdir)/BasicSample.cs
	$(CSC)  /out:$@ $(references) $(srcdir)/BasicSample.cs

advanced.exe: $(assembly) $(srcdir)/AdvancedSample.cs
	$(CSC)  /out:$@ $(references) $(srcdir)/AdvancedSample.cs

