# GNSS Airborne startup configuration

The STM32 applies `CFG-NAVSPG-DYNMODEL = 6` (Airborne <1g) in RAM on each boot using UBX-CFG-VALSET. It then polls UBX-CFG-VALGET for the active RAM value. A checksum-valid response containing the requested key and value is required for success; an I2C ACK alone is insufficient. Three attempts are allowed, with a one-second response window each. The shared I2C mutex is released between operations and during RTOS delays. If configuration cannot be verified, acquisition continues with an explicit console warning.

The verifier skips unrelated UBX messages (including ACK/NAK), accepts fragmented responses, and checks both checksum bytes, response version, layer, position, key and value. Startup stream bytes are consumed before normal NMEA parsing resumes. No telemetry layout changes are made.

References:
- https://cdn.sparkfun.com/assets/5/c/a/0/b/MAX-M10S_IntegrationManual_UBX-20053088.pdf (I2C write access)
- https://www.u-blox.com/docs/UBX-20053845 (M10 SPG 5.00 interface)
- https://content.u-blox.com/sites/default/files/documents/u-blox-M10-SPG-5.20_InterfaceDescription_UBXDOC-304424225-20128.pdf (VALSET/VALGET and dynamic model key)

Run host tests from the repository root:

```sh
gcc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -include tests/gnss/hal_stub.h -ICore/Inc \
  Core/Src/gnss_i2c.c tests/gnss/test_gnss.c -o /tmp/bless_gnss_test
ASAN_OPTIONS=detect_leaks=0 /tmp/bless_gnss_test
```

The tests mock HAL; they do not demonstrate hardware acceptance. Before flight:
1. Build with STM32CubeIDE / the ARM toolchain and flash the payload.
2. Confirm `GNSS Airborne <1g verified` at startup.
3. Confirm valid GNSS coordinates/altitude and normal sensor, RF and E-Link telemetry.
4. Power-cycle and repeat; the RAM configuration must be reapplied.
5. Test a disconnected GNSS and check that other acquisition tasks continue.

A GNSS-only reset after startup is outside this change: RAM configuration is lost and requires reconfiguration or an MCU restart. There is no periodic readback during nominal acquisition.
