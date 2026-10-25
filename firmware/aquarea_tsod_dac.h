/* aquarea_tsod_dac.h -- Panasonic Aquarea external outdoor sensor (PAW-A2W-TSOD),
 * emulated by DRIVING THE DIVIDER NODE with a DAC instead of presenting a resistance.
 *
 * Sensor : NTC 6.523 kOhm @ 25 C, B25/50 ~ 3485 K (PAW-A2W-TSOD / -TSRT / -TSSO)
 * Output : 12 bit. MCP4726 (recommended) or MCP4725. 10 bit (MCP4716) also fine:
 *          worst error 0.08 K.
 *
 * TOPOLOGY: the board pulls the sensor node to Vcc through R_PULLUP; the DAC drives
 * the same node to V = Vcc * R / (R + R_PULLUP). The DAC references its own Vdd and
 * the board's ADC is ratiometric to the same rail, so the code is a pure ratio and
 * Vcc cancels:
 *
 *     code = 4095 * R / (R + R_PULLUP)
 *
 * -> Power the DAC from the board's 5 V rail (the one feeding the pull-up) and Vcc
 *    drift drops out. R_PULLUP does NOT cancel: 1 % error = 0.24 K, 5 % = 1.2 K.
 *    Measure it (see below) and set AQUAREA_R_PULLUP_OHM.
 *
 * The DAC must SINK the pull-up current (Vcc/R_PULLUP = 0.5 mA at V_dac = 0,
 * 0.07..0.37 mA inside the used range). A rail-to-rail buffer (MCP6001, TLV9061)
 * keeps the error below 0.05 K; without one, count on a few mV = <=0.1 K.
 *
 * MCP4726 configuration (do not deviate - the Vcc cancellation depends on it):
 *   - VREF1:VREF0 = VDD (the internal band gap is 1.22 V; x2 gives 2.44 V full
 *     scale, which cannot reach the 4.38 V the cold end needs, and it destroys
 *     the ratiometric cancellation because the code would no longer be a ratio).
 *   - Gain G = 1x, PD1:PD0 = 00 (normal). With VDD as reference the gain bit is
 *     forced to 1x anyway.
 *   - EEPROM: the MCP4726 stores DAC register AND configuration (VREF, gain,
 *     power-down) and recalls both on power-up. That is the feature to exploit:
 *     save the "safe" code once (RAMB in the cold season), then update only the
 *     volatile register. EEPROM write takes 25..50 ms and blocks, and endurance
 *     is ~100k cycles - never write EEPROM per setpoint change.
 *   - I2C address is factory-set (MCP4726A0/A1/...), not strapped - order the
 *     suffix you need.
 *   - VOUT sinks (40 mA abs max), so the 0.5 mA pull-up current is a non-issue;
 *     the buffer is only about the last 0.1 K of load regulation.
 *
 * Failure modes:
 *   - controller dead, 5 V alive -> MCP4726 auto-recall keeps the node at the
 *     saved code, the board keeps reading a plausible temperature.
 *   - your 5 V supply also gone -> output high-Z -> pull-up takes the node to Vcc
 *     -> the board reads "sensor open". A digipot keeps its stored wiper setting
 *     and keeps reading a valid value in that case.
 *
 * Wiring: same isolation rules as the digipot - never tie PC/USB ground to the
 * heat pump's logic ground; use an I2C isolator (ADuM1250) or a battery-powered MCU.
 */
#ifndef AQUAREA_TSOD_DAC_H
#define AQUAREA_TSOD_DAC_H

#include <math.h>

#define AQUAREA_R_PULLUP_OHM  10000.0f   /* measure, do not assume */
#define AQUAREA_DAC_FS        4095

#define AQUAREA_LUT_MIN_X10   (-250)     /* -25.0 C */
#define AQUAREA_LUT_STEP_X10  (5)        /* 0.5 K   */
#define AQUAREA_LUT_LEN       131
#define AQUAREA_NODES          15

/* vendor characteristic, -40..+30 C */
static const short aquarea_node_t_c[AQUAREA_NODES]   = {-40,-35,-30,-25,-20,-15,-10,-5,0,5,10,15,20,25,30};
static const long  aquarea_node_r_ohm[AQUAREA_NODES] = {167820,124240,93050,70530,53920,41450,32100,25050,19700,15604,12443,9980,8044,6523,5326};

/* 12-bit codes, -25.0 .. +40.0 C in 0.5 K steps, R_PULLUP = 10 kOhm.
 * Worst quantisation error 0.02 K, worst step 0.04 K (+20 / +35 C). */
static const unsigned short aquarea_dac_lut[AQUAREA_LUT_LEN] = {
    3586, 3574, 3562, 3549, 3536, 3523, 3510, 3496, 3482, 3468, 3454, 3440, 3425, 3410, 3395, 3379, 3364, 3348, 3332, 3316, 3299, 3282, 3265, 3248, 3231, 3213, 3195, 3177, 3159, 3141, 3122, 3103, 3084, 3065, 3046, 3026, 3007, 2987, 2967, 2947, 2927, 2906, 2886, 2865, 2844, 2823, 2802, 2781, 2759, 2738, 2716, 2694, 2673, 2651, 2629, 2607, 2585, 2562, 2540, 2518, 2496, 2473, 2451, 2428, 2406, 2383, 2361, 2338, 2315, 2293, 2270, 2248, 2225, 2202, 2180, 2157, 2135, 2112, 2090, 2068, 2045, 2023, 2001, 1978, 1956, 1934, 1912, 1890, 1869, 1847, 1826, 1804, 1783, 1761, 1740, 1719, 1699, 1678, 1657, 1637, 1617, 1597, 1577, 1557, 1537, 1518, 1499, 1479, 1460, 1442, 1423, 1405, 1386, 1368, 1350, 1333, 1315, 1298, 1281, 1264, 1247, 1230, 1214, 1197, 1181, 1165, 1150, 1134, 1119, 1103, 1088
};

static inline float aquarea_ohm_for_temp(float t_c)
{
    const float k = 273.15f;
    int i;
    if (t_c <= (float)aquarea_node_t_c[0])
    {
        float b = logf((float)aquarea_node_r_ohm[1] / (float)aquarea_node_r_ohm[0]) /
                  (1.0f / ((float)aquarea_node_t_c[1] + k) - 1.0f / ((float)aquarea_node_t_c[0] + k));
        return (float)aquarea_node_r_ohm[0] *
               expf(b * (1.0f / (t_c + k) - 1.0f / ((float)aquarea_node_t_c[0] + k)));
    }
    if (t_c >= (float)aquarea_node_t_c[AQUAREA_NODES - 1])
    {
        float b = logf((float)aquarea_node_r_ohm[AQUAREA_NODES - 1] / (float)aquarea_node_r_ohm[AQUAREA_NODES - 2]) /
                  (1.0f / ((float)aquarea_node_t_c[AQUAREA_NODES - 1] + k) -
                   1.0f / ((float)aquarea_node_t_c[AQUAREA_NODES - 2] + k));
        return (float)aquarea_node_r_ohm[AQUAREA_NODES - 1] *
               expf(b * (1.0f / (t_c + k) - 1.0f / ((float)aquarea_node_t_c[AQUAREA_NODES - 1] + k)));
    }
    for (i = 0; i < AQUAREA_NODES - 1; i++)
    {
        if (t_c <= (float)aquarea_node_t_c[i + 1])
        {
            float f = (1.0f / (t_c + k) - 1.0f / ((float)aquarea_node_t_c[i] + k)) /
                      (1.0f / ((float)aquarea_node_t_c[i + 1] + k) - 1.0f / ((float)aquarea_node_t_c[i] + k));
            return expf(logf((float)aquarea_node_r_ohm[i]) +
                        f * (logf((float)aquarea_node_r_ohm[i + 1]) - logf((float)aquarea_node_r_ohm[i])));
        }
    }
    return (float)aquarea_node_r_ohm[AQUAREA_NODES - 1];
}

/* resistance -> DAC code (the whole point: no Vcc in here) */
static inline unsigned short aquarea_code_for_ohm(float ohm)
{
    float c = (float)AQUAREA_DAC_FS * ohm / (ohm + AQUAREA_R_PULLUP_OHM);
    if (c < 0.0f) c = 0.0f;
    if (c > (float)AQUAREA_DAC_FS) c = (float)AQUAREA_DAC_FS;
    return (unsigned short)(c + 0.5f);
}

/* temperature in 0.1 K -> DAC code (LUT + log-linear interpolation in R) */
static inline unsigned short aquarea_code_for_temp_x10(short t_x10)
{
    int idx, t_lo;
    float f, r_lo, r_hi;
    /* outside the LUT: extrapolate with the analytic curve instead of clamping,
     * so -35 C or +45 C do not silently return the end value */
    if (t_x10 <= AQUAREA_LUT_MIN_X10 ||
        t_x10 >= AQUAREA_LUT_MIN_X10 + AQUAREA_LUT_STEP_X10 * (AQUAREA_LUT_LEN - 1))
        return aquarea_code_for_ohm(aquarea_ohm_for_temp((float)t_x10 / 10.0f));
    idx = (t_x10 - AQUAREA_LUT_MIN_X10) / AQUAREA_LUT_STEP_X10;
    f = (float)(t_x10 - (AQUAREA_LUT_MIN_X10 + idx * AQUAREA_LUT_STEP_X10)) / (float)AQUAREA_LUT_STEP_X10;
    t_lo = AQUAREA_LUT_MIN_X10 + idx * AQUAREA_LUT_STEP_X10;
    r_lo = aquarea_ohm_for_temp((float)t_lo / 10.0f);
    r_hi = aquarea_ohm_for_temp((float)(t_lo + AQUAREA_LUT_STEP_X10) / 10.0f);
    return aquarea_code_for_ohm(expf(logf(r_lo) + f * (logf(r_hi) - logf(r_lo))));
}

#endif /* AQUAREA_TSOD_DAC_H */
