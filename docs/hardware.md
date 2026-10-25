# Hardware

## The sensor input

The indoor PCB expects the optional external ambient sensor `PAW-A2W-TSOD`
(6.523 kΩ @ 25 °C, B25/50 ≈ 3485 K). The board pulls the sensor node up to Vcc
through a resistor `R_pullup` and measures the resulting divider voltage.

The full vendor characteristic is in `reference/sensor-curves.md`; the relevant
range is:

| T | R | T | R |
|---|---|---|---|
| +40 °C | 3.62 kΩ | 0 °C | 19.70 kΩ |
| +30 °C | 5.33 kΩ | −10 °C | 32.10 kΩ |
| +20 °C | 8.04 kΩ | −20 °C | 53.92 kΩ |
| +10 °C | 12.44 kΩ | −40 °C | 167.82 kΩ |

## Divider maths

To make the board believe the sensor has resistance R, hold the node at

    V = Vcc · R / (R + R_pullup)

A 12-bit DAC output code is therefore

    code = 4095 · R / (R + R_pullup)

because the DAC references its own Vdd and the board's ADC is ratiometric to the
same rail. **Vcc is not in that formula** — power the DAC from the board's 5 V
rail and supply drift drops out. `R_pullup` does not drop out: 1 % error is
0.24 K, 5 % is 1.2 K. Measure it.

### Measuring the pull-up

1. Unplug the sensor from the input. The open node sits at Vcc — measure it.
2. Fit a known resistor `Rn` (10 kΩ, 0.1 %) between signal and ground.
3. Measure the node voltage V and compute `R_pullup = Rn · (Vcc/V − 1)`.

## MCP4726 configuration

| setting | value | why |
|---|---|---|
| Reference | `VREF = VDD` | keeps the ratio ratiometric |
| Gain | 1x | forced to 1x when VDD is the reference |
| Power-down | `PD1:PD0 = 00` | output active |
| EEPROM | save the safe code once, update the volatile register at runtime | write time 25–50 ms, endurance ~100 k cycles |

Do not select the internal band gap (1.22 V; ×2 = 2.44 V full scale). It cannot
reach the 4.38 V the cold end needs, and it destroys the ratiometric
cancellation. The I²C address of the MCP4726 is factory-set — order the correct
suffix (`MCP4726A0`, `A1`, …).

## Output drive

The DAC **sinks** the pull-up current: `Vcc/R_pullup` = 0.5 mA at 0 V, and
0.07…0.37 mA across the used range. The MCP4726 output is specified to sink far
more than that (absolute maximum 40 mA), so no buffer is required for
correctness; a rail-to-rail buffer (MCP6001, TLV9061) only buys back the last
~0.1 K of load regulation.

## Wiring

| signal | note |
|---|---|
| MCP4726 VDD | board 5 V rail (same rail as the pull-up), or isolated DC/DC |
| MCP4726 VSS | board logic ground — single point, no PC/USB ground |
| MCP4726 VOUT | sensor input signal pin, short lead, 100 nF at the DAC |
| I²C | to the ESP32-C6 on the same board; no external cable |
| DS18B20 | 3-wire, shielded, 4.7 kΩ pull-up, IP67 version; no parasitic power |
| Relay (optional) | NC contact returns the input to a real PAW-A2W-TSOD when unpowered |

Keep the DAC and the ESP32-C6 physically close to the indoor PCB. The whole
point of the radio on the same die is that no wire leaves the enclosure, so no
ground loop can form through a USB cable or an instrument.

## Bill of materials

| part | function |
|---|---|
| ESP32-C6 module | Zigbee router, control loop, ADC readback |
| MCP4726A0-E/CH | 12-bit DAC, SOT-23-6 |
| DS18B20 (IP67) | local fallback temperature |
| 100 nF, 4.7 kΩ | DAC decoupling, 1-Wire pull-up |
| B0505S-1W (optional) | isolated 5 V for the emulator |
| 1× changeover relay (optional) | fail-safe back to a real sensor |

## Node voltage verification

Feed the node into one ESP32-C6 ADC channel through a 2:1 divider (12 dB
attenuation, ~0…3.1 V range) and report it back. This is the only real feedback
available: it catches a disconnected lead, a wrong pull-up or a stuck DAC, none
of which the reported setpoint would reveal. What the heat pump computes
internally can only be read on its own remote controller.
