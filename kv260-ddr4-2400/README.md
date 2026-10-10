# DDR4-2400 on the KV260

The Vivado board preset for the AMD Kria KV260 (`kv260_som`) configures the PS DDR4 as DDR4-2400. But the DDR actually runs at 2133 MT/s. This folder has a short Vivado Tcl script that makes it run at 2400.

## Why it runs at 2133

The preset also clocks the FPD main switch (TOPSW_MAIN, 533.33 MHz) from the DDR PLL. Vivado then picks a DDR PLL frequency that serves both: 1066.7 MHz, which means DDR4-2133. Clocking the switch from the video PLL instead frees the DDR PLL for 1200 MHz, which is DDR4-2400. At 2400, the preset's CAS write latency of 14 (a DDR4-2133 value) isn't allowed; 12 is.

## Apply it

Open a block design whose Zynq UltraScale+ PS has the `kv260_som` board preset applied, then in the Vivado Tcl console:

```tcl
source ddr4-2400.tcl
```

The script changes two settings (TOPSW_MAIN to the video PLL, CAS write latency 12) and prints the resulting values.

## Check it

- **In Vivado:** the script's output, or the PS customization dialog (Clock Configuration → Output Clocks → Full Power Domain), where DDR shows 600 MHz and TOPSW_MAIN its source VPLL. The DDR controller clock is half the DRAM clock: 600 MHz is DDR4-2400; 533.333 MHz would be DDR4-2133.
- **On the board:** CRF_APB DPLL_CTRL (0xFD1A002C), FBDIV in bits 14:8, reads 72 at 2400 and 64 at 2133. After training, the DDR PHY's PGSR0 (0xFD080030) read 0x80004FFF on ours: done, no error flags.

## What changes, as tested

Vivado 2026.1, zynq_ultra_ps_e 3.5, board part `xilinx.com:kv260_som:part0:2.0`, part `xck26-sfvc784-2LV-c`:

| Setting | Board preset | After the script |
|---|---|---|
| DDR controller clock | 533.328 MHz (DDR4-2133) | 599.994 MHz (DDR4-2400) |
| CAS write latency | 14 | 12 |
| TOPSW_MAIN source | DPLL | VPLL (Vivado keeps it at 533.3 MHz by retuning the VPLL) |
| GDMA | 533.3 MHz, DDR PLL | 600 MHz, DDR PLL |
| DP_VIDEO_REF | DPLL, 320 MHz | unchanged |

The design validates without errors.

## What we measured

On one KV260, the DDR was re-initialised over JTAG with these DDR4-2400 settings: training was clean, a memory pattern test passed, and our FPGA graphics tests ran with every checked frame exact (1,801 of 1,801 in two 30-second runs). Our own graphics workloads got no faster, because they are limited by our engine, not by the DDR.

We did not measure raw DDR bandwidth. In theory, the peak of the K26's 64-bit DDR4 rises from 17.1 GB/s at 2133 to 19.2 GB/s at 2400, 12.5 % more.

## Caveats

1. **The boot firmware sets up the DDR.** On the KV260 that is the firmware in QSPI, so changing the Vivado design alone doesn't change a running board. The firmware (psu_init in the boot image) must be rebuilt from the changed design. We tested by re-initialising over JTAG, not with a flashed boot image. Flashing boot firmware is at your own risk.
2. **One board, one session:** no temperature or long-term qualification. Whether DDR4-2400 is fine for production use on the K26 is a question for AMD; we will add their answer here.
3. **Other clocks move:** TOPSW_MAIN moves to the video PLL, and GDMA to 600 MHz. If your design uses the video PLL, for example for a DisplayPort video clock, check its clocks after the change.

## Licence

MIT, see [LICENSE](LICENSE). The script comes without any warranty.
