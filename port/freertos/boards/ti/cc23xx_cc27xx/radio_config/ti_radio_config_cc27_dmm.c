/*
 *  ======== ti_radio_config.c ========
 *  Configured RadioConfig module definitions
 *
 *  DO NOT EDIT - This file is generated for the CC2755P105RHA
 *  by the SysConfig tool.
 *
 *  Tool Name:           RadioConfig 8
 *  Tool Version:        9.11.00.02_ga
 *  RF Settings Version: 
 */

#include "ti_radio_config.h"
#include DeviceFamily_constructPath(rf_patches/lrf_pbe_binary_ble5.h)
#include DeviceFamily_constructPath(rf_patches/lrf_mce_binary_ble5_phac.h)
#include DeviceFamily_constructPath(rf_patches/lrf_rfe_binary_ble5.h)
#include DeviceFamily_constructPath(rf_patches/lrf_pbe_binary_ble_cs.h)
#include DeviceFamily_constructPath(rf_patches/lrf_mce_binary_ble_cs.h)
#include DeviceFamily_constructPath(rf_patches/lrf_rfe_binary_ble_cs.h)
#include DeviceFamily_constructPath(rf_patches/lrf_pbe_binary_generic.h)


// ****************************************************************
//  PHY name:            1 Mbps / 2 Mbps / Coded
//  PHY abbreviation/ID: ble
//  PHY definition file: ble_bt5_1m_2m_coded_cc27xx.json
// ****************************************************************



// Configuration: Common
static const uint32_t LRF_commonRegConfig[] =
{
    0x0000003D,                     // Segment length = 61
    0x0008A001,                     //   Data structure 32-bit region (start byte position = 8, count = 2)
    (uint32_t) &LRF_txPowerTable,   //     LRF_swParam : txPowerTable
    (uint32_t) &(fcfg->appTrims),   //     LRF_swParam : trimDef
    0x14502001,                     //   HW 32-bit region (start address = 0x1450, count = 2)
    0x22000000,                     //     LRFDPBE.POLY0H                LRFDPBE.POLY0L
    0x00065B00,                     //     LRFDPBE.POLY1H                LRFDPBE.POLY1L
    0x10B41004,                     //   HW 16-bit region (start address = 0x10B4, count = 5)
    0x00C40021,                     //     LRFDPBE.FCFG1                 LRFDPBE.FCFG0
    0x00800086,                     //     LRFDPBE.FCFG3                 LRFDPBE.FCFG2
    0x00000044,                     //     -                             LRFDPBE.FCFG4
    0x14682000,                     //   HW 32-bit region (start address = 0x1468, count = 1)
    0x00020004,                     //     LRFDPBE.TXFWBTHRS             LRFDPBE.RXFRBTHRS
    0x10DC1001,                     //   HW 16-bit region (start address = 0x10DC, count = 2)
    0x0B0B0202,                     //     LRFDPBE.TIMPRE                LRFDPBE.TIMCTL
    0x0000300A,                     //   HW sparse region (address/value pairs, count = 11)
    0x20C00003,                     //     LRFDMDM.ADCDIGCONF
    0x20C8001F,                     //     LRFDMDM.MODSYMMAP0
    0x20F00000,                     //     LRFDMDM.DEMIQMC0
    0x20F80080,                     //     LRFDMDM.DEMDSBU2
    0x21000000,                     //     LRFDMDM.DEMFIDC0
    0x21480000,                     //     LRFDMDM.MODCTRL
    0x30840000,                     //     LRFDRFE.MAGNTHRCFG
    0x308C0000,                     //     LRFDRFE.RSSIOFFSET
    0x30AC0000,                     //     LRFDRFE.SPARE4
    0x3120D820,                     //     LRFDRFE.MISC0
    0x31300C07,                     //     LRFDRFE.PHEDISC
    0x30B41002,                     //   HW 16-bit region (start address = 0x30B4, count = 3)
    0x00060006,                     //     LRFDRFE.IFAMPRFLDO            LRFDRFE.LNA
    0x00002970,                     //     -                             LRFDRFE.PA0
    0x30C40005,                     //   HW zero region (start address = 0x30C4, count = 6)
    0x30E4100C,                     //   HW 16-bit region (start address = 0x30E4, count = 13)
    0x00000200,                     //     LRFDRFE.DCO                   LRFDRFE.ATSTREFH
    0x00000008,                     //     LRFDRFE.DIVLDO                LRFDRFE.DIV
    0x00000000,                     //     LRFDRFE.DCOLDO0               LRFDRFE.TDCLDO
    0x07060000,                     //     LRFDRFE.PRE0                  LRFDRFE.DCOLDO1
    0x06050000,                     //     LRFDRFE.PRE2                  LRFDRFE.PRE1
    0x40080603,                     //     LRFDRFE.CAL0                  LRFDRFE.PRE3
    0x00007F00,                     //     -                             LRFDRFE.CAL1
    0x31381002,                     //   HW 16-bit region (start address = 0x3138, count = 3)
    0x047FDF7F,                     //     LRFDRFE.PLLMON1               LRFDRFE.PLLMON0
    0x00001804,                     //     -                             LRFDRFE.MOD0
    0x202C6000,                     //   RAM 32-bit region (start address = 0x202C, count = 1)
    0x00B400B4,                     //     PBE_BLE5_RAM.RECALTIMEOUT     PBE_BLE5_RAM.SYNTHCALTIMEOUT
    0x00007001,                     //   RAM sparse region (address/value pairs, count = 2)
    0x20340197,                     //     PBE_BLE5_RAM.FIFOCFG
    0x20380002,                     //     PBE_BLE5_RAM.NAKHUB
    0x68046005,                     //   RAM 32-bit region (start address = 0x6804, count = 6)
    0x03000012,                     //     RFE_COMMON_RAM.TDCCAL0        RFE_COMMON_RAM.SYNTHCTL
    0x00100000,                     //     RFE_COMMON_RAM.TDCCAL2        RFE_COMMON_RAM.TDCCAL1
    0x569B0400,                     //     RFE_COMMON_RAM.K1LSB          RFE_COMMON_RAM.TDCPLL
    0x012D010A,                     //     RFE_COMMON_RAM.K2BL           RFE_COMMON_RAM.K1MSB
    0x132C0034,                     //     RFE_COMMON_RAM.K3BL           RFE_COMMON_RAM.K2AL
    0x916F07AB,                     //     RFE_COMMON_RAM.K5             RFE_COMMON_RAM.K3AL
    0x68204001,                     //   RAM zero region (start address = 0x16820, count = 2)
    0x68286004,                     //   RAM 32-bit region (start address = 0x6828, count = 5)
    0x48080008,                     //     RFE_COMMON_RAM.DIVF           RFE_COMMON_RAM.DIVI
    0x00000000,                     //     RFE_COMMON_RAM.DIVLDOF        RFE_COMMON_RAM.DIVLDOI
    0x00470014,                     //     RFE_COMMON_RAM.LDOSETTLE      RFE_COMMON_RAM.DIVLDOIOFF
    0x0005002E,                     //     RFE_COMMON_RAM.DCOSETTLE      RFE_COMMON_RAM.CHRGSETTLE
    0x0000FE00,                     //     RFE_COMMON_RAM.IFAMPRFLDODEFAULT RFE_COMMON_RAM.IFAMPRFLDOTX
    0x684E4001                      //   RAM zero region (start address = 0x1684E, count = 2)
};

// Configuration: Sub-PHY = 1 Mbps, 2 Mbps
static const uint32_t LRF_subPhy1Mbps2MbpsRegConfig[] =
{
    0x00024015,                     // Segment length = 21
    0x00003006,                     //   HW sparse region (address/value pairs, count = 7)
    0x1098AAAA,                     //     LRFDPBE.MDMCMDPAR2
    0x20B80015,                     //     LRFDMDM.SYSTIMEVTMUX0
    0x20F40087,                     //     LRFDMDM.DEMDSBU
    0x2134005F,                     //     LRFDMDM.DEMSWQU0
    0x22A00003,                     //     LRFDMDM.BAUDCOMP
    0x232C7F00,                     //     LRFDMDM.DEMC1BE12
    0x30A81F40,                     //     LRFDRFE.SPARE3
    0x246C2000,                     //   HW 32-bit region (start address = 0x246C, count = 1)
    0x0000000C,                     //     LRFDMDM.MODMAIN               LRFDMDM.BAUDPRE
    0x20E41001,                     //   HW 16-bit region (start address = 0x20E4, count = 2)
    0x00FB0001,                     //     LRFDMDM.DEMMISC2              LRFDMDM.DEMMISC1
    0x21081006,                     //   HW 16-bit region (start address = 0x2108, count = 7)
    0x04000005,                     //     LRFDMDM.DEMMAFI0              LRFDMDM.DEMFIFE0
    0x00C17B20,                     //     LRFDMDM.DEMMAFI2              LRFDMDM.DEMMAFI1
    0x7F27000F,                     //     LRFDMDM.DEMC1BE1              LRFDMDM.DEMC1BE0
    0x0000017F,                     //     -                             LRFDMDM.DEMC1BE2
    0x24D42000,                     //   HW 32-bit region (start address = 0x24D4, count = 1)
    0x00540004,                     //     LRFDMDM.DEMD2XB0              LRFDMDM.DEMDSXB0
    0x68486000,                     //   RAM 32-bit region (start address = 0x6848, count = 1)
    0x00000045                      //     RFE_COMMON_RAM.AGCINFO        RFE_COMMON_RAM.SPARE1SHADOW
};

// Configuration: Sub-PHY = 1 Mbps, Coded
static const uint32_t LRF_subPhy1MbpsCodedRegConfig[] =
{
    0x00014009,                     // Segment length = 9
    0x0000A001,                     //   Data structure 32-bit region (start byte position = 0, count = 2)
    (uint32_t) &LRF_swConfig1Mbps,  //     LRF_swParam : swConfig
    (uint32_t) &LRF_txPowerLimitTable1Mbps,//     LRF_swParam : txPowerLimitTable
    0x00003002,                     //   HW sparse region (address/value pairs, count = 3)
    0x20D44000,                     //     LRFDMDM.BAUD
    0x20E00387,                     //     LRFDMDM.DEMMISC0
    0x21242318,                     //     LRFDMDM.SPARE0
    0x00007000,                     //   RAM sparse region (address/value pairs, count = 1)
    0x68440051                      //     RFE_COMMON_RAM.PHYRSSIOFFSET
};

// Configuration: Sub-PHY = 2 Mbps, Coded
static const uint32_t LRF_subPhy2MbpsCodedRegConfig[] =
{
    0x00030002,                     // Segment length = 2
    0x00003000,                     //   HW sparse region (address/value pairs, count = 1)
    0x21040A24                      //     LRFDMDM.DEMFEXB0
};

// Configuration: Sub-PHY = 1 Mbps
static const uint32_t LRF_subPhy1MbpsRegConfig[] =
{
    0x0003400E,                     // Segment length = 14
    0x00003006,                     //   HW sparse region (address/value pairs, count = 7)
    0x10908007,                     //     LRFDPBE.MDMCMDPAR0
    0x10E803E8,                     //     LRFDPBE.TIMPER1
    0x20C40007,                     //     LRFDMDM.MODPRECTRL
    0x20EC1182,                     //     LRFDMDM.DEMMISC3
    0x20FC0F50,                     //     LRFDMDM.DEMCODC0
    0x21040224,                     //     LRFDMDM.DEMFEXB0
    0x30A4A246,                     //     LRFDRFE.SPARE2
    0x00007001,                     //   RAM sparse region (address/value pairs, count = 2)
    0x20200000,                     //     PBE_BLE5_RAM.PHY
    0x684675F8,                     //     RFE_COMMON_RAM.SPARE0SHADOW
    0x20265002,                     //   RAM 16-bit region (start address = 0x2026, count = 3)
    0x0186012A,                     //     PBE_BLE5_RAM.PRERXIFS         PBE_BLE5_RAM.PRETXIFS
    0x00000226                      //     -                             PBE_BLE5_RAM.RXTIMEOUT
};

// Configuration: Sub-PHY = 2 Mbps
static const uint32_t LRF_subPhy2MbpsRegConfig[] =
{
    0x00010013,                     // Segment length = 19
    0x0000A001,                     //   Data structure 32-bit region (start byte position = 0, count = 2)
    (uint32_t) &LRF_swConfig2Mbps,  //     LRF_swParam : swConfig
    (uint32_t) &LRF_txPowerLimitTable2Mbps,//     LRF_swParam : txPowerLimitTable
    0x00003007,                     //   HW sparse region (address/value pairs, count = 8)
    0x1090800F,                     //     LRFDPBE.MDMCMDPAR0
    0x20C40017,                     //     LRFDMDM.MODPRECTRL
    0x20D48000,                     //     LRFDMDM.BAUD
    0x20E00399,                     //     LRFDMDM.DEMMISC0
    0x20EC1081,                     //     LRFDMDM.DEMMISC3
    0x20FC0F70,                     //     LRFDMDM.DEMCODC0
    0x2124232C,                     //     LRFDMDM.SPARE0
    0x30A4A357,                     //     LRFDRFE.SPARE2
    0x00007000,                     //   RAM sparse region (address/value pairs, count = 1)
    0x20200001,                     //     PBE_BLE5_RAM.PHY
    0x20265002,                     //   RAM 16-bit region (start address = 0x2026, count = 3)
    0x018C014E,                     //     PBE_BLE5_RAM.PRERXIFS         PBE_BLE5_RAM.PRETXIFS
    0x000001C2,                     //     -                             PBE_BLE5_RAM.RXTIMEOUT
    0x68446000,                     //   RAM 32-bit region (start address = 0x6844, count = 1)
    0x55FA004C                      //     RFE_COMMON_RAM.SPARE0SHADOW   RFE_COMMON_RAM.PHYRSSIOFFSET
};

// Configuration: Sub-PHY = Coded
static const uint32_t LRF_subPhyCodedRegConfig[] =
{
    0x00020025,                     // Segment length = 37
    0x00003006,                     //   HW sparse region (address/value pairs, count = 7)
    0x10900097,                     //     LRFDPBE.MDMCMDPAR0
    0x10983C3C,                     //     LRFDPBE.MDMCMDPAR2
    0x20C40097,                     //     LRFDMDM.MODPRECTRL
    0x20F4008D,                     //     LRFDMDM.DEMDSBU
    0x20FC0670,                     //     LRFDMDM.DEMCODC0
    0x2134001F,                     //     LRFDMDM.DEMSWQU0
    0x22A00007,                     //     LRFDMDM.BAUDCOMP
    0x246C2000,                     //   HW 32-bit region (start address = 0x246C, count = 1)
    0x000CA00C,                     //     LRFDMDM.MODMAIN               LRFDMDM.BAUDPRE
    0x20E41002,                     //   HW 16-bit region (start address = 0x20E4, count = 3)
    0x00DF0003,                     //     LRFDMDM.DEMMISC2              LRFDMDM.DEMMISC1
    0x00003582,                     //     -                             LRFDMDM.DEMMISC3
    0x24842002,                     //   HW 32-bit region (start address = 0x2484, count = 3)
    0x0000000C,                     //     LRFDMDM.DEMMAFI0              LRFDMDM.DEMFIFE0
    0x00505014,                     //     LRFDMDM.DEMMAFI2              LRFDMDM.DEMMAFI1
    0x7F7F001F,                     //     LRFDMDM.DEMC1BE1              LRFDMDM.DEMC1BE0
    0x24D42000,                     //   HW 32-bit region (start address = 0x24D4, count = 1)
    0x00560024,                     //     LRFDMDM.DEMD2XB0              LRFDMDM.DEMDSXB0
    0x22B41001,                     //   HW 16-bit region (start address = 0x22B4, count = 2)
    0x7C404040,                     //     LRFDMDM.DEMFB2P1              LRFDMDM.DEMFB2P0
    0x25682001,                     //   HW 32-bit region (start address = 0x2568, count = 2)
    0xFD90030C,                     //     LRFDMDM.DEMPHAC1              LRFDMDM.DEMPHAC0
    0x0000783C,                     //     LRFDMDM.DEMPHAC3              LRFDMDM.DEMPHAC2
    0x23241002,                     //   HW 16-bit region (start address = 0x2324, count = 3)
    0x7F7F8804,                     //     LRFDMDM.DEMC1BE11             LRFDMDM.DEMC1BE10
    0x00002C7F,                     //     -                             LRFDMDM.DEMC1BE12
    0x344C2000,                     //   HW 32-bit region (start address = 0x344C, count = 1)
    0x36F21307,                     //     LRFDRFE.SPARE0                LRFDRFE.MAGNCTL1
    0x30A41001,                     //   HW 16-bit region (start address = 0x30A4, count = 2)
    0x00B00015,                     //     LRFDRFE.SPARE3                LRFDRFE.SPARE2
    0x20206002,                     //   RAM 32-bit region (start address = 0x2020, count = 3)
    0x014D0002,                     //     PBE_BLE5_RAM.PRETXIFS500K     PBE_BLE5_RAM.PHY
    0x01340044,                     //     PBE_BLE5_RAM.PRETXIFS         PBE_BLE5_RAM.TXIFS500KADJ
    0x0884012C,                     //     PBE_BLE5_RAM.RXTIMEOUT        PBE_BLE5_RAM.PRERXIFS
    0x68486000,                     //   RAM 32-bit region (start address = 0x6848, count = 1)
    0x00010027                      //     RFE_COMMON_RAM.AGCINFO        RFE_COMMON_RAM.SPARE1SHADOW
};

// Configuration: Coded TX rate = S8 (8 symbols per bit = 125 kbps)
static const uint32_t LRF_codedTxRateS8RegConfig[] =
{
    0x00044002,                     // Segment length = 2
    0x00003000,                     //   HW sparse region (address/value pairs, count = 1)
    0x21280000                      //     LRFDMDM.SPARE1
};

// Configuration: Coded TX rate = S2 (2 symbols per bit = 500 kbps)
static const uint32_t LRF_codedTxRateS2RegConfig[] =
{
    0x00040002,                     // Segment length = 2
    0x00003000,                     //   HW sparse region (address/value pairs, count = 1)
    0x21280001                      //     LRFDMDM.SPARE1
};


// LRF register configuration list
static const LRF_RegConfigList LRF_regConfigList = {
    .numEntries = 9,
    .entries = {
        (LRF_ConfigWord*)LRF_commonRegConfig,
        (LRF_ConfigWord*)LRF_subPhy1Mbps2MbpsRegConfig,
        (LRF_ConfigWord*)LRF_subPhy1MbpsCodedRegConfig,
        (LRF_ConfigWord*)LRF_subPhy2MbpsCodedRegConfig,
        (LRF_ConfigWord*)LRF_subPhy1MbpsRegConfig,
        (LRF_ConfigWord*)LRF_subPhy2MbpsRegConfig,
        (LRF_ConfigWord*)LRF_subPhyCodedRegConfig,
        (LRF_ConfigWord*)LRF_codedTxRateS8RegConfig,
        (LRF_ConfigWord*)LRF_codedTxRateS2RegConfig
    }
};

// LRF_TxShape data structure
const LRF_TxShape LRF_shapeBaseGfsk067 = {
    .scale = 0x0FDE2,
    .numCoeff = 0x0011,
    .coeff = { 0x01, 0x02, 0x05, 0x0A, 0x14, 0x22, 0x37, 0x52, 0x71, 0x91, 0xB0, 0xCB, 0xE0, 0xEE, 0xF8, 0xFD, 0xFF}
};

// LRF_TxShape data structure
const LRF_TxShape LRF_shapeBaseGfsk05 = {
    .scale = 0x0F183,
    .numCoeff = 0x0014,
    .coeff = { 0x01, 0x02, 0x03, 0x06, 0x0A, 0x11, 0x1A, 0x27, 0x37, 0x4B, 0x62, 0x7B, 0x94, 0xAD, 0xC4, 0xD8, 0xE7, 0xF3, 0xFB, 0xFF}
};

// LRF_SwConfig data structure
const LRF_SwConfig LRF_swConfig1Mbps = {
    .rxIntFrequency = 1000000,
    .rxFrequencyOffset = 0,
    .txFrequencyOffset = 1000000,
    .modFrequencyDeviation = 0x0003D090,
    .txShape = &LRF_shapeBaseGfsk067,
    .bwIndex = 0x00,
    .bwIndexDither = 0x01
};

// LRF_SwConfig data structure
const LRF_SwConfig LRF_swConfig2Mbps = {
    .rxIntFrequency = 850000,
    .rxFrequencyOffset = 0,
    .txFrequencyOffset = 850000,
    .modFrequencyDeviation = 0x0007A120,
    .txShape = &LRF_shapeBaseGfsk05,
    .bwIndex = 0x01,
    .bwIndexDither = 0x01
};

// LRF_TxPowerTable data structure
const LRF_TxPowerTable LRF_txPowerTable = {
    .numEntries = 0x0000001A,
    .powerTable = {
        { .power = { .fraction = 0, .dBm = -20 }, .tempCoeff = 0, .value = { .ibBoost = 0, .ib = 18, .gain = 0, .mode = 0, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = -16 }, .tempCoeff = 3, .value = { .ibBoost = 0, .ib = 26, .gain = 0, .mode = 0, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = -12 }, .tempCoeff = 8, .value = { .ibBoost = 0, .ib = 30, .gain = 1, .mode = 0, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = -8 }, .tempCoeff = 13, .value = { .ibBoost = 0, .ib = 27, .gain = 3, .mode = 0, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = -4 }, .tempCoeff = 35, .value = { .ibBoost = 0, .ib = 46, .gain = 3, .mode = 0, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 0 }, .tempCoeff = 30, .value = { .ibBoost = 0, .ib = 37, .gain = 4, .mode = 1, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 1 }, .tempCoeff = 35, .value = { .ibBoost = 0, .ib = 43, .gain = 4, .mode = 1, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 2 }, .tempCoeff = 35, .value = { .ibBoost = 0, .ib = 35, .gain = 5, .mode = 1, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 3 }, .tempCoeff = 45, .value = { .ibBoost = 0, .ib = 43, .gain = 5, .mode = 1, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 4 }, .tempCoeff = 50, .value = { .ibBoost = 0, .ib = 33, .gain = 6, .mode = 1, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 5 }, .tempCoeff = 10, .value = { .ibBoost = 0, .ib = 54, .gain = 3, .mode = 2, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 6 }, .tempCoeff = 6, .value = { .ibBoost = 1, .ib = 29, .gain = 3, .mode = 2, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 7 }, .tempCoeff = 5, .value = { .ibBoost = 1, .ib = 34, .gain = 3, .mode = 2, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 8 }, .tempCoeff = 8, .value = { .ibBoost = 1, .ib = 41, .gain = 3, .mode = 2, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 9 }, .tempCoeff = 20, .value = { .ibBoost = 1, .ib = 34, .gain = 4, .mode = 2, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 10 }, .tempCoeff = 30, .value = { .ibBoost = 1, .ib = 45, .gain = 4, .mode = 2, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 11 }, .tempCoeff = 65, .value = { .ibBoost = 1, .ib = 47, .gain = 5, .mode = 2, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 12 }, .tempCoeff = 15, .value = { .ibBoost = 1, .ib = 44, .gain = 3, .mode = 3, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 13 }, .tempCoeff = 18, .value = { .ibBoost = 1, .ib = 49, .gain = 3, .mode = 3, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 14 }, .tempCoeff = 25, .value = { .ibBoost = 1, .ib = 40, .gain = 4, .mode = 3, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 15 }, .tempCoeff = 20, .value = { .ibBoost = 2, .ib = 48, .gain = 3, .mode = 3, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 16 }, .tempCoeff = 40, .value = { .ibBoost = 1, .ib = 40, .gain = 5, .mode = 3, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 17 }, .tempCoeff = 36, .value = { .ibBoost = 3, .ib = 39, .gain = 4, .mode = 3, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 18 }, .tempCoeff = 40, .value = { .ibBoost = 3, .ib = 40, .gain = 5, .mode = 3, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 19 }, .tempCoeff = 80, .value = { .ibBoost = 2, .ib = 49, .gain = 6, .mode = 3, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } },
        { .power = { .fraction = 0, .dBm = 20 }, .tempCoeff = 140, .value = { .ibBoost = 3, .ib = 47, .gain = 7, .mode = 3, .rtrimTxCompCtl = 0, .pa20dBmEsdCtl = 0, .noIfampRfLdoBypass = 0 } }
    }
};

// LRF_TxPowerLimitTable data structure
const LRF_TxPowerLimitTable LRF_txPowerLimitTable1Mbps = {
    .numEntries = 0x00000005,
    .freqDiv = 0x000F4240,
    .limitTable = {
        { .minFreq = 2401, .maxFreq = 2403, .regulatoryMask = 0x01, .maxTxPower = { .fraction = 0, .dBm = 15 } },
        { .minFreq = 2401, .maxFreq = 2403, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 16 } },
        { .minFreq = 2475, .maxFreq = 2477, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 15 } },
        { .minFreq = 2477, .maxFreq = 2479, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 14 } },
        { .minFreq = 2479, .maxFreq = 2481, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 10 } }
    }
};

// LRF_TxPowerLimitTable data structure
const LRF_TxPowerLimitTable LRF_txPowerLimitTable2Mbps = {
    .numEntries = 0x00000003,
    .freqDiv = 0x000F4240,
    .limitTable = {
        { .minFreq = 2473, .maxFreq = 2475, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 17 } },
        { .minFreq = 2475, .maxFreq = 2477, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 16 } },
        { .minFreq = 2477, .maxFreq = 2479, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 13 } }
    }
};

// LRF_Config data structure
const LRF_Config LRF_config = {
    .pbeImage = (const LRF_TOPsmImage*) LRF_PBE_binary_ble5,
    .mceImage = (const LRF_TOPsmImage*) LRF_MCE_binary_ble5_phac,
    .rfeImage = (const LRF_TOPsmImage*) LRF_RFE_binary_ble5,
    .regConfigList = &LRF_regConfigList
};


// ****************************************************************
//  PHY name:            ADC Noise
//  PHY abbreviation/ID: adc_noise
//  PHY definition file: adc_noise_bt5_cc27xx.json
// ****************************************************************



// Configuration: Common
static const uint32_t LRF_commonRegConfigAdcNoise[] =
{
    0x0000002E,                     // Segment length = 46
    0x0000A003,                     //   Data structure 32-bit region (start byte position = 0, count = 4)
    (uint32_t) &LRF_swConfigAdcNoise,//     LRF_swParam : swConfig
    0x00000000,                     //     LRF_swParam : txPowerLimitTable
    0x00000000,                     //     LRF_swParam : txPowerTable
    (uint32_t) &(fcfg->appTrims),   //     LRF_swParam : trimDef
    0x00003003,                     //   HW sparse region (address/value pairs, count = 4)
    0x30840000,                     //     LRFDRFE.MAGNTHRCFG
    0x308C0000,                     //     LRFDRFE.RSSIOFFSET
    0x3120D820,                     //     LRFDRFE.MISC0
    0x31300C07,                     //     LRFDRFE.PHEDISC
    0x30A41002,                     //   HW 16-bit region (start address = 0x30A4, count = 3)
    0x1F40A246,                     //     LRFDRFE.SPARE3                LRFDRFE.SPARE2
    0x00000000,                     //     -                             LRFDRFE.SPARE4
    0x30B41002,                     //   HW 16-bit region (start address = 0x30B4, count = 3)
    0x00060006,                     //     LRFDRFE.IFAMPRFLDO            LRFDRFE.LNA
    0x00002940,                     //     -                             LRFDRFE.PA0
    0x30C40005,                     //   HW zero region (start address = 0x30C4, count = 6)
    0x30E4100C,                     //   HW 16-bit region (start address = 0x30E4, count = 13)
    0x00000200,                     //     LRFDRFE.DCO                   LRFDRFE.ATSTREFH
    0x00000008,                     //     LRFDRFE.DIVLDO                LRFDRFE.DIV
    0x00000000,                     //     LRFDRFE.DCOLDO0               LRFDRFE.TDCLDO
    0x07060000,                     //     LRFDRFE.PRE0                  LRFDRFE.DCOLDO1
    0x06050000,                     //     LRFDRFE.PRE2                  LRFDRFE.PRE1
    0x40080603,                     //     LRFDRFE.CAL0                  LRFDRFE.PRE3
    0x00007F00,                     //     -                             LRFDRFE.CAL1
    0x31381002,                     //   HW 16-bit region (start address = 0x3138, count = 3)
    0x047FDF7F,                     //     LRFDRFE.PLLMON1               LRFDRFE.PLLMON0
    0x00001804,                     //     -                             LRFDRFE.MOD0
    0x68046005,                     //   RAM 32-bit region (start address = 0x6804, count = 6)
    0x03000012,                     //     RFE_COMMON_RAM.TDCCAL0        RFE_COMMON_RAM.SYNTHCTL
    0x00100000,                     //     RFE_COMMON_RAM.TDCCAL2        RFE_COMMON_RAM.TDCCAL1
    0x569B0400,                     //     RFE_COMMON_RAM.K1LSB          RFE_COMMON_RAM.TDCPLL
    0x012D010A,                     //     RFE_COMMON_RAM.K2BL           RFE_COMMON_RAM.K1MSB
    0x132C0034,                     //     RFE_COMMON_RAM.K3BL           RFE_COMMON_RAM.K2AL
    0x916F07AB,                     //     RFE_COMMON_RAM.K5             RFE_COMMON_RAM.K3AL
    0x68204001,                     //   RAM zero region (start address = 0x16820, count = 2)
    0x68286004,                     //   RAM 32-bit region (start address = 0x6828, count = 5)
    0x48080008,                     //     RFE_COMMON_RAM.DIVF           RFE_COMMON_RAM.DIVI
    0x00000000,                     //     RFE_COMMON_RAM.DIVLDOF        RFE_COMMON_RAM.DIVLDOI
    0x00470014,                     //     RFE_COMMON_RAM.LDOSETTLE      RFE_COMMON_RAM.DIVLDOIOFF
    0x0005002E,                     //     RFE_COMMON_RAM.DCOSETTLE      RFE_COMMON_RAM.CHRGSETTLE
    0x0000FE00,                     //     RFE_COMMON_RAM.IFAMPRFLDODEFAULT RFE_COMMON_RAM.IFAMPRFLDOTX
    0x68446001,                     //   RAM 32-bit region (start address = 0x6844, count = 2)
    0x00220051,                     //     RFE_COMMON_RAM.SPARE0SHADOW   RFE_COMMON_RAM.PHYRSSIOFFSET
    0x00000045,                     //     RFE_COMMON_RAM.AGCINFO        RFE_COMMON_RAM.SPARE1SHADOW
    0x684E4001                      //   RAM zero region (start address = 0x1684E, count = 2)
};


// LRF register configuration list
static const LRF_RegConfigList LRF_regConfigListAdcNoise = {
    .numEntries = 1,
    .entries = {
        (LRF_ConfigWord*)LRF_commonRegConfigAdcNoise
    }
};

// LRF_SwConfig data structure
const LRF_SwConfig LRF_swConfigAdcNoise = {
    .rxIntFrequency = 0,
    .rxFrequencyOffset = 0,
    .txFrequencyOffset = 0,
    .modFrequencyDeviation = 0x00000000,
    .txShape = (LRF_TxShape*) 0,
    .bwIndex = 0x01,
    .bwIndexDither = 0x01
};

// LRF_Config data structure
const LRF_Config LRF_configAdcNoise = {
    .pbeImage = (LRF_TOPsmImage*) 0,
    .mceImage = (LRF_TOPsmImage*) 0,
    .rfeImage = (const LRF_TOPsmImage*) LRF_RFE_binary_ble5,
    .regConfigList = &LRF_regConfigListAdcNoise
};


// ****************************************************************
//  PHY name:            Channel Sounding
//  PHY abbreviation/ID: ble_cs_hp
//  PHY definition file: ble_bt6_cs_hp_cc27xx.json
// ****************************************************************



// Configuration: Common
static const uint32_t LRF_commonRegConfigBleCsHp[] =
{
    0x00000044,                     // Segment length = 68
    0x0000A003,                     //   Data structure 32-bit region (start byte position = 0, count = 4)
    (uint32_t) &LRF_swConfigBleCsHp,//     LRF_swParam : swConfig
    0x00000000,                     //     LRF_swParam : txPowerLimitTable
    (uint32_t) &LRF_txPowerTable,   //     LRF_swParam : txPowerTable
    (uint32_t) &(fcfg->appTrims),   //     LRF_swParam : trimDef
    0x00003008,                     //   HW sparse region (address/value pairs, count = 9)
    0x20B80015,                     //     LRFDMDM.SYSTIMEVTMUX0
    0x20C00003,                     //     LRFDMDM.ADCDIGCONF
    0x20C8001F,                     //     LRFDMDM.MODSYMMAP0
    0x2134005F,                     //     LRFDMDM.DEMSWQU0
    0x22A00000,                     //     LRFDMDM.BAUDCOMP
    0x30840000,                     //     LRFDRFE.MAGNTHRCFG
    0x308C0000,                     //     LRFDRFE.RSSIOFFSET
    0x3120D820,                     //     LRFDRFE.MISC0
    0x31300C07,                     //     LRFDRFE.PHEDISC
    0x20D41002,                     //   HW 16-bit region (start address = 0x20D4, count = 3)
    0x000C8000,                     //     LRFDMDM.BAUDPRE               LRFDMDM.BAUD
    0x00000000,                     //     -                             LRFDMDM.MODMAIN
    0x20E41010,                     //   HW 16-bit region (start address = 0x20E4, count = 17)
    0x05FB0001,                     //     LRFDMDM.DEMMISC2              LRFDMDM.DEMMISC1
    0x00001081,                     //     LRFDMDM.DEMIQMC0              LRFDMDM.DEMMISC3
    0x00800087,                     //     LRFDMDM.DEMDSBU2              LRFDMDM.DEMDSBU
    0x00000F70,                     //     LRFDMDM.DEMFIDC0              LRFDMDM.DEMCODC0
    0x00050224,                     //     LRFDMDM.DEMFIFE0              LRFDMDM.DEMFEXB0
    0x7B200400,                     //     LRFDMDM.DEMMAFI1              LRFDMDM.DEMMAFI0
    0x000F00C1,                     //     LRFDMDM.DEMC1BE0              LRFDMDM.DEMMAFI2
    0x017F7F27,                     //     LRFDMDM.DEMC1BE2              LRFDMDM.DEMC1BE1
    0x00000A2C,                     //     -                             LRFDMDM.SPARE0
    0x24D42000,                     //   HW 32-bit region (start address = 0x24D4, count = 1)
    0x00540004,                     //     LRFDMDM.DEMD2XB0              LRFDMDM.DEMDSXB0
    0x25942000,                     //   HW 32-bit region (start address = 0x2594, count = 1)
    0x7F7F7F7F,                     //     LRFDMDM.DEMC1BE12             LRFDMDM.DEMC1BE11
    0x30A41002,                     //   HW 16-bit region (start address = 0x30A4, count = 3)
    0x1F40A357,                     //     LRFDRFE.SPARE3                LRFDRFE.SPARE2
    0x00000000,                     //     -                             LRFDRFE.SPARE4
    0x30B41002,                     //   HW 16-bit region (start address = 0x30B4, count = 3)
    0x00060006,                     //     LRFDRFE.IFAMPRFLDO            LRFDRFE.LNA
    0x00002940,                     //     -                             LRFDRFE.PA0
    0x30C40005,                     //   HW zero region (start address = 0x30C4, count = 6)
    0x30E4100C,                     //   HW 16-bit region (start address = 0x30E4, count = 13)
    0x00000200,                     //     LRFDRFE.DCO                   LRFDRFE.ATSTREFH
    0x00000008,                     //     LRFDRFE.DIVLDO                LRFDRFE.DIV
    0x00000000,                     //     LRFDRFE.DCOLDO0               LRFDRFE.TDCLDO
    0x03060000,                     //     LRFDRFE.PRE0                  LRFDRFE.DCOLDO1
    0x06050000,                     //     LRFDRFE.PRE2                  LRFDRFE.PRE1
    0x40080603,                     //     LRFDRFE.CAL0                  LRFDRFE.PRE3
    0x00007F00,                     //     -                             LRFDRFE.CAL1
    0x31381002,                     //   HW 16-bit region (start address = 0x3138, count = 3)
    0x047FDF7F,                     //     LRFDRFE.PLLMON1               LRFDRFE.PLLMON0
    0x00001804,                     //     -                             LRFDRFE.MOD0
    0x68046005,                     //   RAM 32-bit region (start address = 0x6804, count = 6)
    0x03000010,                     //     RFE_COMMON_RAM.TDCCAL0        RFE_COMMON_RAM.SYNTHCTL
    0x00100000,                     //     RFE_COMMON_RAM.TDCCAL2        RFE_COMMON_RAM.TDCCAL1
    0x569B0400,                     //     RFE_COMMON_RAM.K1LSB          RFE_COMMON_RAM.TDCPLL
    0x04B4010A,                     //     RFE_COMMON_RAM.K2BL           RFE_COMMON_RAM.K1MSB
    0x2FEFA5A6,                     //     RFE_COMMON_RAM.K3BL           RFE_COMMON_RAM.K2AL
    0x916F0BFB,                     //     RFE_COMMON_RAM.K5             RFE_COMMON_RAM.K3AL
    0x68204001,                     //   RAM zero region (start address = 0x16820, count = 2)
    0x68286004,                     //   RAM 32-bit region (start address = 0x6828, count = 5)
    0x48080008,                     //     RFE_COMMON_RAM.DIVF           RFE_COMMON_RAM.DIVI
    0x00000000,                     //     RFE_COMMON_RAM.DIVLDOF        RFE_COMMON_RAM.DIVLDOI
    0x00470014,                     //     RFE_COMMON_RAM.LDOSETTLE      RFE_COMMON_RAM.DIVLDOIOFF
    0x0005002E,                     //     RFE_COMMON_RAM.DCOSETTLE      RFE_COMMON_RAM.CHRGSETTLE
    0x0000FE00,                     //     RFE_COMMON_RAM.IFAMPRFLDODEFAULT RFE_COMMON_RAM.IFAMPRFLDOTX
    0x684E4001,                     //   RAM zero region (start address = 0x1684E, count = 2)
    0x68546001,                     //   RAM 32-bit region (start address = 0x6854, count = 2)
    0x03870038,                     //     RFE_BLE_CS_RAM.K2CSTNTX       RFE_BLE_CS_RAM.K2CSPKTTX
    0x2FEF0BFB                      //     RFE_BLE_CS_RAM.K3CSTNTX       RFE_BLE_CS_RAM.K3CSPKTTX
};


// LRF register configuration list
static const LRF_RegConfigList LRF_regConfigListBleCsHp = {
    .numEntries = 1,
    .entries = {
        (LRF_ConfigWord*)LRF_commonRegConfigBleCsHp
    }
};

// LRF_SwConfig data structure
const LRF_SwConfig LRF_swConfigBleCsHp = {
    .rxIntFrequency = 0,
    .rxFrequencyOffset = 0,
    .txFrequencyOffset = 0,
    .modFrequencyDeviation = 0x00000000,
    .txShape = (LRF_TxShape*) 0,
    .bwIndex = 0x00,
    .bwIndexDither = 0x01
};

// LRF_Config data structure
const LRF_Config LRF_configBleCsHp = {
    .pbeImage = (const LRF_TOPsmImage*) LRF_PBE_binary_ble_cs,
    .mceImage = (const LRF_TOPsmImage*) LRF_MCE_binary_ble_cs,
    .rfeImage = (const LRF_TOPsmImage*) LRF_RFE_binary_ble_cs,
    .regConfigList = &LRF_regConfigListBleCsHp
};


// ****************************************************************
//  PHY name:            Generic 1 Mbps / 2 Mbps / Coded
//  PHY abbreviation/ID: ble_gen
//  PHY definition file: ble_bt5_1m_2m_coded_generic_cc27xx.json
// ****************************************************************

// PARAMETER SUMMARY

// Default sub-PHY: 1_mbps
// Coded rate in TX: s8
// Channel: 2440
// TX Power (dBm): 20

// Command: CMD_GENERIC_TX_TEST - Generic transmitter test command
RCL_CmdGenericTxTest rclPacketTxCmdGenericTxTest_ble_gen_3 = 
{
    .common.cmdId = RCL_CMDID_GENERIC_TX_TEST,
    .common.phyFeatures = 0x0000,
    .common.scheduling = RCL_Schedule_Now,
    .common.status = RCL_CommandStatus_Idle,
    .common.conflictPolicy = RCL_ConflictPolicy_AlwaysInterrupt,
    .common.allowDelay = 0x00,
    .common.runtime.handler = RCL_Handler_Generic_TxTest,
    .common.runtime.client = (RCL_Client*) 0,
    .common.runtime.lrfCallbackMask.value = 0x00000000,
    .common.runtime.rclCallbackMask.value = 0x00000000,
    .common.runtime.callback = (RCL_Callback) 0,
    .common.runtime.activityInfo = 0x00000000,
    .common.timing.absStartTime = 0x00000000,
    .common.timing.relMinTime = 0x00000000,
    .common.timing.relGracefulStopTime = 0,
    .common.timing.relHardStopTime = 0x00000000,
    .rfFrequency = 0x916F7200,
    .txPower.fraction = 0x0,
    .txPower.dBm = 0,
    .config.txWord = 0x0000,
    .config.whitenMode = 0x2,
    .config.sendCw = 0x0,
    .config.fsOff = 0x1
};


// Configuration: Common
static const uint32_t LRF_commonRegConfigBleGen[] =
{
    0x0000003F,                     // Segment length = 63
    0x0008A001,                     //   Data structure 32-bit region (start byte position = 8, count = 2)
    (uint32_t) &LRF_txPowerTable,   //     LRF_swParam : txPowerTable
    (uint32_t) &(fcfg->appTrims),   //     LRF_swParam : trimDef
    0x0000300B,                     //   HW sparse region (address/value pairs, count = 12)
    0x10940000,                     //     LRFDPBE.MDMCMDPAR1
    0x20C00003,                     //     LRFDMDM.ADCDIGCONF
    0x20C8001F,                     //     LRFDMDM.MODSYMMAP0
    0x20F00000,                     //     LRFDMDM.DEMIQMC0
    0x20F80080,                     //     LRFDMDM.DEMDSBU2
    0x21000000,                     //     LRFDMDM.DEMFIDC0
    0x21480000,                     //     LRFDMDM.MODCTRL
    0x30840000,                     //     LRFDRFE.MAGNTHRCFG
    0x308C0000,                     //     LRFDRFE.RSSIOFFSET
    0x30AC0000,                     //     LRFDRFE.SPARE4
    0x3120D820,                     //     LRFDRFE.MISC0
    0x31300C07,                     //     LRFDRFE.PHEDISC
    0x14502004,                     //   HW 32-bit region (start address = 0x1450, count = 5)
    0x22000000,                     //     LRFDPBE.POLY0H                LRFDPBE.POLY0L
    0x00065B00,                     //     LRFDPBE.POLY1H                LRFDPBE.POLY1L
    0x00210000,                     //     LRFDPBE.FCFG0                 LRFDPBE.PHACFG
    0x008600C4,                     //     LRFDPBE.FCFG2                 LRFDPBE.FCFG1
    0x00440080,                     //     LRFDPBE.FCFG4                 LRFDPBE.FCFG3
    0x14682000,                     //   HW 32-bit region (start address = 0x1468, count = 1)
    0x00020004,                     //     LRFDPBE.TXFWBTHRS             LRFDPBE.RXFRBTHRS
    0x10DC1001,                     //   HW 16-bit region (start address = 0x10DC, count = 2)
    0x0B0B0202,                     //     LRFDPBE.TIMPRE                LRFDPBE.TIMCTL
    0x30B41002,                     //   HW 16-bit region (start address = 0x30B4, count = 3)
    0x00060006,                     //     LRFDRFE.IFAMPRFLDO            LRFDRFE.LNA
    0x00002970,                     //     -                             LRFDRFE.PA0
    0x30C40005,                     //   HW zero region (start address = 0x30C4, count = 6)
    0x30E4100C,                     //   HW 16-bit region (start address = 0x30E4, count = 13)
    0x00000200,                     //     LRFDRFE.DCO                   LRFDRFE.ATSTREFH
    0x00000008,                     //     LRFDRFE.DIVLDO                LRFDRFE.DIV
    0x00000000,                     //     LRFDRFE.DCOLDO0               LRFDRFE.TDCLDO
    0x07060000,                     //     LRFDRFE.PRE0                  LRFDRFE.DCOLDO1
    0x06050000,                     //     LRFDRFE.PRE2                  LRFDRFE.PRE1
    0x40080603,                     //     LRFDRFE.CAL0                  LRFDRFE.PRE3
    0x00007F00,                     //     -                             LRFDRFE.CAL1
    0x31381002,                     //   HW 16-bit region (start address = 0x3138, count = 3)
    0x047FDF7F,                     //     LRFDRFE.PLLMON1               LRFDRFE.PLLMON0
    0x00001804,                     //     -                             LRFDRFE.MOD0
    0x2022500A,                     //   RAM 16-bit region (start address = 0x2022, count = 11)
    0x009000B4,                     //     PBE_GENERIC_RAM.PKTCFG        PBE_GENERIC_RAM.SYNTHCALTIMEOUT
    0x03B00018,                     //     PBE_GENERIC_RAM.FIFOCFG       PBE_GENERIC_RAM.NUMCRCBITS
    0x00510000,                     //     PBE_GENERIC_RAM.WHITEINIT     PBE_GENERIC_RAM.EXTRABYTES
    0x55000000,                     //     PBE_GENERIC_RAM.CRCINITL      < GAP >
    0x01085555,                     //     PBE_GENERIC_RAM.LENCFG        PBE_GENERIC_RAM.CRCINITH
    0x00000000,                     //     -                             PBE_GENERIC_RAM.LENOFFSET
    0x68046005,                     //   RAM 32-bit region (start address = 0x6804, count = 6)
    0x03000012,                     //     RFE_COMMON_RAM.TDCCAL0        RFE_COMMON_RAM.SYNTHCTL
    0x00100000,                     //     RFE_COMMON_RAM.TDCCAL2        RFE_COMMON_RAM.TDCCAL1
    0x569B0400,                     //     RFE_COMMON_RAM.K1LSB          RFE_COMMON_RAM.TDCPLL
    0x012D010A,                     //     RFE_COMMON_RAM.K2BL           RFE_COMMON_RAM.K1MSB
    0x132C0034,                     //     RFE_COMMON_RAM.K3BL           RFE_COMMON_RAM.K2AL
    0x916F07AB,                     //     RFE_COMMON_RAM.K5             RFE_COMMON_RAM.K3AL
    0x68204001,                     //   RAM zero region (start address = 0x16820, count = 2)
    0x68286004,                     //   RAM 32-bit region (start address = 0x6828, count = 5)
    0x48080008,                     //     RFE_COMMON_RAM.DIVF           RFE_COMMON_RAM.DIVI
    0x00000000,                     //     RFE_COMMON_RAM.DIVLDOF        RFE_COMMON_RAM.DIVLDOI
    0x00470014,                     //     RFE_COMMON_RAM.LDOSETTLE      RFE_COMMON_RAM.DIVLDOIOFF
    0x0005002E,                     //     RFE_COMMON_RAM.DCOSETTLE      RFE_COMMON_RAM.CHRGSETTLE
    0x0000FE00,                     //     RFE_COMMON_RAM.IFAMPRFLDODEFAULT RFE_COMMON_RAM.IFAMPRFLDOTX
    0x684E4001                      //   RAM zero region (start address = 0x1684E, count = 2)
};

// Configuration: Sub-PHY = 1 Mbps, 2 Mbps
static const uint32_t LRF_subPhy1Mbps2MbpsRegConfigBleGen[] =
{
    0x00024015,                     // Segment length = 21
    0x00003006,                     //   HW sparse region (address/value pairs, count = 7)
    0x1098AAAA,                     //     LRFDPBE.MDMCMDPAR2
    0x20B80015,                     //     LRFDMDM.SYSTIMEVTMUX0
    0x20F40087,                     //     LRFDMDM.DEMDSBU
    0x2134005F,                     //     LRFDMDM.DEMSWQU0
    0x22A00003,                     //     LRFDMDM.BAUDCOMP
    0x232C7F00,                     //     LRFDMDM.DEMC1BE12
    0x30A81F40,                     //     LRFDRFE.SPARE3
    0x246C2000,                     //   HW 32-bit region (start address = 0x246C, count = 1)
    0x0000000C,                     //     LRFDMDM.MODMAIN               LRFDMDM.BAUDPRE
    0x20E41001,                     //   HW 16-bit region (start address = 0x20E4, count = 2)
    0x00FB0001,                     //     LRFDMDM.DEMMISC2              LRFDMDM.DEMMISC1
    0x21081006,                     //   HW 16-bit region (start address = 0x2108, count = 7)
    0x04000005,                     //     LRFDMDM.DEMMAFI0              LRFDMDM.DEMFIFE0
    0x00C17B20,                     //     LRFDMDM.DEMMAFI2              LRFDMDM.DEMMAFI1
    0x7F27000F,                     //     LRFDMDM.DEMC1BE1              LRFDMDM.DEMC1BE0
    0x0000017F,                     //     -                             LRFDMDM.DEMC1BE2
    0x24D42000,                     //   HW 32-bit region (start address = 0x24D4, count = 1)
    0x00540004,                     //     LRFDMDM.DEMD2XB0              LRFDMDM.DEMDSXB0
    0x68486000,                     //   RAM 32-bit region (start address = 0x6848, count = 1)
    0x00000045                      //     RFE_COMMON_RAM.AGCINFO        RFE_COMMON_RAM.SPARE1SHADOW
};

// Configuration: Sub-PHY = 1 Mbps, Coded
static const uint32_t LRF_subPhy1MbpsCodedRegConfigBleGen[] =
{
    0x00014009,                     // Segment length = 9
    0x0000A001,                     //   Data structure 32-bit region (start byte position = 0, count = 2)
    (uint32_t) &LRF_swConfig1Mbps_ble_gen_3,//     LRF_swParam : swConfig
    (uint32_t) &LRF_txPowerLimitTable1Mbps_ble_gen_3,//     LRF_swParam : txPowerLimitTable
    0x00003002,                     //   HW sparse region (address/value pairs, count = 3)
    0x20D44000,                     //     LRFDMDM.BAUD
    0x20E00387,                     //     LRFDMDM.DEMMISC0
    0x21242318,                     //     LRFDMDM.SPARE0
    0x00007000,                     //   RAM sparse region (address/value pairs, count = 1)
    0x68440051                      //     RFE_COMMON_RAM.PHYRSSIOFFSET
};

// Configuration: Sub-PHY = 2 Mbps, Coded
static const uint32_t LRF_subPhy2MbpsCodedRegConfigBleGen[] =
{
    0x00030002,                     // Segment length = 2
    0x00003000,                     //   HW sparse region (address/value pairs, count = 1)
    0x21040A24                      //     LRFDMDM.DEMFEXB0
};

// Configuration: Sub-PHY = 1 Mbps
static const uint32_t LRF_subPhy1MbpsRegConfigBleGen[] =
{
    0x0003400B,                     // Segment length = 11
    0x00003006,                     //   HW sparse region (address/value pairs, count = 7)
    0x10908007,                     //     LRFDPBE.MDMCMDPAR0
    0x10E803E8,                     //     LRFDPBE.TIMPER1
    0x20C40007,                     //     LRFDMDM.MODPRECTRL
    0x20EC1182,                     //     LRFDMDM.DEMMISC3
    0x20FC0F50,                     //     LRFDMDM.DEMCODC0
    0x21040224,                     //     LRFDMDM.DEMFEXB0
    0x30A4A246,                     //     LRFDRFE.SPARE2
    0x00007001,                     //   RAM sparse region (address/value pairs, count = 2)
    0x20200000,                     //     PBE_GENERIC_RAM.PHY
    0x684675F8                      //     RFE_COMMON_RAM.SPARE0SHADOW
};

// Configuration: Sub-PHY = 2 Mbps
static const uint32_t LRF_subPhy2MbpsRegConfigBleGen[] =
{
    0x00010010,                     // Segment length = 16
    0x0000A001,                     //   Data structure 32-bit region (start byte position = 0, count = 2)
    (uint32_t) &LRF_swConfig2Mbps_ble_gen_3,//     LRF_swParam : swConfig
    (uint32_t) &LRF_txPowerLimitTable2Mbps_ble_gen_3,//     LRF_swParam : txPowerLimitTable
    0x00003007,                     //   HW sparse region (address/value pairs, count = 8)
    0x1090800F,                     //     LRFDPBE.MDMCMDPAR0
    0x20C40017,                     //     LRFDMDM.MODPRECTRL
    0x20D48000,                     //     LRFDMDM.BAUD
    0x20E00399,                     //     LRFDMDM.DEMMISC0
    0x20EC1081,                     //     LRFDMDM.DEMMISC3
    0x20FC0F70,                     //     LRFDMDM.DEMCODC0
    0x2124232C,                     //     LRFDMDM.SPARE0
    0x30A4A357,                     //     LRFDRFE.SPARE2
    0x00007000,                     //   RAM sparse region (address/value pairs, count = 1)
    0x20200009,                     //     PBE_GENERIC_RAM.PHY
    0x68446000,                     //   RAM 32-bit region (start address = 0x6844, count = 1)
    0x55FA004C                      //     RFE_COMMON_RAM.SPARE0SHADOW   RFE_COMMON_RAM.PHYRSSIOFFSET
};

// Configuration: Sub-PHY = Coded
static const uint32_t LRF_subPhyCodedRegConfigBleGen[] =
{
    0x00020023,                     // Segment length = 35
    0x00003006,                     //   HW sparse region (address/value pairs, count = 7)
    0x10900097,                     //     LRFDPBE.MDMCMDPAR0
    0x10983C3C,                     //     LRFDPBE.MDMCMDPAR2
    0x20C40097,                     //     LRFDMDM.MODPRECTRL
    0x20F4008D,                     //     LRFDMDM.DEMDSBU
    0x20FC0670,                     //     LRFDMDM.DEMCODC0
    0x2134001F,                     //     LRFDMDM.DEMSWQU0
    0x22A00007,                     //     LRFDMDM.BAUDCOMP
    0x246C2000,                     //   HW 32-bit region (start address = 0x246C, count = 1)
    0x000CA00C,                     //     LRFDMDM.MODMAIN               LRFDMDM.BAUDPRE
    0x20E41002,                     //   HW 16-bit region (start address = 0x20E4, count = 3)
    0x00DF0003,                     //     LRFDMDM.DEMMISC2              LRFDMDM.DEMMISC1
    0x00003582,                     //     -                             LRFDMDM.DEMMISC3
    0x24842002,                     //   HW 32-bit region (start address = 0x2484, count = 3)
    0x0000000C,                     //     LRFDMDM.DEMMAFI0              LRFDMDM.DEMFIFE0
    0x00505014,                     //     LRFDMDM.DEMMAFI2              LRFDMDM.DEMMAFI1
    0x7F7F001F,                     //     LRFDMDM.DEMC1BE1              LRFDMDM.DEMC1BE0
    0x24D42000,                     //   HW 32-bit region (start address = 0x24D4, count = 1)
    0x00560024,                     //     LRFDMDM.DEMD2XB0              LRFDMDM.DEMDSXB0
    0x22B41001,                     //   HW 16-bit region (start address = 0x22B4, count = 2)
    0x7C404040,                     //     LRFDMDM.DEMFB2P1              LRFDMDM.DEMFB2P0
    0x25682001,                     //   HW 32-bit region (start address = 0x2568, count = 2)
    0xFD90030C,                     //     LRFDMDM.DEMPHAC1              LRFDMDM.DEMPHAC0
    0x0000783C,                     //     LRFDMDM.DEMPHAC3              LRFDMDM.DEMPHAC2
    0x23241002,                     //   HW 16-bit region (start address = 0x2324, count = 3)
    0x7F7F8804,                     //     LRFDMDM.DEMC1BE11             LRFDMDM.DEMC1BE10
    0x00002C7F,                     //     -                             LRFDMDM.DEMC1BE12
    0x344C2000,                     //   HW 32-bit region (start address = 0x344C, count = 1)
    0x36F21307,                     //     LRFDRFE.SPARE0                LRFDRFE.MAGNCTL1
    0x30A41001,                     //   HW 16-bit region (start address = 0x30A4, count = 2)
    0x00B00015,                     //     LRFDRFE.SPARE3                LRFDRFE.SPARE2
    0x00007000,                     //   RAM sparse region (address/value pairs, count = 1)
    0x20200012,                     //     PBE_GENERIC_RAM.PHY
    0x68486000,                     //   RAM 32-bit region (start address = 0x6848, count = 1)
    0x00010027                      //     RFE_COMMON_RAM.AGCINFO        RFE_COMMON_RAM.SPARE1SHADOW
};

// Configuration: Coded TX rate = S8 (8 symbols per bit = 125 kbps)
static const uint32_t LRF_codedTxRateS8RegConfigBleGen[] =
{
    0x00044002,                     // Segment length = 2
    0x00003000,                     //   HW sparse region (address/value pairs, count = 1)
    0x21280000                      //     LRFDMDM.SPARE1
};

// Configuration: Coded TX rate = S2 (2 symbols per bit = 500 kbps)
static const uint32_t LRF_codedTxRateS2RegConfigBleGen[] =
{
    0x00040002,                     // Segment length = 2
    0x00003000,                     //   HW sparse region (address/value pairs, count = 1)
    0x21280001                      //     LRFDMDM.SPARE1
};


// LRF register configuration list
static const LRF_RegConfigList LRF_regConfigList_ble_gen_3 = {
    .numEntries = 9,
    .entries = {
        (LRF_ConfigWord*)LRF_commonRegConfigBleGen,
        (LRF_ConfigWord*)LRF_subPhy1Mbps2MbpsRegConfigBleGen,
        (LRF_ConfigWord*)LRF_subPhy1MbpsCodedRegConfigBleGen,
        (LRF_ConfigWord*)LRF_subPhy2MbpsCodedRegConfigBleGen,
        (LRF_ConfigWord*)LRF_subPhy1MbpsRegConfigBleGen,
        (LRF_ConfigWord*)LRF_subPhy2MbpsRegConfigBleGen,
        (LRF_ConfigWord*)LRF_subPhyCodedRegConfigBleGen,
        (LRF_ConfigWord*)LRF_codedTxRateS8RegConfigBleGen,
        (LRF_ConfigWord*)LRF_codedTxRateS2RegConfigBleGen
    }
};

// LRF_TxShape data structure
const LRF_TxShape LRF_shapeBaseGfsk067_ble_gen_3 = {
    .scale = 0x0FDE2,
    .numCoeff = 0x0011,
    .coeff = { 0x01, 0x02, 0x05, 0x0A, 0x14, 0x22, 0x37, 0x52, 0x71, 0x91, 0xB0, 0xCB, 0xE0, 0xEE, 0xF8, 0xFD, 0xFF}
};

// LRF_TxShape data structure
const LRF_TxShape LRF_shapeBaseGfsk05_ble_gen_3 = {
    .scale = 0x0F183,
    .numCoeff = 0x0014,
    .coeff = { 0x01, 0x02, 0x03, 0x06, 0x0A, 0x11, 0x1A, 0x27, 0x37, 0x4B, 0x62, 0x7B, 0x94, 0xAD, 0xC4, 0xD8, 0xE7, 0xF3, 0xFB, 0xFF}
};

// LRF_SwConfig data structure
const LRF_SwConfig LRF_swConfig1Mbps_ble_gen_3 = {
    .rxIntFrequency = 1000000,
    .rxFrequencyOffset = 0,
    .txFrequencyOffset = 0,
    .modFrequencyDeviation = 0x0003D090,
    .txShape = &LRF_shapeBaseGfsk067_ble_gen_3,
    .bwIndex = 0x00,
    .bwIndexDither = 0x01
};

// LRF_SwConfig data structure
const LRF_SwConfig LRF_swConfig2Mbps_ble_gen_3 = {
    .rxIntFrequency = 850000,
    .rxFrequencyOffset = 0,
    .txFrequencyOffset = 0,
    .modFrequencyDeviation = 0x0007A120,
    .txShape = &LRF_shapeBaseGfsk05_ble_gen_3,
    .bwIndex = 0x01,
    .bwIndexDither = 0x01
};

// LRF_TxPowerLimitTable data structure
const LRF_TxPowerLimitTable LRF_txPowerLimitTable1Mbps_ble_gen_3 = {
    .numEntries = 0x00000005,
    .freqDiv = 0x000F4240,
    .limitTable = {
        { .minFreq = 2401, .maxFreq = 2403, .regulatoryMask = 0x01, .maxTxPower = { .fraction = 0, .dBm = 15 } },
        { .minFreq = 2401, .maxFreq = 2403, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 16 } },
        { .minFreq = 2475, .maxFreq = 2477, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 15 } },
        { .minFreq = 2477, .maxFreq = 2479, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 14 } },
        { .minFreq = 2479, .maxFreq = 2481, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 10 } }
    }
};

// LRF_TxPowerLimitTable data structure
const LRF_TxPowerLimitTable LRF_txPowerLimitTable2Mbps_ble_gen_3 = {
    .numEntries = 0x00000003,
    .freqDiv = 0x000F4240,
    .limitTable = {
        { .minFreq = 2473, .maxFreq = 2475, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 17 } },
        { .minFreq = 2475, .maxFreq = 2477, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 16 } },
        { .minFreq = 2477, .maxFreq = 2479, .regulatoryMask = 0x04, .maxTxPower = { .fraction = 0, .dBm = 13 } }
    }
};

// LRF_Config data structure
const LRF_Config LRF_config_ble_gen_3 = {
    .pbeImage = (const LRF_TOPsmImage*) LRF_PBE_binary_generic,
    .mceImage = (const LRF_TOPsmImage*) LRF_MCE_binary_ble5_phac,
    .rfeImage = (const LRF_TOPsmImage*) LRF_RFE_binary_ble5,
    .regConfigList = &LRF_regConfigList_ble_gen_3
};



