/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "diag/trace.h"

#include "timer.h"
#include "led.h"
//===========================================
#include "stm32f4xx_hal.h"
#include "stm32f407xx.h"

// ----------------------------------------------------------------------------
//
// Standalone STM32F4 led blink sample (trace via DEBUG).
//
// In debug configurations, demonstrate how to print a greeting message
// on the trace device. In release configurations the message is
// simply discarded.
//
// Then demonstrates how to blink a led with 1 Hz, using a
// continuous loop and SysTick delays.
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace-impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- Timing definitions -------------------------------------------------

// Keep the LED on for 2/3 of a second.
#define BLINK_ON_TICKS  (TIMER_FREQUENCY_HZ * 3 / 4)
#define BLINK_OFF_TICKS (TIMER_FREQUENCY_HZ - BLINK_ON_TICKS)
//===================================================

#define DHT11_GPIO_Port GPIOA
#define DHT11_PIN GPIO_PIN_0
#define FAN_GPIO_PORT GPIOD
#define FAN_PIN GPIO_PIN_15
//#define RCC_APB2Periph_ADC1             ((uint32_t)0x00000100)

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"
//===================================================
uint8_t dht11_read_byte(void);
uint8_t dht11_read_byte() {
    uint8_t data = 0;
    for (int i = 0; i < 8; ++i) {
        while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_PIN) == GPIO_PIN_RESET) {}
        HAL_Delay(30);
        if (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_PIN) == GPIO_PIN_SET) {
            data |= (1 << (7 - i));
        }
        while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_PIN) == GPIO_PIN_SET) {}
    }
    return data;
}

void read_dht11_data(float* temperature, float* humidity) {
    uint8_t data[5];
    int i;

    HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_PIN, GPIO_PIN_SET);

    HAL_Delay(40);

    if (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_PIN) == GPIO_PIN_RESET) {
        HAL_Delay(80);
        if (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_PIN) == GPIO_PIN_SET) {
            HAL_Delay(40);
            for (i = 0; i < 5; ++i) {
                data[i] = dht11_read_byte();
            }

            if ((data[0] + data[1] + data[2] + data[3]) == data[4]) {
                *humidity = data[0] + data[1] / 10.0;
                *temperature = data[2] + data[3] / 10.0;
            }
        }
    }
}


float temp = 28;
float humi = 75;
int m,n;
float A[6] = {23,24,28,24,32,34};

int
main(int argc, char* argv[])
{
  // Send a greeting to the trace device (skipped on Release).
  trace_puts("Hello Arm World!");

  // At this stage the system clock should have already been configured
  // at high speed.
  trace_printf("System clock: %u Hz\n", SystemCoreClock);
  timer_start();

  blink_led_init();

  uint32_t seconds = 0;

  // Infinite loop
  GPIO_InitTypeDef GPIO_InitStruct;
  /* Configure FAN Pin */
  GPIO_InitStruct.Pin = FAN_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FAN_GPIO_PORT, &GPIO_InitStruct);

  while (1)
    {
	  read_dht11_data(&temp, &humi);
	  trace_printf("Temp: %.1f\n", temp);
	  if(temp > 27){
	  		  blink_led_on();
	  		  timer_sleep(seconds == 0 ? TIMER_FREQUENCY_HZ : BLINK_ON_TICKS);
	  		  HAL_GPIO_WritePin(FAN_GPIO_PORT, FAN_PIN, GPIO_PIN_SET);
	  		  		  //turn on quat
	  		 }else{
	  		  HAL_GPIO_WritePin(FAN_GPIO_PORT, FAN_PIN, GPIO_PIN_RESET);//turn off
	  		  blink_led_off();
	  		  timer_sleep(BLINK_OFF_TICKS);

	  		 }
	  HAL_Delay(1000);
	  ++seconds;
	  // Count seconds on the trace device.
	  trace_printf("Second %u\n", seconds);
	 }

  // Infinite loop, never return.
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
