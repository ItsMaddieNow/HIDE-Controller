#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"


int SwPin = 22;

int VrxPin = 27;
int VrxAdcInput = 1;

int VryPin = 26;
int VryAdcInput = 0;

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}


int main()
{
    stdio_init_all();

    gpio_init(SwPin);
    gpio_set_dir(SwPin, GPIO_IN);
    gpio_pull_up(SwPin);

    adc_init();
    adc_gpio_init(VrxPin);
    adc_gpio_init(VryPin);

    // Timer example code - This example fires off the callback after 2000ms
    while (true)
    {
        bool SwResult = gpio_get(SwPin);
        printf("Button Is Pressed: %s \n", !SwResult ? "true" : "false");

        adc_select_input(VrxAdcInput);
        uint16_t VrxResult = adc_read();
        printf("X axis Output: %u \n", VrxResult);

        adc_select_input(VryAdcInput);
        uint16_t VryResult = adc_read();
        printf("Y axis Output: %u \n", VryResult);

        sleep_ms(500);
    }

    return 0;
}
