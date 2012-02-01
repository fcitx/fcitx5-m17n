clean:
	rm -f *.o $(INTERFILES)

nuke: clean
	rm -f $(OUTFILES)

.PHONY: all clean nuke install

.DEFAULT_GOAL := all
