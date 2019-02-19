
include sources

#NOTE: I'm going to need to remember the root directory from which to build
#		the output directory.  Directory structure must be maintained
#		in a separate output directory to compensate for files
#		with identical filenames in different source directories


INCLUDEDIRS=$(patsubst %,-I$(root)%,$(includes)) 


relativesources=$(patsubst %,$(root)%,$(sources))

objects=$(patsubst %.cpp,%.o,$(patsubst %.a,%.o,$(relativesources)))


#FLAGS= -fzero-initialized-in-bss -march=amdfam10 -fstrict-aliasing -Wstrict-aliasing=3 -mpopcnt -msse4.2
#FLAGS= -fzero-initialized-in-bss -march=i686 -fstrict-aliasing -Wstrict-aliasing=3 -mpopcnt -msse4.2 -O3
# -g -O0 -fno-inline
FLAGS=-fmax-errors=1 -std=c++17 -fzero-initialized-in-bss -march=native -fstrict-aliasing -Wstrict-aliasing=3 -mpopcnt -msse4.2 -O3
DEBUGFLAGS=-g
RELEASEFLAGS=

CC=g++ -v

all: $(target)

$(target): $(objects)
	$(CC) -lrt -lpthread  $(objects) -o cogs

.PHONY : clean
clean:
	-rm -f $(objects) $(relativesources:.cpp=.d)


%.o: %.cpp
	$(CC) $(FLAGS) -O3 -c $(INCLUDEDIRS) -MD -o $@ $<
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp


-include $(relativesources:.cpp=.d)


#%.o: %.cpp %.d
#	$(CC) -c $(INCLUDEDIRS) -o $@ $<
