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
#include "mprime.h"
#include "vectors.h"

/* Macros */
#define OVERLAY_SIZE 3072U
#define RSA_ITERS 10U
#define RSA_SIZE 256U
#define PRIME_ITERS 1000U

/* Externs */
extern uint8_t __ovl_vma_start;
extern uint8_t __ovl_rsa_lma_start;
extern uint8_t __ovl_rsa_lma_end;
extern uint8_t __ovl_prime_lma_start;
extern uint8_t __ovl_prime_lma_end;

/* Function Prototypes */
void SystemClock_Config(void);
int _write(int fd, const char* buf, int size);
static void overlay_load_rsa(void);
static void overlay_load_prime(void);
static uint32_t rsa_bench(size_t iters);
static uint32_t prime_bench(size_t iters);
int main(void)
{
  // System init
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();

  printf("\r\nSystem Init @ %lu Hz\r\n", SystemCoreClock);

  //Load RSA overlay
  overlay_load_rsa();
  printf("RSA Overlay: %lu bytes @ %p -> %p\r\n",
               (unsigned long)(&__ovl_rsa_lma_end - &__ovl_rsa_lma_start),
               &__ovl_rsa_lma_start,
               &__ovl_vma_start);

  uint8_t tmp[RSA_SIZE];
  memcpy(tmp, M0_be, RSA_SIZE);
  (void)br_rsa_i15_public(tmp, RSA_SIZE, &pk);

  uint32_t t_rsa = rsa_bench(RSA_ITERS);
  uint32_t us_per_rsa = (t_rsa + RSA_ITERS/2) / RSA_ITERS;

  printf("RSA2048 (overlay): iters=%lu total_us=%lu, us/op=%lu\r\n",
         (unsigned long)RSA_ITERS, (unsigned long)t_rsa, (unsigned long)us_per_rsa);

  //load prime overlay
  overlay_load_prime();
  printf("Prime Overlay: %lu bytes @ %p -> %p\r\n",
               (unsigned long)(&__ovl_prime_lma_end - &__ovl_prime_lma_start),
               &__ovl_prime_lma_start,
               &__ovl_vma_start);

  uint32_t t_prime = prime_bench(PRIME_ITERS);
  uint32_t us_per_prime = (t_prime + PRIME_ITERS/2) / PRIME_ITERS;

  printf("mPrime (overlay): iters=%lu total_us=%lu, us/iter=%lu\r\n",
         (unsigned long)PRIME_ITERS,
         (unsigned long)t_prime,
         (unsigned long)us_per_prime);

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

//overlay loader
static void overlay_load(const uint8_t* lma_start, const uint8_t* lma_end) {
  size_t size = lma_end - lma_start;
  memcpy(&__ovl_vma_start, lma_start, size);
}

//rsa overlay load
static void overlay_load_rsa(void) {
  overlay_load(&__ovl_rsa_lma_start, &__ovl_rsa_lma_end);
}

//prime overlay load
static void overlay_load_prime(void) {
  overlay_load(&__ovl_prime_lma_start, &__ovl_prime_lma_end);
}

// rsa benchmark
static uint32_t rsa_bench(size_t iters) {
  uint8_t work[RSA_SIZE];

  LL_TIM_SetCounter(TIM2, 0);
  LL_TIM_EnableCounter(TIM2);

  for (size_t i = 0; i < iters; i++) {
    memcpy(work, M0_be, sizeof work);
    work[127] ^= (uint8_t)i;  // vary input but keep < N

    
    uint32_t ok = br_rsa_i15_public(work, sizeof work, &pk);
    __asm__ volatile("" :: "r"(ok), "r"(work[127]) : "memory");
  }

  return LL_TIM_GetCounter(TIM2);  // us
}

// mprime benchmark
static uint32_t prime_bench(size_t iters) {
  LL_TIM_SetCounter(TIM2, 0);
  LL_TIM_EnableCounter(TIM2);

  volatile int acc = 0;

  for (size_t i = 0; i < iters; i++) {
      acc += ll_test_M127();
  }

  // Prevent optimizer from deleting the loop
  __asm__ volatile ("" :: "r"(acc) : "memory");

  return LL_TIM_GetCounter(TIM2);
}