FLAGS = -O2 -D"STANDALONE_CLI_APP" -I"../../include/"

standalone_cli.elf: sequencer.o uart_cli.o units.o ad9910.o performance.o stubs.o
	@echo " [LD]" $@
	@$(CC) $(FLAGS) $^ -o $@ -lreadline

%.o: %.c
	@echo " [CC]" $<
	@$(CC) $(FLAGS) -c -o $@ $<

%.o: ../../%.c
	@echo " [CC]" $<
	@$(CC) $(FLAGS) -c -o $@ $<

clean:
	rm -f *.o *.elf
