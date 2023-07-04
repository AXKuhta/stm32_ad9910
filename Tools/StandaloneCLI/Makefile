FLAGS = -O2 -D"STANDALONE_CLI_APP" -I"../../include/"
OBJS = sequencer.o uart_cli.o units.o ad9910/units.o performance.o stubs.o

standalone_cli.elf: $(OBJS)
	@echo " [LD]" $@
	@$(CC) $(FLAGS) $^ -o $@ -lreadline

%.o: %.c
	@echo " [CC]" $<
	@$(CC) $(FLAGS) -c -o $@ $<

%.o: ../../%.c
	@echo " [CC]" $<
	@$(CC) $(FLAGS) -c -o $@ $<

ad9910/%.o: ../../ad9910/%.c
	@echo " [CC]" $<
	@$(CC) $(FLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) *.elf
