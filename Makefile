default:
	@echo Gen cells.s
	./gencell.rb ../roms Asteroids cells.s
	make -f Makefile2

# make clean rule
clean:
	rm -f cells.s *.bin *.o *.d *.axf *.lst *.dmp *.nm *.map
