#ifndef MQTT_CHAT_H
#define MQTT_CHAT_H

#include "esp_err.h"
#include "lvgl.h"

/**
 * @brief WiFi + MQTT 聊天模块初始化
 * @param wifi_ssid WiFi名称
 * @param wifi_password WiFi密码
 * @param mqtt_broker_uri MQTT服务器域名/地址
 * @param mqtt_broker_port MQTT端口号
 * @param mqtt_client_id 自定义固定ClientID，设置为NULL时启用esp32.mac作为生成部分
 * @param server_name MQTT用户名
 * @param server_password MQTT密码
 * @return ESP_OK 初始化成功，其他为错误码
 */
esp_err_t mqtt_chat_init(const char *wifi_ssid,
                         const char *wifi_password,
                         const char *mqtt_broker_uri,
                         int mqtt_broker_port,
                         const char *mqtt_client_id,
                         const char *server_name,
                         const char *server_password);

/**
 * @brief 发送消息到指定订阅
 * @param message 发送的消息内容
 * @param topic 发送到的订阅
 */
void mqtt_chat_send_message(const char *message, const char *topic);

esp_err_t mqtt_chat_stop();

esp_err_t mqtt_chat_reconnect();

esp_err_t mqtt_chat_set_receive_message(lv_obj_t * ui_jieshou_obj);

#endif