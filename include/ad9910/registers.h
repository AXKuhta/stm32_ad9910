
extern uint8_t r00[4]; // CFR1
extern uint8_t r01[4]; // CFR2
extern uint8_t r02[4]; // CFR3
extern uint8_t r03[4]; // Aux DAC control
extern uint8_t r04[4]; // I/O Update Rate
extern uint8_t r05[6]; // ???
extern uint8_t r06[6]; // ???
extern uint8_t r07[4]; // FTW
extern uint8_t r08[2]; // POW
extern uint8_t r09[4]; // ASF
extern uint8_t r0A[4]; // Multichip Sync
extern uint8_t r0B[8]; // Digital Ramp Limit
extern uint8_t r0C[8]; // Digital Ramp Step
extern uint8_t r0D[4]; // Digital Ramp Rate
extern uint8_t r0E[8]; // Profile 0
extern uint8_t r0F[8]; // Profile 1
extern uint8_t r10[8]; // Profile 2
extern uint8_t r11[8]; // Profile 3
extern uint8_t r12[8]; // Profile 4
extern uint8_t r13[8]; // Profile 5
extern uint8_t r14[8]; // Profile 6
extern uint8_t r15[8]; // Profile 7
extern uint8_t r16[4]; // RAM


void ad_write(uint8_t reg_addr, uint8_t* buffer, uint16_t size);
void ad_readback_all();
void ad_write_all();

