#ifndef APP_MANAGER_H
#define APP_MANAGER_H

typedef enum
{
    APP_NONE = 0,
    APP_LED,
    APP_BUTTON,
    APP_RELAY,
    APP_TOUCH,
    APP_MOTOR,
    APP_PIR,
    APP_7_SEGMENT,
    APP_JOYSTICK,
    APP_SERVO,
    APP_POTENTIOMETER,
    APP_IO_EXPANDER,
    APP_PROXIMITY,
    APP_IMU,
    APP_TEMPERATURE,
    APP_OLED,
    APP_SECURE,
    APP_EEPROM,
    APP_SD_CARD,
    APP_WIFI
} app_id_t;

void app_manager_init(void);
void app_manager_start(app_id_t app);
void app_manager_stop(void);
void app_manager_run(void);

#endif
