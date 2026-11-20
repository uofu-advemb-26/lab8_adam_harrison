#include <can2040.h>
#include <hardware/regs/intctrl.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

static struct can2040 cbus;
QueueHandle_t queue;
volatile uint32_t tx_done_count = 0;

static void can2040_cb(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg)
{
    if (notify == CAN2040_NOTIFY_TX) tx_done_count++;
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

    struct can2040_msg msg =
    {
        .id = 0x200,
        .dlc = 7,
        .data = {'M', 'e','s','s','a','g','e'}
    };

    vTaskDelay(5000);
    while(1)
    {
        int status;
        status = can2040_transmit(&cbus, &msg);
        vTaskDelay(1000);
        printf("status code %d", status);
        printf("\n");
    }
}

void rx_task(__unused void *params)
{
    struct can2040_msg msg;
    uint32_t rx_count = 0;
    TickType_t last_print = xTaskGetTickCount();
    printf("Main task begin\n");

    while(1)
    {
    // Count instead of printing every frame so we can keep up with the babbler
    if(xQueueReceive(queue, &msg, 100) == pdTRUE)
        {
        rx_count++;
        }
    if(xTaskGetTickCount() - last_print >= 1000)
        {
        last_print = xTaskGetTickCount();
        printf("rx/s=%lu tx done total=%lu\n",
            (unsigned long)rx_count, (unsigned long)tx_done_count);
        rx_count = 0;
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
    xTaskCreate(rx_task, "RxThread",
                configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
    return 0;
}
