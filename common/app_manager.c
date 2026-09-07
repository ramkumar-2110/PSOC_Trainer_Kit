#include "app_manager.h"

#include "cycfg_peripherals.h"

#include "led_app.h"
#include "button_app.h"
#include "relay_app.h"
#include "touch_app.h"
#include "seven_segment_app.h"
#include "pir_app.h"
#include "potentiometer_app.h"
#include "joystick_app.h"
#include "servo_app.h"
#include "potentiometer_app.h"
#include "io_expander_app.h"
#include "proximity_app.h"
#include "imu_app.h"
#include "temperature_app.h"
#include "oled_app.h"
#include "secure_app.h"
#include "eeprom_app.h"
#include "sd_card_app.h"

static app_id_t current_app = APP_NONE;

void app_manager_init(void)
{
    current_app = APP_NONE;
}

void app_manager_stop(void)
{
    if (current_app == APP_LED)
    {
        led_app_stop();
    }
    else if (current_app == APP_BUTTON)
    {
        button_app_stop();
    }
    else if (current_app == APP_RELAY)
    {
    	relay_app_stop();
    }
    else if (current_app == APP_TOUCH)
    {
    	touch_app_stop();
    }
    else if (current_app == APP_PIR)
    {
        pir_app_stop();
    }
    else if (current_app == APP_7_SEGMENT)
    {
        seven_segment_app_stop();
    }
    else if (current_app == APP_JOYSTICK)
    {
        joystick_app_stop();
    }
    else if (current_app == APP_POTENTIOMETER)
    {
        potentiometer_app_stop();
    }
    else if (current_app == APP_SERVO)
    {
        servo_app_stop();
    }
    else if (current_app == APP_IO_EXPANDER)
    {
        io_expander_app_stop();
    }
    else if (current_app == APP_PROXIMITY)
    {
        proximity_app_stop();
    }
    else if (current_app == APP_IMU)
    {
        imu_app_stop();
    }
    else if (current_app == APP_TEMPERATURE)
    {
        temperature_app_stop();
    }
    else if (current_app == APP_OLED)
    {
        oled_app_stop();
    }
    else if (current_app == APP_SECURE)
    {
        secure_app_stop();
    }
    else if (current_app == APP_EEPROM)
    {
        eeprom_app_stop();
    }
    else if (current_app == APP_SD_CARD)
    {
        sd_card_app_stop();
    }


    current_app = APP_NONE;
}

void app_manager_start(app_id_t app)
{
    /*
     * Stop the currently running application first.
     */
    app_manager_stop();

    if (app == APP_LED)
    {
        current_app = APP_LED;
        led_app_init();
    }
    else if (app == APP_BUTTON)
    {
        current_app = APP_BUTTON;
        button_app_init();
    }
    else if (app == APP_RELAY)
    {
    	current_app = APP_RELAY;
    	relay_app_init();
    }
    else if (app == APP_TOUCH)
    {
    	current_app = APP_TOUCH;
    	touch_app_init();
    }
    else if (app == APP_PIR)
    {
        current_app = APP_PIR;
        pir_app_init();
    }
    else if (app == APP_7_SEGMENT)
    {
        current_app = APP_7_SEGMENT;
        seven_segment_app_init();
    }
    else if (app == APP_JOYSTICK)
    {
        current_app = APP_JOYSTICK;
        joystick_app_init();
    }
    else if (app == APP_POTENTIOMETER)
    {
        current_app = APP_POTENTIOMETER;
        potentiometer_app_init();
    }
    else if (app == APP_SERVO)
    {
        current_app = APP_SERVO;
        servo_app_init();
    }
    else if (app == APP_IO_EXPANDER)
    {
        current_app = APP_IO_EXPANDER;
        io_expander_app_init();
    }
    else if (app == APP_PROXIMITY)
    {
        current_app = APP_PROXIMITY;
        proximity_app_init();
    }
    else if (app == APP_IMU)
    {
        current_app = APP_IMU;
        imu_app_init();
    }
    else if (app == APP_TEMPERATURE)
    {
        current_app = APP_TEMPERATURE;
        temperature_app_init();
    }
    else if (app == APP_OLED)
    {
        current_app = APP_OLED;
        oled_app_init();
    }
    else if (app == APP_SECURE)
    {
        current_app = APP_SECURE;
        secure_app_init();
    }
    else if (app == APP_EEPROM)
    {
        current_app = APP_EEPROM;
        eeprom_app_init();
    }
    else if (app == APP_SD_CARD)
    {
        current_app = APP_SD_CARD;
        sd_card_app_init();
    }
}

void app_manager_run(void)
{
    if (current_app == APP_LED)
    {
        led_app_run();
    }
    else if (current_app == APP_BUTTON)
    {
        button_app_run();
    }
    else if (current_app == APP_RELAY)
    {
        relay_app_run();
    }
    else if (current_app == APP_TOUCH)
    {
        touch_app_run();
    }
    else if (current_app == APP_PIR)
    {
        pir_app_run();
    }
    else if (current_app == APP_7_SEGMENT)
    {
        seven_segment_app_run();
    }
    else if (current_app == APP_JOYSTICK)
    {
        joystick_app_run();
    }
    else if (current_app == APP_POTENTIOMETER)
    {
        potentiometer_app_run();
    }
    else if (current_app == APP_SERVO)
    {
        servo_app_run();
    }
    else if (current_app == APP_IO_EXPANDER)
    {
        io_expander_app_run();
    }
    else if (current_app == APP_PROXIMITY)
    {
        proximity_app_run();
    }
    else if (current_app == APP_IMU)
    {
        imu_app_run();
    }
    else if (current_app == APP_TEMPERATURE)
    {
        temperature_app_run();
    }
    else if (current_app == APP_OLED)
    {
        oled_app_run();
    }
    else if (current_app == APP_SECURE)
    {
        secure_app_run();
    }
    else if (current_app == APP_EEPROM)
    {
        eeprom_app_run();
    }
    else if (current_app == APP_SD_CARD)
    {
        sd_card_app_run();
    }
}
