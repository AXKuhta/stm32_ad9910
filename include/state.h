
typedef enum {
	TRIG_RISE,
	TRIG_FALL
} trigger_mode_t;

typedef struct {
	// Используемое по умолчанию значение амплитуды (ASF)
	// Максимальным допустимым значением является 0x3FFF
	uint16_t asf;

	// Используемое по умолчанию значение тока ЦАПа (FSC)
	uint8_t fsc;

	// Полярность триггера (TRIG_RISE/TRIG_FALL)
	trigger_mode_t trigger;
} state_t;

extern state_t state;
