
// Single Tone Profile
// Действует, когда оперативная память выключена
typedef struct profile_t {
	uint32_t ftw;
	uint16_t pow;
	uint16_t asf;
} profile_t;

// RAM Profile
// Действует, когда оперативная память включена
typedef struct ram_profile_t {
	uint16_t start;
	uint16_t end;
	uint16_t rate;
	uint8_t mode;
} ram_profile_t;

// Настройки Digital Ramp Generator
typedef struct sweep_t {
	uint32_t prologue_hold;
	uint32_t lower_ftw;
	uint32_t upper_ftw;
	uint32_t fstep_ftw;
	uint16_t tstep;
} sweep_t;

// Исходные данные для выстраивания произвольных последовательностей логических уровней, см. timer/sequencing.c
// TODO: генерировтать это всё just-in-time чтобы сберечь ОЗУ
typedef struct logic_level_sequence_t {
	struct { 				// Коды принимаемых состояний slave timer A
		uint32_t ab; 		// Пины 1 и 2
		uint32_t cd; 		// Пины 3 и 4
	}* slave_a_stream;
	struct { 				// Коды принимаемых состояний slave timer B
		uint32_t ab; 		// Пины 5 и 6
		uint32_t cd; 		// Пины 7 и 8
	}* slave_b_stream;
	uint16_t* hold_time; 	// Длительность нот в машинных единицах
	size_t count; 			// Количество нот
} logic_level_sequence_t;

// Универсальная структура данных для описания импульса
// Pulse Descriptor, иначе
typedef struct seq_entry_t {
	sweep_t sweep;
	logic_level_sequence_t logic_level_sequence;
	struct {
		uint32_t* buffer;
		size_t size;
	} ram_image;
	uint8_t fsc;
	uint8_t ram_destination;
	profile_t ram_secondary_params;
	union {
		profile_t profiles[8];
		ram_profile_t ram_profiles[8];
	};
} seq_entry_t;

void enter_rfkill_mode();
void enter_test_tone_mode(uint32_t freq_hz);
void sequencer_init();
void sequencer_reset();
void sequencer_show();
void sequencer_add(seq_entry_t entry);
void sequencer_run();
void sequencer_stop();
void spi_write_entry(seq_entry_t entry);
