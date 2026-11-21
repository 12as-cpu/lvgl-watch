#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
// #include "wifi_connect.h"
// #include "esp_sntp.h"
#include "esp_timer.h"
#include "watch_time.h"
#include "lvgl.h"
#include "demos/lv_demos.h"
#include "lv_port.h"
#include "ui/ui.h"
// #include "ui/ui_jiemian.h"
#include "st7789_driver.h"
#include "mqtt_chat.h"
// #include "esp_pm.h"
#include "watch_creat_timer.h"

#define TAG "main"


void watch_time_cb(void *parameter)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_ERROR_CHECK(watch_fix_time(ui_sjian, ui_riqi, ui_Calendar2, NULL, NULL));
    }
}

void _lv_task(void *parameter)
{
    while (1)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(3));
    }
}

void app_main(void)
{
    lv_port_init();
    st7789_lcd_backlight(4095);
    // ESP_ERROR_CHECK(mqtt_chat_init(NULL, NULL, "mqtt://broker-cn.emqx.io", 1883, NULL, "sever", "a13432332914"));
    // ESP_ERROR_CHECK(watch_time_init());
    // ESP_ERROR_CHECK(watch_time_wifi_sntp_init(NULL,NULL));
    // ESP_ERROR_CHECK(watch_time_wifi_sntp());
    ui_init();
    // lv_demo_music();
    // lv_demo_widgets();
    // ESP_ERROR_CHECK(mqtt_chat_set_receive_message(ui_Label13));
    // ESP_ERROR_CHECK(watch_fix_time(ui_sjian, ui_riqi, ui_Calendar2, NULL, NULL));
    // xTaskCreatePinnedToCore(watch_time_cb, "timer", 10000, NULL, 10, NULL, 1);
    xTaskCreatePinnedToCore(_lv_task, "lv_task", 10000, NULL, 15, NULL, 0);
}