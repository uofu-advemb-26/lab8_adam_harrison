#include "portmacro.h"
#include "projdefs.h"
#include <can2040.h>
#include <hardware/regs/intctrl.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

static struct can2040 cbus;
QueueHandle_t queue;

static void can2040_cb(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg)
{
    if (notify != CAN2040_NOTIFY_RX) return;
    xQueueSendToBackFromISR(queue, msg, NULL);
}

static void PIOx_IRQHandler(void)
{
    can2040_pio_irq_handler(&cbus);
}

void canbus_setup(void)
{
    uint32_t pio_num = 0;
    uint32_t sys_clock = 125000000, bitrate = 500000;
    uint32_t gpio_rx = 4, gpio_tx = 5;

    // Setup canbus
    can2040_setup(&cbus, pio_num);
    can2040_callback_config(&cbus, can2040_cb);

    // Enable irqs
    irq_set_exclusive_handler(PIO0_IRQ_0, PIOx_IRQHandler);
    irq_set_priority(PIO0_IRQ_0, PICO_DEFAULT_IRQ_PRIORITY - 1);
    irq_set_enabled(PIO0_IRQ_0, 1);

    // Start canbus
    can2040_start(&cbus, sys_clock, bitrate, gpio_rx, gpio_tx);
}

void main_task(__unused void *params)
{
    struct can2040_msg msg;
    printf("Main task begin\n");

    while(1)
    {
    if(xQueueReceive(queue, &msg, portMAX_DELAY) == pdTRUE)
        {
        printf("Message received\n");
        printf("id=0x%03lx dlc=%lu data:",
            (unsigned long)(msg.id & 0x1FFFFFFF), (unsigned long)msg.dlc);
        for(uint32_t i = 0; i < msg.dlc && i < 8; i++)
            printf(" %02x", msg.data[i]);
        printf("\n");
        }
    else{
        printf("No message received\n");
    }
    }
}

int main( void ) 
{
    stdio_init_all();
    queue = xQueueCreate(100, sizeof(struct can2040_msg));
    canbus_setup();
    TaskHandle_t task;
    xTaskCreate(main_task, "MainThread",
                configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &task);
    vTaskStartScheduler();
    return 0;
}
