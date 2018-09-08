ASSEMBLE=../glulx-assemble

all: minimal.ulx basic.ulx complex.ulx

minimal.ulx: minimal.ga $(ASSEMBLE)
	$(ASSEMBLE) minimal.ga minimal.ulx

basic.ulx: basic.ga $(ASSEMBLE)
	$(ASSEMBLE) basic.ga basic.ulx

complex.ulx: complex.ga glk.ga $(ASSEMBLE)
	$(ASSEMBLE) complex.ga complex.ulx

clean:
	$(RM) *.ulx out_*.txt

.PHONY: all clean
