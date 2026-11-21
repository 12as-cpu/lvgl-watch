#pragma once
#include "esp_err.h"
#include "tinyusb_msc.h"

#ifdef __cplusplus
extern "C" {
#endif

// 挂载模式枚举，对外复用TinyUSB原生定义
typedef tinyusb_msc_mount_point_t usb_msc_mount_mode_t;
#define USB_MSC_MODE_APP    TINYUSB_MSC_STORAGE_MOUNT_APP
#define USB_MSC_MODE_USB    TINYUSB_MSC_STORAGE_MOUNT_USB

/**
 * @brief 初始化USB MSC大容量存储
 * @return ESP_OK 成功
 */
esp_err_t usb_msc_storage_init(void);

/**
 * @brief 切换存储挂载模式
 * @param mode USB_MSC_MODE_APP / USB_MSC_MODE_USB
 */
esp_err_t usb_msc_storage_set_mount(usb_msc_mount_mode_t mode);

/**
 * @brief 获取当前挂载模式
 * @param out_mode 输出当前模式
 */
esp_err_t usb_msc_storage_get_mount(usb_msc_mount_mode_t *out_mode);

/**
 * @brief 获取存储容量信息
 * @param out_sec_size 扇区大小
 * @param out_sec_cnt 总扇区数
 */
esp_err_t usb_msc_storage_get_cap(uint32_t *out_sec_size, uint32_t *out_sec_cnt);

/**
 * @brief 销毁USB MSC，释放所有资源
 */
esp_err_t usb_msc_storage_deinit(void);

// 挂载状态回调注册
typedef void (*usb_msc_event_cb_t)(tinyusb_msc_event_t *event, void *user_arg);
esp_err_t usb_msc_register_event_cb(usb_msc_event_cb_t cb, void *user_arg);

#ifdef __cplusplus
}
#endif