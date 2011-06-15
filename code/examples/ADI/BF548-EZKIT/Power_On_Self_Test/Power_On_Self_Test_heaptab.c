/* MANAGED-BY-SYSTEM-BUILDER                                    */

/*
** User heap source file generated on Aug 24, 2007 at 15:20:53.
**
** Copyright (C) 2000-2007 Analog Devices Inc., All Rights Reserved.
**
** This file is generated automatically based upon the options selected
** in the LDF Wizard. Changes to the LDF configuration should be made by
** changing the appropriate options rather than editing this file.
**
** Configuration:-
**     crt_doj:                                .\Debug\Power_On_Self_Test_basiccrt.doj
**     processor:                              ADSP-BF548
**     si_revision:                            automatic
**     using_cplusplus:                        false
**     mem_init:                               false
**     use_vdk:                                false
**     use_eh:                                 true
**     use_argv:                               false
**     running_from_internal_memory:           true
**     user_heap_src_file:                     C:\Build Tools\nightly_build\cvsStage\_5.0ExportBlackfinReGen\Examples\Blackfin\Examples\ADSP-BF548 EZ-KIT Lite\Power_On_Self_Test\Power_On_Self_Test_heaptab.c
**     libraries_use_stdlib:                   true
**     libraries_use_fileio_libs:              false
**     libraries_use_ieeefp_emulation_libs:    false
**     libraries_use_eh_enabled_libs:          false
**     system_heap:                            L1
**     system_heap_min_size:                   2K
**     system_stack:                           L1
**     system_stack_min_size:                  8K
**     use_sdram:                              true
**     use_sdram_size:                         32M
**     use_sdram_partitioned:                  custom
**
*/

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_2_2)
#pragma diag(suppress:misra_rule_8_10)
#pragma diag(suppress:misra_rule_10_1_a)
#pragma diag(suppress:misra_rule_11_3)
#pragma diag(suppress:misra_rule_12_7)
#endif /* _MISRA_RULES */


extern "asm" int ldf_heap_space;
extern "asm" int ldf_heap_length;


struct heap_table_t
{
  void          *base;
  unsigned long  length;
  long int       userid;
};

#pragma file_attr("libData=HeapTable")
#pragma section("constdata")
struct heap_table_t heap_table[2] =
{


  { &ldf_heap_space, (int) &ldf_heap_length, 0 },


  { 0, 0, 0 }
};


#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

