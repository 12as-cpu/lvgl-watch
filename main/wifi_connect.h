#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"

/**
 * @brief WiFi STA初始化并连接路由器
 * @param wifi_ssid WiFi名称字符串
 * @param wifi_password WiFi密码字符串
 * @return ESP_OK 成功；其他失败码
 */
esp_err_t wifi_connect_init(char *wifi_ssid, char *wifi_password);

esp_err_t wifi_connect_stop();

#endif /* WIFI_H */