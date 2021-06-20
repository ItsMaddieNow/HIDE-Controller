#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "bsp/board.h"

int LeftSetterPin = 20;
int RightSetterPin = 21;

int SwPin = 22;

int VrxPin = 27;
int VrxAdcInput = 1;

int VryPin = 26;
int VryAdcInput = 0;

int TriggerPin = 28;
int TriggerAdcPin = 2;

enum InputSide {Left, Right};


int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}
class SideResults{
    public:
        uint16_t Vrx;
        uint16_t Vry;
        uint16_t Trigger;
        bool button;
};

SideResults ReadSide(InputSide Side)
{
    // Set Side
    gpio_put(LeftSetterPin, Side);
    gpio_put(RightSetterPin, !Side);

    // Read Joystick and Analog Trigger
    SideResults CurrentSide;
    // Joystick X Axis
    adc_select_input(VrxAdcInput);
    CurrentSide.Vrx = adc_read();
    // Joystick Y Axis
    adc_select_input(VryAdcInput);
    CurrentSide.Vry = adc_read();
    // Analog Trigger
    adc_select_input(TriggerAdcPin);
    CurrentSide.Trigger = adc_read();
    // Joystick Button
    CurrentSide.button = gpio_get(SwPin);

    return CurrentSide;
}

void HidTask (void) {
    const uint32_t interval_ms = 1;
    static uint32_t start_ms = 0;
    
    if ( board_millis() - start_ms < interval_ms) return; // not enough time
    start_ms += interval_ms;

    tud_hid_gamepad_report(REPORT_ID_GAMEPAD,0,0,0,0,0,0,0,0);
}

int main()
{
    stdio_init_all();

    gpio_init(SwPin);
    gpio_set_dir(SwPin, GPIO_IN);
    gpio_pull_up(SwPin);

    gpio_init(LeftSetterPin);
    gpio_init(RightSetterPin);
    gpio_set_dir(LeftSetterPin,GPIO_OUT);
    gpio_set_dir(RightSetterPin,GPIO_OUT);

    adc_init();
    adc_gpio_init(VrxPin);
    adc_gpio_init(VryPin);

    tusb_init();


    // Timer example code - This example fires off the callback after 2000ms
    InputSide SideToRead = Left;

    while (true)
    {
        tud_task();
        
        SideToRead = SideToRead==Left? Right : Left;
        SideResults Results = ReadSide(SideToRead);
        HidTask();

        sleep_ms(500);
    }

    return 0;
}