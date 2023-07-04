
void ad_set_ram_freq(uint32_t ftw);
void ad_set_ram_phase(uint16_t pow);
void ad_set_ram_amplitude(uint16_t asf);
void ad_init();

#include "ad9910/registers.h"
#include "ad9910/profiles.h"
#include "ad9910/pins.h"
#include "ad9910/drg.h"
#include "ad9910/units.h"
#include "ad9910/ram.h"
#include "ad9910/misc.h"
