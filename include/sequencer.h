
typedef struct profile_t {
	uint32_t ftw;
	uint16_t pow;
	uint16_t asf;
} profile_t;

typedef struct ram_profile_t {
	uint16_t start;
	uint16_t end;
	uint16_t rate;
	uint8_t mode;
} ram_profile_t;

typedef struct sweep_t {
	uint32_t prologue_hold;
	uint32_t lower_ftw;
	uint32_t upper_ftw;
	uint32_t fstep_ftw;
	uint16_t tstep;
} sweep_t;

typedef struct seq_entry_t {
	uint32_t t1;
	uint32_t t2;
	sweep_t sweep;
	struct {
		uint8_t* buffer;
		size_t size;
		uint32_t tstep;
	} profile_modulation;
	struct {
		uint32_t* buffer;
		size_t size;
	} ram_image;
	uint8_t ram_destination;
	profile_t ram_secondary_params;
	union {
		profile_t profiles[8];
		ram_profile_t ram_profiles[8];
	};
} seq_entry_t;

void enter_rfkill_mode();
void enter_test_tone_mode(uint32_t freq_hz);
void enter_basic_pulse_mode(uint32_t offset_ns, uint32_t duration_ns, uint32_t freq_hz);
void enter_basic_sweep_mode(uint32_t offset_ns, uint32_t duration_ns, uint32_t f1_hz, uint32_t f2_hz);
void sequencer_init();
void sequencer_reset();
void sequencer_show();
void sequencer_add(seq_entry_t entry);
void sequencer_run();
void sequencer_stop();
void spi_write_entry(seq_entry_t entry);
