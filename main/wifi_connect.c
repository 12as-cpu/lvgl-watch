#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "watch_config.h"

static const char *TAG = "WIFI";
#define MAX_RECONNECT_CNT 5
static int s_disconnect_cnt = 0;
static int WIFI_INITED = 0;
static int WIFI_INITING = 0;
static int evt_loop_created = 0;

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WIFI STA start, try connect");
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "Connected to AP");
            s_disconnect_cnt = 0;
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected from AP");
            s_disconnect_cnt++;

            if (s_disconnect_cnt < MAX_RECONNECT_CNT)
            {
                ESP_LOGI(TAG, "Reconnect %d/%d", s_disconnect_cnt, MAX_RECONNECT_CNT);
                esp_wifi_connect();
            }
            else
            {
                ESP_LOGE(TAG, "Reconnect max times reached, stop retry! Check SSID/PASSWORD or AP");
                WIFI_INITED = 0;
                s_disconnect_cnt = 0;
            }
            break;

        default:
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Get IP: " IPSTR, IP2STR(&event->ip_info.ip));
            WIFI_INITING = 0;
            WIFI_INITED = 1;
            s_disconnect_cnt = 0;
            break;
        default:
            break;
        }
    }
}

//ssid,password为0则使用默认名称，密码
esp_err_t wifi_connect_init(char *wifi_ssid, char *wifi_password)
{
    while (WIFI_INITING == 1)
    {
        vTaskDelay(10);
    }
    if (WIFI_INITED == 1)
    {
        ESP_LOGW(TAG, "WiFi already initialized");
        return ESP_OK;
    }
    WIFI_INITING = 1;
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs init failed");
        return ret;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    if (!evt_loop_created)
    {
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));
        evt_loop_created = 1;
    }
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta.threshold.authmode = WIFI_AUTH_WPA2_PSK,
        .sta.pmf_cfg.capable = true,
        .sta.pmf_cfg.required = false,
    };
    if (wifi_ssid == NULL || wifi_password == NULL)
    {
        strncpy((char *)wifi_config.sta.ssid, watch_default_wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
        wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';

        strncpy((char *)wifi_config.sta.password, watch_default_wifi_password, sizeof(wifi_config.sta.password) - 1);
        wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';
    }
    else
    {
        strncpy((char *)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
        wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';

        strncpy((char *)wifi_config.sta.password, wifi_password, sizeof(wifi_config.sta.password) - 1);
        wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi init finish, ssid:%s", wifi_config.sta.ssid);
    return ESP_OK;
}

esp_err_t wifi_connect_stop()
{
    if (WIFI_INITED == 0)
    {
        ESP_LOGW(TAG, "WiFi already stop");
        return ESP_OK;
    }
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    WIFI_INITED = 0;
    return ESP_OK;
}