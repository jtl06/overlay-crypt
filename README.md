# Code Overlay on MCU
This project evaluates code overlays on a STM32G031K8 (Cortex M0+) MCU. Code is overlaid from flash to a specific slice of SRAM, increasing performance for multiple applications while maintaining memory footprint.

## Overview
The current overlays implemented are:
- RSA-2048 verification (`.ovl_rsa`)
- Miller-Rabin primality test performed on 2<sup>127</sup> - 1 (`.ovl_prime`)

Code is initially stored in flash at seperate addresses, but are copied into the SRAM overlay window immediately prior to execution.

See `benchmark.txt` for program output.

## Performance

| Program | w/o Overlay (us) | w/ Overlay (us) | Speedup |
|---|---|---|---|
| RSA2048 Verify | 383,256 | 285,862 | 1.34x |
| MR Primality | 3,679| 3,365 | 1.09x |

## Analysis
Benefits include:
- Performance improvements by executing code from SRAM instead of flash
- Efficient use of limited memory footprint for hardware without a MMU
- Deterministic behavior compared to memory paging and other MMU operations 

Drawbacks include:
- Exclusivity of the overlay window: only one program may reside in the overlay at any time.
- Increased complexity due to manual overlay management, requiring explicit runtime copy and execute logic.

## Applications
Code overlays are best for programs which are infrequently invoked, but have significant internal code reuse. Cryptographic functions, compression algorithms, and DSP are examples of intensive, bursty operations that benefit from being copied into SRAM for execution while keeping steady state memory usage low. 

Overlays also allow for higher performance for code that is too large to fit in memory.

On more complex systems with a MMU, overlays can be used to provide deterministic behavior for sensitive tasks.

## Platform
- **MCU / Board:** NUCLEO-G031K8 (STM32G031K8, 64 KB FLASH, 8 KB SRAM)  
- **Core:** ARM Cortex-M0+  
- **Toolchain:** `gcc-arm-none-eabi`  
- **Debug / Flash:** OpenOCD + ST-Link  
- **IDE:** VS Code + `cortex-debug` (gdb)

## Running the Demo
To compile the code and flash it, connect the NUCLEO-G031K8 via an OpenOCD-supported tool such as ST-Link and run:

```bash
make flash
```
Output is returned over UART2 at baud `115200`.

If you would like to run the code without overlays, see the branch at https://github.com/jtl06/overlay-crypt/tree/noverlay.

## Implementation Notes
Overlay functions are placed into `.ovl_*` sections via `__attribute__((section(".ovl_xxx")))` and copied into a shared SRAM block by a small runtime (`static void overlay_load(const uint8_t* lma_start, const uint8_t* lma_end)`)
