# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Sergej Klementinovski
#
# KV260 (K26 SOM): run the PS DDR4 at DDR4-2400, the rate the board preset asks for, instead of DDR4-2133.
#
# The kv260_som board preset sets DDR4_2400R, but it also clocks the FPD main switch (TOPSW_MAIN, 533.33 MHz) from
# the DDR PLL.  Vivado then picks a DDR PLL frequency that serves both: 1066.7 MHz, which is DDR4-2133.  Clocking the
# switch from the video PLL instead frees the DDR PLL for 1200 MHz (DDR4-2400).  At 2400 the preset's CAS write
# latency 14 (a DDR4-2133 value) is not allowed; 12 is.
#
# Use on a block design whose Zynq UltraScale+ PS has the kv260_som board preset applied.
set ps [get_bd_cells -hierarchical -filter {VLNV =~ "xilinx.com:ip:zynq_ultra_ps_e:*"}]
set_property -dict [list \
    CONFIG.PSU__CRF_APB__TOPSW_MAIN_CTRL__SRCSEL {VPLL} \
    CONFIG.PSU__DDRC__CWL {12} \
] $ps

# Check: the DDR controller clock (half the DRAM clock) 600 MHz is DDR4-2400; 533.333 MHz would be DDR4-2133.
foreach p {DDRC__SPEED_BIN DDRC__CWL CRF_APB__DDR_CTRL__SRCSEL CRF_APB__DDR_CTRL__ACT_FREQMHZ
           CRF_APB__TOPSW_MAIN_CTRL__SRCSEL CRF_APB__TOPSW_MAIN_CTRL__ACT_FREQMHZ} {
    puts [format "%-40s %s" $p [get_property CONFIG.PSU__$p $ps]]
}
