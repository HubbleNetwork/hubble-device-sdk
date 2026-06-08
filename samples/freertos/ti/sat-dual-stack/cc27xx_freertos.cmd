/******************************************************************************

 @file  cc27xx_freertos.cmd

 @brief cc27xx family linker configuration file for FreeRTOS
        with Code Composer Studio.

        Imported Symbols
        Note: Linker defines are located in the CCS IDE project by placing them
        in
        Properties->Build->Linker->Advanced Options->Command File Preprocessing.

 Group: WCS, BTS
 Target Device: cc23xx

 ******************************************************************************
 $License: BSD3 2017 - 2025 $
 ******************************************************************************
 
 
 *****************************************************************************/

/* include the file that generates the globals which holds the base and size */
/* of the regions                                                            */
#include "ti_utils_build_linker.cmd.genmap"

--stack_size=1024
--heap_size=0
--entry_point resetISR

/* Retain interrupt vector table variable                                    */
--retain "*(.resetVecs)"

/* Suppress warnings and errors:                                             */
/* - 10063: Warning about entry point not being _c_int00                     */
/* - 16011, 16012: 8-byte alignment errors. Observed when linking in object  */
/*   files compiled using Keil (ARM compiler)                                */
--diag_suppress=10063,16011,16012

/* Set severity of diagnostics to Remark instead of Warning                  */
/* - 10068: Warning about no matching log_ptr* sections                      */
--diag_remark=10068

/* The starting address of the application. Normally the interrupt vectors   */
/* must be located at the beginning of the application.                      */

/*******************************************************************************
 * Memory Sizes
 */

/* Note: The assumption is that the NVS handle name is CONFIG_NVSINTERNAL in SysConfig */
#if defined(ti_utils_build_GenMap_NVS_CONFIG_NVSINTERNAL_BASE) && \
    defined(ti_utils_build_GenMap_NVS_CONFIG_NVSINTERNAL_SIZE)
#define NVS_BASE                ti_utils_build_GenMap_NVS_CONFIG_NVSINTERNAL_BASE
#define NVS_SIZE                ti_utils_build_GenMap_NVS_CONFIG_NVSINTERNAL_SIZE
#endif
#if defined(ti_utils_build_GenMap_NVS_CONFIG_NVSINTERNAL_ZB_BASE) && \
    defined(ti_utils_build_GenMap_NVS_CONFIG_NVSINTERNAL_ZB_SIZE)
#define NVRAM_BASE             ti_utils_build_GenMap_NVS_CONFIG_NVSINTERNAL_ZB_BASE
#define NVRAM_SIZE             ti_utils_build_GenMap_NVS_CONFIG_NVSINTERNAL_ZB_SIZE
#endif
#if defined(ti_utils_build_GenMap_FLASH0_BASE) && \
    defined(ti_utils_build_GenMap_FLASH0_SIZE)
#define FLASH_BASE              ti_utils_build_GenMap_FLASH0_BASE
/* If NVS_BASE is defined, set it as the FLASH_SIZE                             */
/* Note: The assumption is that the NVS slot is located in the end of the flash */
#if defined(NVS_BASE)
#define FLASH_SIZE              NVS_BASE
#elif (defined(ti_utils_build_GenMap_HSM_FW_BASE) && (ti_utils_build_GenMap_HSM_FW_BASE < ti_utils_build_GenMap_FLASH0_BASE + ti_utils_build_GenMap_FLASH0_SIZE))
/* The last part of Flash is reserved for the HSM FW, if it overlaps.
 * Otherwise Flash size stays the same.
 */
#define FLASH_SIZE              (ti_utils_build_GenMap_HSM_FW_BASE - ti_utils_build_GenMap_FLASH0_BASE)
#else
#define FLASH_SIZE              ti_utils_build_GenMap_FLASH0_SIZE
#endif
#endif

#if defined(ti_utils_build_GenMap_RAM0_BASE) && \
    defined(ti_utils_build_GenMap_RAM0_SIZE)
#define RAM_BASE                ti_utils_build_GenMap_RAM0_BASE
#define RAM_SIZE                ti_utils_build_GenMap_RAM0_SIZE
#endif

/* Not all LPF3 devices have S2RRAM */
#if defined(ti_utils_build_GenMap_S2RRAM_BASE) && \
    defined(ti_utils_build_GenMap_S2RRAM_SIZE)
#define S2RRAM_BASE             ti_utils_build_GenMap_S2RRAM_BASE
#define S2RRAM_SIZE             ti_utils_build_GenMap_S2RRAM_SIZE
#endif

#if defined(ti_utils_build_GenMap_CCFG_BASE) && \
    defined(ti_utils_build_GenMap_CCFG_SIZE)
#define CCFG_BASE               ti_utils_build_GenMap_CCFG_BASE
#define CCFG_SIZE               ti_utils_build_GenMap_CCFG_SIZE
#endif

/* Not all LPF3 devices have SCFG */
#if defined(ti_utils_build_GenMap_SCFG_BASE) && \
    defined(ti_utils_build_GenMap_SCFG_SIZE)
#define SCFG_BASE               ti_utils_build_GenMap_SCFG_BASE
#define SCFG_SIZE               ti_utils_build_GenMap_SCFG_SIZE
#endif

/*******************************************************************************
 * OAD - Not all LPF3 devices supports OAD
 * If they do support, there are regions to define and align differnetlly
 */
#if defined(OAD_APP_ONCHIP) || defined(OAD_PERSISTENT)
/* The SECURE BOOT trailer size, this size is taken from the end of the application */
/* This space is used by the sbtool running in the post build step to store information */
#define SB_TRAILER_SIZE        0x640
#endif

#if defined(OAD_APP_OFFCHIP) || defined(OAD_APP_ONCHIP) || defined(OAD_DUAL_IMAGE)

/* If SECURE_BOOT is used, set the APP_HDR_SIZE to the size of the secure boot header */
/* Set the APP_BASE to the SECURE_BOOT flash base */
/* Set the APP_SIZE to the SECURE_BOOT flash size */
#if defined(ti_utils_build_GenMap_FLASH0_SIZE) && \
    defined(ti_utils_build_GenMap_SECURE_BOOT_FLASH_BASE) && \
    defined(ti_utils_build_GenMap_SECURE_BOOT_FLASH_SIZE) && \
    defined(ti_utils_build_GenMap_SECURE_BOOT_HDR_SIZE)

#define APP_HDR_SIZE    ti_utils_build_GenMap_SECURE_BOOT_HDR_SIZE
#define APP_BASE        ti_utils_build_GenMap_SECURE_BOOT_FLASH_BASE
#define APP_SIZE        ti_utils_build_GenMap_SECURE_BOOT_FLASH_SIZE
#define APP_HDR_BASE    (APP_BASE - APP_HDR_SIZE)

#if defined(OAD_APP_ONCHIP)
#define PERSISTENT_HDR_BASE     (FLASH_BASE)
#define PERSISTENT_BASE         (PERSISTENT_HDR_BASE + APP_HDR_SIZE)
#define PERSISTENT_SIZE         (APP_BASE - APP_HDR_SIZE - SB_TRAILER_SIZE)
#endif //defined(OAD_APP_ONCHIP)

#endif
#endif //defined(OAD_APP_OFFCHIP) || defined(OAD_APP_ONCHIP) || defined(OAD_DUAL_IMAGE)

#if defined(OAD_PERSISTENT)
/* If SECURE_BOOT is used, set the APP_HDR_SIZE to the size of the secure boot header */
/* Set the APP_BASE to the SECURE_BOOT flash base */
/* Set the APP_SIZE to the SECURE_BOOT flash size */
#if defined(ti_utils_build_GenMap_FLASH0_SIZE) && \
    defined(ti_utils_build_GenMap_SECURE_BOOT_FLASH_BASE) && \
    defined(ti_utils_build_GenMap_SECURE_BOOT_FLASH_SIZE) && \
    defined(ti_utils_build_GenMap_SECURE_BOOT_HDR_SIZE)

#define APP_HDR_SIZE            ti_utils_build_GenMap_SECURE_BOOT_HDR_SIZE
#define PERSISTENT_BASE         ti_utils_build_GenMap_SECURE_BOOT_FLASH_BASE
#define PERSISTENT_SIZE         ti_utils_build_GenMap_SECURE_BOOT_FLASH_SIZE
#define PERSISTENT_HDR_BASE     (PERSISTENT_BASE - APP_HDR_SIZE)
#define APP_HDR_BASE            (PERSISTENT_BASE + PERSISTENT_SIZE + SB_TRAILER_SIZE)
#define APP_BASE                (APP_HDR_BASE + APP_HDR_SIZE)
#define APP_SIZE                (FLASH_SIZE - APP_BASE - SB_TRAILER_SIZE)

#endif
#endif //defined(OAD_PERSISTENT)

/*******************************************************************************
 * Stack
 */

/* Create global constant that points to top of stack */
/* CCS: Change stack size under Project Properties    */
__STACK_TOP = __stack + __STACK_SIZE;


/*******************************************************************************
 * System memory map
 */
MEMORY
{
#if defined(OAD_APP_OFFCHIP)|| defined(OAD_APP_ONCHIP) || defined(OAD_PERSISTENT) || defined(OAD_DUAL_IMAGE)

    APP_HDR_SLOT(RX)    : origin = APP_HDR_BASE,    length = APP_HDR_SIZE
    APP_SLOT (RX)       : origin = APP_BASE,        length = APP_SIZE

#if defined(OAD_APP_ONCHIP) || defined(OAD_PERSISTENT)

    PERSISTENT_HDR_SLOT(RX) : origin = PERSISTENT_HDR_BASE, length = APP_HDR_SIZE
    PERSISTENT_SLOT(RX)     : origin = PERSISTENT_BASE,     length = PERSISTENT_SIZE

#endif //defined(OAD_APP_ONCHIP) || defined(OAD_PERSISTENT)

#if defined(OAD_PERSISTENT)
    /* If this is a persistent image, the target base and slot are set to */
    /* the persistent image base and slot. This is to ensure that the */
    /* application is loaded in the correct location. */
    #define TARGET_BASE PERSISTENT_BASE
    #define TARGET_SLOT PERSISTENT_SLOT
#else
    /* If this is not a persistent image, the target base and slot are set to */
    /* the application base and slot. This is to ensure that the application */
    /* is loaded in the correct location. */
    #define TARGET_BASE APP_BASE
    #define TARGET_SLOT APP_SLOT
#endif //defined(OAD_PERSISTENT)

#else //No OAD

    /* Application stored in and executes from internal flash */
    FLASH (RX) : origin = FLASH_BASE, length = FLASH_SIZE

    /* If this is a non OAD image, the target base and slot are set to */
    /* the flash base and slot. This is to ensure that the application */
    /* is loaded in the correct location. */
    #define TARGET_BASE FLASH_BASE
    #define TARGET_SLOT FLASH

#endif //defined(OAD_APP_OFFCHIP)|| defined(OAD_APP_ONCHIP) || defined(OAD_PERSISTENT) || defined(OAD_DUAL_IMAGE)

#if defined(NVRAM_BASE) && defined(NVRAM_SIZE)
    NVRAM (RX) : origin = NVRAM_BASE, length = NVRAM_SIZE
#endif

#if defined(NVS_BASE) && defined(NVS_SIZE)
    NVS_SLOT(RX) : origin = NVS_BASE ,length = NVS_SIZE
#endif

#if defined(RAM_BASE) && defined(RAM_SIZE)
    /* Application uses internal RAM for data */
    SRAM (RWX) : origin = RAM_BASE, length = RAM_SIZE
#endif

#if defined(S2RRAM_BASE) && defined(S2RRAM_SIZE)
    /* S2RRAM is intended for the S2R radio module, but it can also be used by
     * the application with some limitations. Please refer to the s2rram example.
     */
    S2RRAM (RW) : origin = S2RRAM_BASE, length = S2RRAM_SIZE
#endif

#if defined(CCFG_BASE) && defined(CCFG_SIZE)
    /* Configuration region */
    CCFG (R) : origin = CCFG_BASE, length = CCFG_SIZE
#endif

#if defined(SCFG_BASE) && defined(SCFG_SIZE)
    /* Security configuration region */
    SCFG (R): origin = SCFG_BASE, length = SCFG_SIZE
#endif

    /* Explicitly placed off target for the storage of logging data.
     * The ARM memory map allocates 1 GB of external memory from 0x60000000 - 0x9FFFFFFF.
     * Unlikely that all of this will be used, so we are using the upper parts of the region.
     * ARM memory map: https://developer.arm.com/documentation/ddi0337/e/memory-map/about-the-memory-map*/

    LOG_DATA (R) : origin = 0x90000000, length = 0x40000        /* 256 KB */
    LOG_PTR  (R) : origin = 0x94000008, length = 0x40000        /* 256 KB */
}

/*******************************************************************************
 * Section allocation in memory
 */
SECTIONS
{
#if defined(OAD_APP_OFFCHIP) || defined(OAD_APP_ONCHIP) || defined(OAD_PERSISTENT) || defined(OAD_DUAL_IMAGE)
	.primary_hdr    :   > APP_HDR_SLOT, type = NOLOAD
#else
    .TI.ramfunc     : {} load=FLASH, run=SRAM, table(BINIT)
#endif
    .resetVecs      :   > TARGET_BASE
    .text           :   > TARGET_SLOT
    .const          :   > TARGET_SLOT
    .constdata      :   > TARGET_SLOT
    .rodata         :   > TARGET_SLOT
    .binit          :   > TARGET_SLOT
    .cinit          :   > TARGET_SLOT
    .pinit          :   > TARGET_SLOT
    .init_array     :   > TARGET_SLOT
    .emb_text       :   > TARGET_SLOT

#if defined(CCFG_BASE) && defined(CCFG_SIZE)
    .ccfg           :   > CCFG
#endif

#if defined(SCFG_BASE) && defined(SCFG_SIZE)
    .scfg           :   > SCFG
#endif

#if defined(RAM_BASE) && defined(RAM_SIZE)
    .ramVecs        :   > SRAM, type = NOLOAD, ALIGN(256)
    .data           :   > SRAM
    .bss            :   > SRAM
    .sysmem         :   > SRAM
    .stack          :   > SRAM (HIGH)
    .nonretenvar    :   > SRAM
#endif

#if defined(S2RRAM_BASE) && defined(S2RRAM_SIZE)
    /* Placing the section .s2rram in S2RRAM region. Only uninitialized
     * objects may be placed in this section.
     */
    .s2rram         :   > S2RRAM, type = NOINIT
#endif

    .log_data       :   > LOG_DATA, type = COPY
    .log_ptr        : { *(.log_ptr*) } > LOG_PTR align 4, type = COPY
}

