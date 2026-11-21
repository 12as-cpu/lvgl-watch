#include <stdio.h>
#include <time.h>
#include "esp_log.h"
#include "esp_err.h"
#include "wifi_connect.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "watch_config.h"

int y, m, d, h, mi, s;
static const char *TAG = "WATCH_TIME";
bool watch_time_inited = false;
static bool sntp_inited = false; // SNTP初始化标记
static struct tm timeinfo;

const char *getWeekdayByYearday(int iY, int iM, int iD)
{
    int iWeekDay = -1;
    if (1 == iM || 2 == iM)
    {
        iM += 12;
        iY--;
    }
    iWeekDay = (iD + 1 + 2 * iM + 3 * (iM + 1) / 5 + iY + iY / 4 - iY / 100 + iY / 400) % 7;
    switch (iWeekDay)
    {
    case 0:
        return "Sun";
        break;
    case 1:
        return "Mon";
        break;
    case 2:
        return "Tues";
        break;
    case 3:
        return "Wed";
        break;
    case 4:
        return "Thurs";
        break;
    case 5:
        return "Fri";
        break;
    case 6:
        return "Sat";
        break;
    default:
        return NULL;
        break;
    }

    return NULL;
}

const char *getMonUseEnglist(int iM)
{
    switch (iM)
    {
    case 1:
        return "Jan";
        break;
    case 2:
        return "Feb";
        break;
    case 3:
        return "Mar";
        break;
    case 4:
        return "Apr";
        break;
    case 5:
        return "May";
        break;
    case 6:
        return "Jun";
        break;
    case 7:
        return "Jul";
        break;
    case 8:
        return "Aug";
        break;
    case 9:
        return "Sep";
        break;
    case 10:
        return "Oct";
        break;
    case 11:
        return "Nov";
        break;
    case 12:
        return "Dec";
        break;
    default:
        return NULL;
        break;
    }

    return NULL;
}

esp_err_t watch_time_init()
{
    if (watch_time_inited)
    {
        return ESP_OK;
    }
    setenv("TZ", "CST-8", 1);
    tzset();

    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    watch_time_inited = true;
    return ESP_OK;
}

void get_watch_time(int *year, int *mon, int *mday, int *hour, int *min, int *sec)
{
    if (year == NULL || mon == NULL || mday == NULL || hour == NULL || min == NULL || sec == NULL)
    {
        ESP_LOGE(TAG, "null pointer error");
        return;
    }
    ESP_ERROR_CHECK(watch_time_init());
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    *year = timeinfo.tm_year + 1900;
    *mon = timeinfo.tm_mon + 1;
    *mday = timeinfo.tm_mday;
    *hour = timeinfo.tm_hour;
    *min = timeinfo.tm_min;
    *sec = timeinfo.tm_sec;
}

esp_err_t watch_time_wifi_sntp_init(char *wifi_ssid, char *wifi_password)
{
    // 防重复初始化
    if (sntp_inited)
    {
        return ESP_OK;
    }
    ESP_ERROR_CHECK(watch_time_init());
    ESP_ERROR_CHECK(wifi_connect_init(wifi_ssid, wifi_password));
    vTaskDelay(pdMS_TO_TICKS(4000));

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("ntp.aliyun.com");
    config.start = false;
    esp_netif_sntp_init(&config);

    sntp_inited = true;
    ESP_LOGI(TAG, "SNTP config done, ntp server: ntp.aliyun.com");
    return ESP_OK;
}

esp_err_t watch_time_wifi_sntp()
{
    if (!sntp_inited)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_netif_sntp_start();
    const int max_retry = 20;
    int retry = 0;
    esp_err_t ret = ESP_ERR_TIMEOUT;

    while (retry < max_retry)
    {
        ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(500));
        if (ret == ESP_OK)
        {
            break;
        }
        retry++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SNTP sync failed after retries");
        return ret;
    }

    time_t now;
    char strftime_buf[64];
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "SNTP synced Beijing time: %s", strftime_buf);
    return ESP_OK;
}

esp_err_t watch_fix_time(lv_obj_t *ui_watch_fix_time_obj, lv_obj_t *ui_watch_fix_day_obj, lv_obj_t *ui_watch_fix_calendar_obj, char *wifi_ssid, char *wifi_password)
{
    // ESP_ERROR_CHECK(watch_time_init());
    // ESP_ERROR_CHECK(wifi_connect_init(wifi_ssid,wifi_password));
    // // vTaskDelay(pdMS_TO_TICKS(1100));
    // ESP_ERROR_CHECK(watch_time_wifi_sntp_init(wifi_ssid,wifi_password));
    // esp_err_t sntp_ret = watch_time_wifi_sntp();
    // if (sntp_ret == ESP_OK)
    // {
    get_watch_time(&y, &m, &d, &h, &mi, &s);
    char time_buf_h_mi[32];
    snprintf(time_buf_h_mi, sizeof(time_buf_h_mi), "%02d:%02d", h, mi);
    lv_label_set_text(ui_watch_fix_time_obj, time_buf_h_mi);
    char time_buf_m_day[32];
    snprintf(time_buf_m_day, sizeof(time_buf_m_day), "%s %02d %s", getMonUseEnglist(m), d, getWeekdayByYearday(y, m, d));
    lv_label_set_text(ui_watch_fix_day_obj, time_buf_m_day);
    lv_calendar_set_today_date(ui_watch_fix_calendar_obj, y, m, d);
    lv_calendar_set_showed_date(ui_watch_fix_calendar_obj, y, m);
    // }
    return ESP_OK;
}

int getclocksvaule(void)
{
    return (s / 60 * 3600);
}

int getclockhvaule(void)
{
    return ((h * 300) + (mi * 5));
}

int getclockmvaule(void)
{
    return (mi * 60);
}