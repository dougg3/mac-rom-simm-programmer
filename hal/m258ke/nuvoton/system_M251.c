/**************************************************************************//**
 * @file     system_M251.c
 * @version  V0.10
 * @brief    System Setting Source File
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ****************************************************************************/

#include <arm_cmse.h>
#include <stdio.h>
#include <stdint.h>
#include "NuMicro.h"

#if defined (__ARM_FEATURE_CMSE) &&  (__ARM_FEATURE_CMSE == 3U)
    #include "partition_M251.h"
#endif


extern void *__Vectors;                   /* see startup file */

/*----------------------------------------------------------------------------
  Clock Variable definitions
 *----------------------------------------------------------------------------*/
uint32_t SystemCoreClock  = __HSI;              /*!< System Clock Frequency (Core Clock) */
uint32_t CyclesPerUs      = (__HSI / 1000000);  /*!< Cycles per micro second             */
uint32_t PllClock         = __HSI;              /*!< PLL Output Clock Frequency          */
const uint32_t gau32ClkSrcTbl[8] = {__HXT, __LXT, 0UL, __LIRC, 0UL, __MIRC, 0UL, __HIRC};


/**
 * @brief    Update the Variable SystemCoreClock
 *
 * @details  This function is used to update the variable SystemCoreClock
 *           and must be called whenever the core clock is changed.
 */
void SystemCoreClockUpdate(void)
{
    uint32_t u32Freq, u32ClkSrc;
    uint32_t u32HclkDiv;

    u32ClkSrc = CLK->CLKSEL0 & CLK_CLKSEL0_HCLKSEL_Msk;

    /* Update PLL Clock */
    PllClock = CLK_GetPLLClockFreq();

    if (u32ClkSrc != CLK_CLKSEL0_HCLKSEL_PLL)
    {
        /* Use the clock sources directly */
        u32Freq = gau32ClkSrcTbl[u32ClkSrc];
    }
    else
    {
        /* Use PLL clock */
        u32Freq = PllClock;
    }

    u32HclkDiv = (CLK->CLKDIV0 & CLK_CLKDIV0_HCLKDIV_Msk) + 1;

    /* Update System Core Clock */
    SystemCoreClock = u32Freq / u32HclkDiv;

    CyclesPerUs = (SystemCoreClock + 500000) / 1000000;
}



/**
 * @brief    System Initialization
 *
 * @details  The necessary initialization of system. Global variables are forbidden here.
 */
void SystemInit(void)
{
    /* Set access cycle for CPU @ 48MHz */
    FMC->CYCCTL = (FMC->CYCCTL & ~FMC_CYCCTL_CYCLE_Msk) | (3 << FMC_CYCCTL_CYCLE_Pos) | 0x100;

#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
    SCB->VTOR = (uint32_t) &__Vectors;
#endif

#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
    TZ_SAU_Setup();
    SCU_Setup();
    FMC_NSBA_Setup();
#endif

#ifdef INIT_SYSCLK_AT_BOOTING

#endif

}


#if USE_ASSERT

/**
 * @brief      Assert Error Message
 *
 * @param[in]  file  the source file name
 * @param[in]  line  line number
 *
 * @details    The function prints the source file name and line number where
 *             the ASSERT_PARAM() error occurs, and then stops in an infinite loop.
 */
void AssertError(uint8_t *file, uint32_t line)
{

    printf("[%s] line %u : wrong parameters.\r\n", file, line);

    /* Infinite loop */
    while (1) ;
}
#endif

/**
 * @brief    Set UART0 Default MPF
 *
 * @details  The initialization of uart0 default multi function pin.
 */
#if defined( __ICCARM__ )
    __WEAK
#else
    __attribute__((weak))
#endif
void Uart0DefaultMPF(void)
{

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~SYS_GPB_MFPH_PB12MFP_Msk) | SYS_GPB_MFPH_PB12MFP_UART0_RXD;
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~SYS_GPB_MFPH_PB13MFP_Msk) | SYS_GPB_MFPH_PB13MFP_UART0_TXD;

}
