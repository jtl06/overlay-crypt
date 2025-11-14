// Overlay Demo
// jtl, 11/13/25

/* Includes */

#include <stdio.h>
#include <stdint.h>
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "bearssl_rsa.h"
#include "vectors.h"

/* Macros */
#define RSA_ITERS 10U

/* Function Prototypes */
void SystemClock_Config(void);
int _write(int fd, const char* buf, int size);
static uint32_t rsa_pub_bench(size_t iters);
void br_i15_montymul(uint16_t*, const uint16_t*, const uint16_t*, const uint16_t*, uint16_t);

int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();


  printf("\r\nSystem Init @ %lu Hz\r\n", SystemCoreClock);

  printf("No overlay, Function pointer for montmult: %p\r\n", (void*)br_i15_montymul);


  uint8_t tmp[256];
  memcpy(tmp, M0_be, 256);
  (void)br_rsa_i15_public(tmp, 256, &pk);

  const size_t ITERS = 10;   // adjust as you like (keep small on M0+)

  uint32_t t_i15  = rsa_pub_bench(ITERS);

  // us/op with simple rounding
  uint32_t us_per_i15  = (t_i15  + ITERS/2) / ITERS;

  printf("RSA2048 public key operation, code from RAM: iters=%lu total_us: i15=%lu\r\n",
         (unsigned long)ITERS, (unsigned long)t_i15);
  printf("us/op: i15 = %lu\r\n", (unsigned long)us_per_i15);

  while (1)
  {
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2)
  {
  }

  /* HSI configuration and activation */
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1)
  {
  }

  /* Main PLL configuration and activation */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 8, LL_RCC_PLLR_DIV_2);
  LL_RCC_PLL_Enable();
  LL_RCC_PLL_EnableDomain_SYS();
  while(LL_RCC_PLL_IsReady() != 1)
  {
  }

  /* Set AHB prescaler*/
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

  /* Sysclk activation on the main PLL */
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  }

  /* Set APB1 prescaler*/
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_8);

  LL_Init1msTick(64000000);

  /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
  LL_SetSystemCoreClock(64000000);
}

//printf to uart2
int _write(int fd, const char *buf, int size) {
  (void)fd;
  for (int i = 0; i < size; i++) {
    while (!LL_USART_IsActiveFlag_TXE(USART2)) { /* wait */ }
    LL_USART_TransmitData8(USART2, (uint8_t)buf[i]);
  }
  return size;
}

// rsa benchmark
static uint32_t rsa_pub_bench(size_t iters) {
  uint8_t work[256];

  LL_TIM_SetCounter(TIM2, 0);
  LL_TIM_EnableCounter(TIM2);

  for (size_t i = 0; i < iters; i++) {
    memcpy(work, M0_be, sizeof work);
    work[126] ^= (uint8_t)i;  // vary input but keep < N

    
    uint32_t ok = br_rsa_i15_public(work, sizeof work, &pk);
    __asm__ volatile("" :: "r"(ok), "r"(work[127]) : "memory");
  }

  return LL_TIM_GetCounter(TIM2);  // us
}