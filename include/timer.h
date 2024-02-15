
#define M_216MHz (216.0 * 1000.0 * 1000.0)
#define NS_TO_216MHZ_MU (0.216)

#define MAX_NS_16BIT_216MHz ((1 << 16) - 1) / NS_TO_216MHZ_MU

#define timer_mu(time_ns) (uint32_t)((time_ns) * NS_TO_216MHZ_MU + 0.5)
#define timer_ns(time_mu) ((double)(time_mu) / NS_TO_216MHZ_MU)
