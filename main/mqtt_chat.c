#include <stdio.h>
#include "mqtt_client.h"
#include "esp_err.h"
#include "esp_log.h"
#include "wifi_connect.h"
#include "lvgl.h"
#include "ui/ui.h"

static bool s_is_mqtt_connected = false;

static const char *TAG = "mqtt";

static lv_obj_t * lvgl_jieshou_obj;

static esp_mqtt_client_handle_t s_mqtt_client = NULL;

esp_err_t mqtt_chat_set_receive_message(lv_obj_t * ui_jieshou_obj)
{
    lvgl_jieshou_obj = ui_jieshou_obj;
    return ESP_OK;
}

void mqtt_chat_receive_message_event(char *message)
{
    lv_label_set_text(lvgl_jieshou_obj, "");
    lv_label_set_text(lvgl_jieshou_obj, message);
}


static void aliot_mqtt_event_handler(void *event_handler_arg,
                                     esp_event_base_t event_base,
                                     int32_t event_id,
                                     void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    //esp_mqtt_client_handle_t client = event->client;

    // your_context_t *context = event->context;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED: // 连接成功
        ESP_LOGI(TAG, "mqtt connected");
        s_is_mqtt_connected = true;
        // 连接成功后，订阅测试主题
        esp_mqtt_client_subscribe_single(s_mqtt_client, "chat/channel0", 1);
        break;
    case MQTT_EVENT_DISCONNECTED: // 连接断开
        ESP_LOGI(TAG, "mqtt disconnected");
        s_is_mqtt_connected = false;
        break;
    case MQTT_EVENT_SUBSCRIBED: // 收到订阅消息ACK
        ESP_LOGI(TAG, " mqtt subscribed ack, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED: // 收到解订阅消息ACK
        break;
    case MQTT_EVENT_PUBLISHED: // 收到发布消息ACK
        ESP_LOGI(TAG, "mqtt publish ack, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        mqtt_chat_receive_message_event(event->data);
        //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic); // 收到Pub消息直接打印出来
        //printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        break;
    }
}

/**
 * @brief WiFi + MQTT 聊天模块初始化
 * @param wifi_ssid WiFi名称
 * @param wifi_password WiFi密码
 * @param mqtt_broker_uri MQTT服务器域名/地址
 * @param mqtt_broker_port MQTT端口号
 * @param use_random_client_id 是否启用随机ClientID
 * @param mqtt_client_id 自定义固定ClientID，use_random_client_id=false时生效
 * @param sever_name MQTT用户名（建议修正为server_name拼写）
 * @param sever_password MQTT密码
 * @return ESP_OK 初始化成功，其他为错误码
 */
esp_err_t mqtt_chat_init(char *wifi_ssid,
                         char *wifi_password,
                         const char *mqtt_broker_uri,
                         int mqtt_broker_port,
                         const char *mqtt_client_id,
                         const char *server_name,
                         const char *server_password)
{
    if (s_is_mqtt_connected)
    {
        ESP_LOGI(TAG, "mqtt alread init");
        return ESP_OK;
    }
    esp_err_t wifi_set = wifi_connect_init(wifi_ssid, wifi_password);
    if (wifi_set != ESP_OK)
    {
        ESP_LOGI(TAG, "wifi set error please check wifi ssid and password");
        return 0;
    }
    esp_mqtt_client_config_t mqtt_cfg = {0};
    mqtt_cfg.broker.address.uri = mqtt_broker_uri;
    mqtt_cfg.broker.address.port = mqtt_broker_port;
    if (mqtt_client_id == NULL)
    {
        mqtt_cfg.credentials.set_null_client_id = true;
    }
    else
    {
        mqtt_cfg.credentials.client_id = mqtt_client_id;
    }
    mqtt_cfg.credentials.username = server_name;
    mqtt_cfg.credentials.authentication.password = server_password;
    ESP_LOGI(TAG, "mqtt init done try to connect");
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, aliot_mqtt_event_handler, s_mqtt_client);
    vTaskDelay(500);
    esp_mqtt_client_start(s_mqtt_client);
    return ESP_OK;
}

/**
 * @brief 发送消息到指定订阅
 * @param message 发送的消息内容
 * @param topic 发送到的订阅
 */
void mqtt_chat_send_message(const char *message, const char *topic)
{
    static char mqtt_pub_buff[64];
    if (s_is_mqtt_connected)
    {
        snprintf(mqtt_pub_buff, 64, message);
        esp_mqtt_client_publish(s_mqtt_client, topic,
                                mqtt_pub_buff, strlen(mqtt_pub_buff), 1, 0);
    }
    else
    {
        ESP_LOGI(TAG, "please check mqtt connect");
    }
}

esp_err_t mqtt_chat_subscribe(const char *topic, int qos)
{
    if (s_is_mqtt_connected)
    {
        esp_mqtt_client_subscribe_single(s_mqtt_client, topic, qos);
        return ESP_OK;
    }
    else
    {
        ESP_LOGI(TAG, "please check mqtt connect");
        return 0;
    }
}

esp_err_t mqtt_chat_stop()
{
    if (!s_is_mqtt_connected)
    {
        ESP_LOGI(TAG, "mqtt already exit");
        s_is_mqtt_connected = false;
        return ESP_OK;
    }
    ESP_ERROR_CHECK(esp_mqtt_client_disconnect(s_mqtt_client));
    ESP_ERROR_CHECK(esp_mqtt_client_stop(s_mqtt_client));
    return ESP_OK;
}

esp_err_t mqtt_chat_reconnect()
{
    ESP_ERROR_CHECK(esp_mqtt_client_reconnect(s_mqtt_client));
    return ESP_OK;
}
