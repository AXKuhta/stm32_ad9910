#include <stdint.h>
#include <stdlib.h>
//#include <readline/readline.h>

#include "sequencer.h"
#include "uart_cli.h"

char* readline(const char* prompt);
void using_history();
void add_history (const char *string);

extern uint32_t ad_system_clock;

int scan_and_run() {
	char* cmd = readline("> ");

	if (cmd) {
		add_history(cmd);
		run(cmd);
		free(cmd);
	} else {
		exit(0);
	}
}

int main() {
    ad_system_clock = 1000*1000*1000;

    using_history();
    sequencer_init();

    while (1)
        scan_and_run();
}

void profile_to_gpio_states() {}
void enter_rfkill_mode() {}
void enter_test_tone_mode(uint32_t freq_hz) {}
void enter_basic_pulse_mode(uint32_t offset_ns, uint32_t duration_ns, uint32_t freq_hz) {}
void enter_basic_sweep_mode(uint32_t offset_ns, uint32_t duration_ns, uint32_t f1_hz, uint32_t f2_hz) {}
void ad_write_all() {}
void ad_readback_all() {}
void ad_ram_test() {}
void print_it() {}
void sequencer_run() {}
void sequencer_stop() {}
