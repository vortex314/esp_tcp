/* This linker script generated from xt-genldscripts.tpp for LSP . */
/* Linker Script for ld -N */
MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
/*  irom0_0_seg :                         org = 0x40240000, len = 0x3C000 */
  irom0_0_seg :                       	org =  0x40202010, len = 0x7C000 
}

PHDRS
{
  dport0_0_phdr PT_LOAD;
  dram0_0_phdr PT_LOAD;
  dram0_0_bss_phdr PT_LOAD;
  iram1_0_phdr PT_LOAD;
  irom0_0_phdr PT_LOAD;
}
INCLUDE "../ld/eagle.app.v6.common.ld"
/* get ROM code address */
/* INCLUDE "../ld/eagle.rom.addr.v6.ld" */
