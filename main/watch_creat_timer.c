#include <stdio.h>
#include "esp_timer.h"
#include "esp_err.h"
#include "wifi_connect.h"
#include "esp_sntp.h"

static esp_timer_handle_t watch_timer_handle = NULL;
bool watch_timer_created = false;
// static int watch_times = 0;

// void watch_time_cb(void *arg)
// {
//     watch_times++;
// }

esp_err_t watch_timer_init(int interval_ms, char *timer_name, void (*Callback)(void *arg))
{
    if (watch_timer_created)
    {
        return ESP_OK;
    }
    esp_timer_create_args_t watch_time_cfg = {
        .callback = Callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = timer_name,
        .skip_unhandled_events = true};
    ESP_ERROR_CHECK(esp_timer_create(&watch_time_cfg, &watch_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(watch_timer_handle, interval_ms)); //60000000us = 1min
    watch_timer_created = true;
    return ESP_OK;
}

esp_err_t watch_timer_stop()
{
    if (!watch_timer_created)
    {
        return ESP_OK;
    }
    ESP_ERROR_CHECK(esp_timer_stop(watch_timer_handle));
    ESP_ERROR_CHECK(esp_timer_delete(watch_timer_handle));
    watch_timer_created = false;
    return ESP_OK;
}

// int get_watch_times(void)
// {
//     return watch_times;
// }


