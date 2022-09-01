
#include <fpioa.h>
#include <gpio.h>
#include <sysctl.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <stdbool.h>
#include <stdio.h>



void LedTask(void* p)
{
	QueueHandle_t queue = (QueueHandle_t)p;
	
	gpio_init();
    	fpioa_set_function(13, FUNC_GPIO3);
    	gpio_set_drive_mode(3, GPIO_DM_OUTPUT);
    	gpio_set_pin(3, GPIO_PV_LOW);

    	int egoera=0;

    	fpioa_set_function(12, FUNC_GPIO4);
    	fpioa_set_function(14, FUNC_GPIO5);
    	gpio_set_drive_mode(5, GPIO_DM_OUTPUT);
    	gpio_set_drive_mode(4, GPIO_DM_OUTPUT);
    	gpio_set_pin(5, GPIO_PV_HIGH);
    	gpio_set_pin(4, GPIO_PV_HIGH);

	while(true)
	{
		bool state;
		xQueueReceive(queue, &state, portMAX_DELAY);
		switch(egoera){

		    case 0://gorria
			//pin=gpio_get_pin(13);
			gpio_set_pin(3, GPIO_PV_HIGH);
			//pin=gpio_get_pin(14);
			gpio_set_pin(5, GPIO_PV_LOW);//urdina jarri
			egoera=1;
			break;

		    case 1://urdina
			//pin=gpio_get_pin(14);
			gpio_set_pin(5, GPIO_PV_HIGH);
			//pin=gpio_get_pin(12);
			gpio_set_pin(4, GPIO_PV_LOW);//berdea jarri
			egoera=2;
			break;

		    case 2://urdina
			//pin=gpio_get_pin(12);
			gpio_set_pin(4, GPIO_PV_HIGH);
			//pin=gpio_get_pin(13);
			gpio_set_pin(3, GPIO_PV_LOW);//gorria jarri
			egoera=0;
			break;

		}
	}
}
