
typedef struct ad_profile_set_t {
    uint32_t freq_hz[8];
} ad_profile_set_t;

typedef struct seq_entry_t {
    struct sweep_t {
        uint32_t prologue_hold;
        uint32_t f1;
        uint32_t f2;
        uint32_t step;
    } sweep;
    uint32_t t1;
    uint32_t t2;
    ad_profile_set_t profiles;
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
