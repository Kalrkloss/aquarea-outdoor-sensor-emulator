# Commissioning

## Before you start

- Heat pump switched off at the breaker while you work on the indoor PCB.
- Pull-up measured (`hardware.md`), not assumed.
- Real external sensor (`PAW-A2W-TSOD`) available if you want the fail-safe relay.
- A thermometer you trust for the two-point calibration.

## Steps

1. **Measure R_pullup and Vcc** at the sensor input. Write both into the firmware
   configuration.
2. **Enable the input.** `Installer → System setup → step 10 "Alternative outdoor
   sensor" = Yes`. The indoor unit then controls on the alternative value; the
   built-in sensor stays installed and keeps serving the outdoor unit's defrost
   and fault logic.
3. **Wire and power up** with the DAC at the safe default (−5 °C, code 2927).
   Confirm the remote controller shows a plausible outdoor temperature.
4. **Verify the divider.** Read back the node voltage with the ESP32-C6 ADC and
   compare it with `Vcc · R/(R+R_pullup)`. A mismatch here is a wiring or
   pull-up error, and no amount of calibration will hide it.
5. **Two-point calibration.** Set the code for 0 °C and for +20 °C, read the
   temperature the remote controller displays, and solve
   `R_true = ntc_ohm_for_temp(T_displayed)` for both points. The two residuals
   give the effective `R_pullup` and the DAC gain error. Iterate once.
6. **Enable Zigbee.** Join the network, confirm the sensor value and the writable
   setpoint appear, and that a write reaches the DAC within a second.
7. **Test the fallbacks.** Pull the Zigbee stick (DS18B20 takes over after
   15 min), then power-cycle the C6 (value must come back from NVS), then cut the
   emulator's supply (relay must return the real sensor).

## Rollback

Set `Alternative outdoor sensor = No`. The indoor unit returns to the built-in
sensor; the emulator becomes irrelevant and can be removed.

## What to log

Applied temperature, winning source, node voltage, and per-sensor ages at one
minute resolution for the first two weeks. Correlate with flow temperature and
compressor power before trusting the aggregate — a systematically low aggregate
shows up as an unexplained COP drop, not as a fault.
