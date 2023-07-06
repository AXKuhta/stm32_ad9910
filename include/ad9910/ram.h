
// RAM
#define AD_RAM_DESTINATION_FREQ 		0
#define AD_RAM_DESTINATION_PHASE 		1 // Не будет работать, пока выставлен бит Enable amplitude scale from single tone profiles (так как амплитуда будет нулевая)
#define AD_RAM_DESTINATION_AMPLITUDE 	2
#define AD_RAM_DESTINATION_POLAR 		3

#define AD_RAM_PROFILE_MODE_DIRECTSWITCH	0b00000000
#define AD_RAM_PROFILE_MODE_ZEROCROSSING	0b00001000 // Если включен zero-crossing, то режим обязательно direct switch
#define AD_RAM_PROFILE_MODE_RAMPUP			0b00000001

void ad_set_ram_destination(uint8_t destination);
void ad_write_ram(uint32_t* buffer, size_t count);
void ad_read_ram(uint32_t* buffer, size_t count);
void ad_ram_test();
