#ifndef WATCH_TIME_H
#define WATCH_TIME_H

#include "esp_err.h"
#include "lvgl.h"

esp_err_t watch_time_init();
void get_watch_time(int *year, int *mon, int *mday, int *hour, int *min, int *sec);
esp_err_t watch_time_wifi_sntp_init(char *wifi_ssid, char *wifi_password);
esp_err_t watch_time_wifi_sntp();
esp_err_t watch_fix_time(lv_obj_t *ui_watch_fix_time_obj,lv_obj_t *ui_watch_fix_day_obj,lv_obj_t *ui_watch_fix_calendar_obj, char *wifi_ssid, char *wifi_password);
int getclocksvaule(void);
int getclockhvaule(void);
int getclockmvaule(void);

#endif