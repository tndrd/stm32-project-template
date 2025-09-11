#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stm32f407xx.h"

#define LED_PIN 1

void sleep(size_t ticks)
{
  for (size_t i = 0; i < ticks; i++);
}

void main(void)
{

  RCC->AHB1ENR |= (1 << RCC_AHB1ENR_GPIOAEN_Pos);
  
  // Errata
  volatile uint32_t dummy;
  dummy = RCC->AHB1ENR;
  dummy = RCC->AHB1ENR;

  GPIOA->MODER |= (1 << GPIO_MODER_MODER1_Pos);
  
  while(1)
  {
    for(int i = 0; i < 10; ++i)
    {
      GPIOA->ODR ^= (1 << LED_PIN);
      sleep(150000);
    }
    sleep(1500000);
  }
}