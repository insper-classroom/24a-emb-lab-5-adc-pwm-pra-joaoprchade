#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

QueueHandle_t xQueueAdc;

uint const X_PIN = 26;
uint const Y_PIN = 27;

typedef struct adc {
    int axis;
    int val;
} adc_t;

void x_task(void *p) {
    adc_t adc_x;
    //adc_x.val = 0;
    adc_x.axis = 0;

    while (1) {
        adc_select_input(0);

        if ((adc_read() - 2047) / 8 > -30 && (adc_read() - 2047) / 8 < 30) {
            adc_x.val = 0;
        } else {
            adc_x.val = (adc_read() - 2047) / 8;
        }

        xQueueSend(xQueueAdc, &adc_x, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p) {
    adc_t adc_y;
    //adc_y.val = 0;
    adc_y.axis = 1;

    while (1) {
        adc_select_input(1);

        if ((adc_read() - 2047) / 8 > -30 && (adc_read() - 2047) / 8 < 30) {
            adc_y.val = 0;
        } else {
            adc_y.val = (adc_read() - 2047) / 8;
        }

        xQueueSend(xQueueAdc, &adc_y, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void write(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF;

    uart_putc_raw(uart0, data.axis);
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb);
    uart_putc_raw(uart0, -1);
}

void uart_task(void *p) {
    adc_t data;

    while (1) {
        if (xQueueReceive(xQueueAdc, &data, portMAX_DELAY)) {
            write(data);
        }
    }
}

int main() {
    stdio_init_all();
    adc_init();

    adc_gpio_init(X_PIN);
    adc_gpio_init(Y_PIN);

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    xTaskCreate(x_task, "x_task", 4096, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
