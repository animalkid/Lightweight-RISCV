#include <fpioa.h>
#include <gpio.h>
#include <uarths.h>
#include <sysctl.h>
#include <i2s.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>


#include "led.h"
#include "lcd.h"
#include "nt35310.h"
#include "board_config.h"
#include "image.h"

#define FRAME_LEN   512

int16_t rx_buf[FRAME_LEN * 2 * 2];
uint32_t g_index;
uint32_t g_tx_len;

uint32_t g_rx_dma_buf[FRAME_LEN * 2 * 2];
uint8_t i2s_rec_flag;
uint32_t g_lcd_gram[LCD_X_MAX * LCD_Y_MAX / 2] __attribute__((aligned(128)));




void ControlTask(void* p)
{
	QueueHandle_t queue = (QueueHandle_t)p;

	bool state = false;
	while(true)
	{
		xQueueSend(queue, &state, portMAX_DELAY);
		state = !state;
		vTaskDelay(500);
	}
}


static void io_set_power(void)
{
#if BOARD_LICHEEDAN
    sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
#else
    sysctl_set_power_mode(SYSCTL_POWER_BANK1, SYSCTL_POWER_V18);
#endif
}

static void io_mux_init(void)
{
#if BOARD_LICHEEDAN
    fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(36, FUNC_SPI0_SS3);
    fpioa_set_function(39, FUNC_SPI0_SCLK);
    fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);
    sysctl_set_spi0_dvp_data(1);
#else
    fpioa_set_function(8, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(6, FUNC_SPI0_SS3);
    fpioa_set_function(7, FUNC_SPI0_SCLK);
    sysctl_set_spi0_dvp_data(1);
#endif
}

void LCDTask(void* p){

	QueueHandle_t queue = (QueueHandle_t)p;
	bool state;

	//uint32_t g_lcd_gram[LCD_X_MAX * LCD_Y_MAX / 2] __attribute__((aligned(128)));
	
	//gpio_set_pin(3, GPIO_PV_HIGH);
	//gpio_set_pin(5, GPIO_PV_LOW);//berdea jarri
    	
/*
#if BOARD_LICHEEDAN
    	fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
    	fpioa_set_function(36, FUNC_SPI0_SS3);
    	fpioa_set_function(39, FUNC_SPI0_SCLK);
    	fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);
    	sysctl_set_spi0_dvp_data(1);
#else
    	fpioa_set_function(8, FUNC_GPIOHS0 + DCX_GPIONUM);
    	fpioa_set_function(6, FUNC_SPI0_SS3);
    	fpioa_set_function(7, FUNC_SPI0_SCLK);
    	sysctl_set_spi0_dvp_data(1);
#endif
    	
#if BOARD_LICHEEDAN
    	sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
    	sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
#else
    	sysctl_set_power_mode(SYSCTL_POWER_BANK1, SYSCTL_POWER_V18);
#endif	
*/	

	io_mux_init();
	io_set_power();
	lcd_init();
		

#if BOARD_LICHEEDAN
    	lcd_set_direction(DIR_YX_RLDU);
#endif

	lcd_clear(RED);
    lcd_draw_picture(0, 0, 320, 240, rgb_image);
	/*for (int i=0;i<320;i++){
		for (int j=0;j<240;j++)
			lcd_draw_point(i, j, galardo[j+(i*320)]);
	}*/
    //lcd_draw_picture(0, 0, 240, 320, galardo);
    	//lcd_draw_string(16, 20, "Canaan", RED);
    	//lcd_draw_string(16, 30, "Kendryte K210", BLUE);	
    	//lcd_draw_string(16, 80, "HEYOU", GREEN);	
	
	while(true){

		xQueueReceive(queue, &state, portMAX_DELAY);
	}


}

int i2s_dma_irq(void *ctx)
{
    uint32_t i;

    if(g_index)
    {
        
        i2s_receive_data_dma(I2S_DEVICE_0, &g_rx_dma_buf[g_index], FRAME_LEN * 2, DMAC_CHANNEL1);
        g_index = 0;
        for(i = 0; i < FRAME_LEN; i++)
        {
            rx_buf[2 * i] = (int16_t)(g_rx_dma_buf[2 * i + 1] & 0xffff);
            rx_buf[2 * i + 1] = (int16_t)(g_rx_dma_buf[2 * i + 1] & 0xffff);
        }
        i2s_rec_flag = 1;
    }
    else
    {
        i2s_receive_data_dma(I2S_DEVICE_0, &g_rx_dma_buf[0], FRAME_LEN * 2, DMAC_CHANNEL1);
        g_index = FRAME_LEN * 2;
        for(i = FRAME_LEN; i < FRAME_LEN * 2; i++)
        {
            rx_buf[2 * i] = (int16_t)(g_rx_dma_buf[2 * i + 1] & 0xffff);
            rx_buf[2 * i + 1] = (int16_t)(g_rx_dma_buf[2 * i + 1] & 0xffff);
        }
        i2s_rec_flag = 2;
    }
    return 0;
}

void PWMTask(void* p){
	QueueHandle_t queue = (QueueHandle_t)p;
	bool state;
	
	sysctl_pll_set_freq(SYSCTL_PLL0, 320000000UL);
    sysctl_pll_set_freq(SYSCTL_PLL1, 160000000UL);
    sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);
	
	uarths_init();
	
    fpioa_set_function(20, FUNC_I2S0_IN_D0);
    fpioa_set_function(19, FUNC_I2S0_WS);
    fpioa_set_function(18, FUNC_I2S0_SCLK);

    fpioa_set_function(33, FUNC_I2S2_OUT_D1);
    fpioa_set_function(35, FUNC_I2S2_SCLK);
    fpioa_set_function(34, FUNC_I2S2_WS);

	g_index = 0;
    g_tx_len = 0;
	
	i2s_init(I2S_DEVICE_0, I2S_RECEIVER, 0x3);
    i2s_init(I2S_DEVICE_2, I2S_TRANSMITTER, 0xC);
	
	i2s_rx_channel_config(I2S_DEVICE_0, I2S_CHANNEL_0,
        RESOLUTION_16_BIT, SCLK_CYCLES_32,
        TRIGGER_LEVEL_4, STANDARD_MODE);

    i2s_tx_channel_config(I2S_DEVICE_2, I2S_CHANNEL_1,
        RESOLUTION_16_BIT, SCLK_CYCLES_32,
        TRIGGER_LEVEL_4,
        RIGHT_JUSTIFYING_MODE
        );
    i2s_set_sample_rate(I2S_DEVICE_0, 16000);
    i2s_set_sample_rate(I2S_DEVICE_2, 16000);
    plic_init();
    dmac_init();
    dmac_set_irq(DMAC_CHANNEL1, i2s_dma_irq, NULL, 4);
	
	i2s_receive_data_dma(I2S_DEVICE_0, &rx_buf[g_index], FRAME_LEN * 2, DMAC_CHANNEL1);

	while (1)
    {
		xQueueReceive(queue, &state, portMAX_DELAY);
        if(i2s_rec_flag == 1)
        {
			//lcd_draw_string(16, 40, ("i", rx_buf), RED);
            i2s_play(I2S_DEVICE_2,
            DMAC_CHANNEL0, (uint8_t *)(&rx_buf[0]), FRAME_LEN * 4, 1024, 16, 2);
            i2s_rec_flag = 0;
        }
        else if(i2s_rec_flag == 2)
        {
			//lcd_draw_string(16, 60, (char*)rx_buf, RED);	
            i2s_play(I2S_DEVICE_2,
            DMAC_CHANNEL0, (uint8_t *)(&rx_buf[FRAME_LEN * 2]), FRAME_LEN * 4, 1024, 16, 2);
            i2s_rec_flag = 0;
        }
    }
	
	
	
}
	
	
int main()
{
	sysctl_pll_set_freq(SYSCTL_PLL0, configCPU_CLOCK_HZ * 2);
	QueueHandle_t queue = xQueueCreate( 10, sizeof(bool) );

	printf("Create tasks...\r\n");
	xTaskCreate(LedTask, "LedTask", 256, queue, 3, NULL);
	xTaskCreate(ControlTask, "ControlTask", 256, queue, 3, NULL);
	xTaskCreate(LCDTask, "LCDTask", 256, queue, 3, NULL);
	xTaskCreate(PWMTask, "PWMTask", 256, queue, 3, NULL);
	
	printf("Start scheduler...\r\n");
	vTaskStartScheduler();
}
