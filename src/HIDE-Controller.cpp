#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/interp.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "bsp/board.h"
#include "tusb.h"

#include "usb_descriptors.h"

int LeftSetterPinPos = 20;
int RightSetterPinPos = 21;
int LeftSetterPinGnd = 18;
int RightSetterPinGnd = 19;

int SwPin = 22;

int VrxPin = 27;
int VrxAdcInput = 1;

int VryPin = 26;
int VryAdcInput = 0;

int TriggerPin = 28;
int TriggerAdcPin = 2;

enum InputSide {Left, Right};

int AButtonPin = 13;
int BButtonPin = 10;
int XButtonPin = 11;
int YButtonPin = 12;

int LeftBumperButtonPin = 2;    //TL
int RightBumperButtonPin = 3;   //TR

int StartButtonPin = 8;
int SelectButtonPin = 9;

int DPadButtonPinUp = 4;
int DPadButtonPinDown = 7;
int DPadButtonPinLeft = 5;
int DPadButtonPinRight = 6;

//static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

class SideResults{
    public:
        uint16_t Vrx;
        uint16_t Vry;
        uint16_t Trigger;
        bool Button;
};

const char* getSideName(enum InputSide Side) 
{
   switch (Side) 
   {
      case Left: return "Left";
      case Right: return "Right";
      /* etc... */
   }
}
const char* getButtonState(bool ButtonResult) 
{
   switch (ButtonResult) 
   {
      case false: return "Pressed";
      case true: return "Released";
      /* etc... */
   }
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}

SideResults ReadSide(InputSide Side);
void hid_task(void);
void switch_setup(int SwitchPin);

int main(void)
{
    stdio_init_all();

    puts("DEBUG: starting up HID device...\n");

    board_init();
    tusb_init();

    switch_setup(AButtonPin);
    switch_setup(BButtonPin);
    switch_setup(XButtonPin);
    switch_setup(YButtonPin);
    
    switch_setup(LeftBumperButtonPin);
    switch_setup(RightBumperButtonPin);

    switch_setup(StartButtonPin);
    switch_setup(SelectButtonPin);

    switch_setup(DPadButtonPinUp);
    switch_setup(DPadButtonPinDown);
    switch_setup(DPadButtonPinLeft);
    switch_setup(DPadButtonPinRight);

    // Interpolator example code
    //interp_config cfg = interp_default_config();
    // Now use the various interpolator library functions for your use case
    // e.g. interp_config_clamp(&cfg, true);
    //      interp_config_shift(&cfg, 2);
    // Then set the config 
    //interp_set_config(interp0, 0, &cfg);

    // Timer example code - This example fires off the callback after 2000ms
    //add_alarm_in_ms(2000, alarm_callback, NULL, false);

    gpio_init(SwPin);
    gpio_set_dir(SwPin, GPIO_IN);
    gpio_pull_up(SwPin);

    gpio_init(LeftSetterPinPos);
    gpio_init(RightSetterPinPos);
    gpio_init(LeftSetterPinGnd);
    gpio_init(RightSetterPinGnd);

    gpio_set_dir(LeftSetterPinPos,GPIO_OUT);
    gpio_set_dir(RightSetterPinPos,GPIO_OUT);

    adc_init();
    adc_gpio_init(TriggerPin);
    adc_gpio_init(VrxPin);
    adc_gpio_init(VryPin);

    printf("DEBUG: HID Device initialized\n");

    // Enter Loop
    InputSide SideToRead = Left;

    int8_t LastLeftStickX = 0;
    int8_t LastLeftStickY = 0;
    int8_t LastRightStickX = 0;
    int8_t LastRightStickY = 0;
    int8_t LastLeftTrigger = 0;
    int8_t LastRightTrigger = 0;

    while (true)
    {
        //SideToRead = SideToRead == Left ? Right : Left;
        //SideResults Results = ReadSide(SideToRead);
        //printf("%s:\n  X-Axis: %u\n  Y-Axis: %u\n  Trigger: %u\n  Button Pressed: %s\n",getSideName(SideToRead),Results.Vrx,Results.Vry,Results.Trigger,getButtonState(Results.Button));
        
        tud_task();

        hid_task();

        sleep_ms(250);
    }

    return 0;
}

SideResults ReadSide(InputSide Side)
{
    // Set Side
    if (Side){
        gpio_set_dir(LeftSetterPinPos,GPIO_OUT);
        gpio_put(LeftSetterPinPos,1);
        gpio_set_dir(LeftSetterPinGnd,GPIO_OUT);
        gpio_put(LeftSetterPinGnd,0);

        gpio_set_dir(RightSetterPinPos,GPIO_IN);
        gpio_set_dir(RightSetterPinGnd,GPIO_IN);
    } else {
        gpio_set_dir(RightSetterPinPos,GPIO_OUT);
        gpio_put(RightSetterPinPos,1);
        gpio_set_dir(RightSetterPinGnd,GPIO_OUT);
        gpio_put(RightSetterPinGnd,0);

        gpio_set_dir(LeftSetterPinPos,GPIO_IN);
        gpio_set_dir(LeftSetterPinGnd,GPIO_IN);
    }

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
    CurrentSide.Button = gpio_get(SwPin);

    return CurrentSide;
}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
    // Poll every 10ms
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if ( board_millis() - start_ms < interval_ms) return; // not enough time
    start_ms += interval_ms;

    uint32_t const btn = board_button_read();

    // Remote wakeup
    if ( tud_suspended() && btn )
    {
        // Wake up host if we are in suspend mode
        // and REMOTE_WAKEUP feature is enabled by host
        tud_remote_wakeup();
    }else
    {
        // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
        if ( !tud_hid_ready() ) return;

        hid_gamepad_report_t report =
        {
            .x = 0, .y = 0, .z = 0, .rz = 0, .rx = 0, .ry = 0, .hat = 0, .buttons = 0
        };

        int DPadX = gpio_get(DPadButtonPinRight)? 1:0 - gpio_get(DPadButtonPinLeft)? 1:0; 
        int DPadY = gpio_get(DPadButtonPinUp)? 1:0 - gpio_get(DPadButtonPinDown)? 1:0;

        switch DPadX {
            case 1:
                switch DPadY{
                    case 1:
                        report.hat = GAMEPAD_HAT_UP_RIGHT;
                        break;
                    case 0:
                        report.hat = GAMEPAD_HAT_RIGHT
                        break;
                    case -1:
                        report.hat = GAMEPAD_HAT_DOWN_RIGHT;
                        break;
                }
                break;
            case 0;
                switch DPadY{
                    case 1:
                        report.hat = GAMEPAD_HAT_UP;
                        break;
                    case -1:
                        report.hat = GAMEPAD_HAT_DOWN
                        break;
                }
                break;
            case -1;
                switch DPadY{
                    case 1:
                        report.hat = GAMEPAD_HAT_UP_LEFT;
                        break;
                    case 0:
                        report.hat = GAMEPAD_HAT_RIGHT
                        break;
                    case -1:
                        report.hat = GAMEPAD_HAT_DOWN_LEFT;
                        break;
                }
                break;
        }
        
        if (gpio_get(AButtonPin)){
            report.buttons += GAMEPAD_BUTTON_A;
        }
        if (gpio_get(BButtonPin)){
            report.buttons += GAMEPAD_BUTTON_B;
        }
        if (gpio_get(XButtonPin)){
            report.buttons += GAMEPAD_BUTTON_X;
        }
        if (gpio_get(YButtonPin)){
            report.buttons += GAMEPAD_BUTTON_Y;
        }

        if (gpio_get(LeftBumperButtonPin)){
            report.buttons += GAMEPAD_BUTTON_TL;
        }
        if (gpio_get(RightBumperButtonPin)){
            report.buttons += GAMEPAD_BUTTON_TR;
        }

        if (gpio_get(StartButtonPin)){
            report.buttons += GAMEPAD_BUTTON_START;
        }
        if (gpio_get(SelectButtonPin)){
            report.buttons += GAMEPAD_BUTTON_SELECT;
        }

        SideResults LeftResults = ReadSide(Left);
        SideResults RightResults = ReadSide(Right);

        if (LeftResults.Button) {
            report.buttons += GAMEPAD_BUTTON_THUMBL;
        }
        if (RightResults.Button) {
            report.buttons += GAMEPAD_BUTTON_THUMBR;
        }
        
        report.x = (LeftResults.Vrx-LastLeftStickX)/4;
        LastLeftStickX = LeftResults.Vrx;

        report.y = (LeftResults.Vry-LastLeftStickY)/4;
        LastLeftStickY = LeftResults.Vry;

        report.rx = (LeftResults.Trigger/2-LastLeftTrigger);
        LastLeftTrigger= LeftResults.Trigger/2;

        report.z = (RightResults.Vrx-LastRightStickX)/4;
        LastRightStickX = RightResults.Vrx;

        report.rz = (RightResults.Vry-LastRightStickY)/4;
        LastRightStickY = RightResults.Vry;

        report.ry = (RightResults.Trigger/2-LastRightTrigger);
        LastRightTrigger= RightResults.Trigger/2;


        tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
    }
}

void switch_setup(int SwitchPin)
{
    gpio_init(SwitchPin);
    gpio_set_dir(SwitchPin,GPIO_IN);
    gpio_pull_down(SwitchPin);
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
    //blink_interval_ms = BLINK_MOUNTED;
    printf("DEBUG: Mounted\n");
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    //blink_interval_ms = BLINK_NOT_MOUNTED;
    printf("DEBUG: Unmounted\n");
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void) remote_wakeup_en;
    printf("DEBUG: Suspended\n");
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    printf("DEBUG: Resumed Mounted\n"); 
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t itf, uint8_t const* report, uint8_t len)
{
    (void) itf;
    (void) len;

    printf("DEBUG: REPORT Sent Successfully to Host\n");
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    // TODO not Implemented
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    (void) itf;
        
    printf("DEBUG: tud_hid_set_report_cb triggered\n");
    printf("DEBUG: report_id: %X\n", report_id);
    printf("DEBUG: report_type: %X\n", report_type);
    printf("DEBUG: bufsize: %d\n", bufsize);

    printf("DEBUG: buffer content:\n");
    for (int i = 0; i < bufsize; i++) {
        printf("%02X ", buffer[i]);
    }
    printf(" - End \n");
    
}