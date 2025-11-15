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
#include "bearssl_ec.h"
#include "vectors.h"

/* Macros */
#define OVERLAY_SIZE 2048U
#define RSA_ITERS 10U
#define RSA_SIZE 256U

/* Externs */
extern uint8_t __ovl_vma_start;
extern uint8_t __ovl_rsa_lma_start;
extern uint8_t __ovl_rsa_lma_end;

/* Function Prototypes */
void SystemClock_Config(void);
int _write(int fd, const char* buf, int size);
static void overlay_load_rsa(void);
static uint32_t c25519_bench(size_t iters);
static uint32_t rsa_bench(size_t iters);
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

  overlay_load_rsa();
  printf("Overlay: %lu bytes @ %p -> %p\r\n",
               (unsigned long)(&__ovl_rsa_lma_end - &__ovl_rsa_lma_start),
               &__ovl_rsa_lma_start, &__ovl_vma_start);
  printf("Function pointer for montmult: %p\r\n", (void*)br_i15_montymul);

  uint8_t tmp[RSA_SIZE];
  memcpy(tmp, M0_be, RSA_SIZE);
  (void)br_rsa_i15_public(tmp, RSA_SIZE, &pk);

  uint32_t t_i15  = rsa_bench(RSA_ITERS);

  // us/op with simple rounding
  uint32_t us_per_i15  = (t_i15  + RSA_ITERS/2) / RSA_ITERS;

  printf("RSA2048 public key operation, code from RAM: iters=%lu total_us: i15=%lu\r\n",
         (unsigned long)RSA_ITERS, (unsigned long)t_i15);
  printf("us/op: rsa2048 i15 = %lu\r\n", (unsigned long)us_per_i15);

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

//overlay for rsa
static void overlay_load_rsa(void) {
  size_t size = &__ovl_rsa_lma_end - &__ovl_rsa_lma_start;
  memcpy(&__ovl_vma_start, &__ovl_rsa_lma_start, size);
}

// ecdsa benchmark
static uint32_t c25519_bench(size_t iters) {
    volatile uint8_t sink = 0;
    LL_TIM_SetCounter(TIM2, 0);
    LL_TIM_EnableCounter(TIM2);

    for (size_t i = 0; i < iters; i++) {
        //in place
        memcpy(C25519_SHARED, C25519_BOB_PUB, C25519_SIZE);
        int ok = br_ec_c25519_m15.mul(C25519_SHARED, C25519_SIZE,
                                      C25519_ALICE_PRIV, C25519_SIZE,
                                      BR_EC_curve25519);
        sink ^= C25519_SHARED[0] ^ (uint8_t)ok;
    }
    (void)sink;
    return LL_TIM_GetCounter(TIM2);  // assuming TIM2 = 1 MHz → microseconds
}