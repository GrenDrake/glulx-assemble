ASSEMBLE=../glulx-assemble

all: minimal.ulx basic.ulx complex.ulx model.ulx mountain.ulx

minimal.ulx: minimal.ga $(ASSEMBLE)
	$(ASSEMBLE) minimal.ga minimal.ulx

basic.ulx: basic.ga $(ASSEMBLE)
	$(ASSEMBLE) basic.ga basic.ulx

complex.ulx: complex.ga glk.ga $(ASSEMBLE)
	$(ASSEMBLE) complex.ga complex.ulx

model.ulx: model.ga glk.ga $(ASSEMBLE)
	$(ASSEMBLE) model.ga model.ulx

mountain.ulx: mountain.ga gamesys.ga glk.ga $(ASSEMBLE)
	$(ASSEMBLE) mountain.ga mountain.ulx

clean:
	$(RM) *.ulx out_*.txt

.PHONY: all clean
