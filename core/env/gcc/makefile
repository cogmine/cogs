
include sources

#NOTE: I'm going to need to remember the root directory from which to build
#		the output directory.  Directory structure must be maintained
#		in a separate output directory to compensate for files
#		with identical filenames in different source directories


INCLUDEDIRS=$(patsubst %,-I$(root)%,$(includes)) 


relativesources=$(patsubst %,$(root)%,$(sources))

objects=$(patsubst %.cpp,%.o,$(patsubst %.a,%.o,$(relativesources)))


# -g -O0 -fno-inline
FLAGS=-mcx16 -fmax-errors=1 -std=gnu++17 -fzero-initialized-in-bss -march=native -fstrict-aliasing -Wstrict-aliasing=3 -mpopcnt -msse4.2 -Winit-self -Wformat-nonliteral -Wpointer-arith -fno-exceptions -D COGS_DEBUG -g
#-O3

CC=g++ -v

all: $(target)

$(target): $(objects)
	$(CC) -lrt -pthread -latomic $(objects) -o cogs

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
