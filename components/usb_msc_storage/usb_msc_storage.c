#include "usb_msc_storage.h"
#include <stdlib.h>
#include "esp_log.h"
#include "esp_partition.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_msc.h"
#include "sdmmc_cmd.h"
#ifdef CONFIG_USB_MSC_STORAGE_SDMMC
#include "diskio_sdmmc.h"
#if CONFIG_USB_MSC_SD_PWR_CTRL_LDO_INTERNAL_IO
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif
#endif
#ifdef CONFIG_USB_MSC_STORAGE_SPIFLASH
#include "wear_levelling.h"
#endif

static const char *TAG = "usb_msc_lib";

static tinyusb_msc_storage_handle_t s_storage_hdl = NULL;
#ifdef CONFIG_USB_MSC_STORAGE_SPIFLASH
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
#else
static sdmmc_card_t *s_sd_card = NULL;
#endif
static bool s_is_inited = false;
static usb_msc_event_cb_t s_user_cb = NULL;
static void *s_user_cb_arg = NULL;

#define EPNUM_MSC       1
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)
enum { ITF_NUM_MSC = 0, ITF_NUM_TOTAL };
enum { EDPT_CTRL_OUT = 0x00, EDPT_CTRL_IN  = 0x80, EDPT_MSC_OUT  = 0x01, EDPT_MSC_IN   = 0x81 };

static tusb_desc_device_t s_desc_device = {
    .bLength = sizeof(s_desc_device),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = CONFIG_USB_MSC_VID,
    .idProduct = CONFIG_USB_MSC_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

static uint8_t const s_fs_config_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, 64),
};
#if TUD_OPT_HIGH_SPEED
static const tusb_desc_device_qualifier_t s_dev_qualifier = {
    .bLength = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved = 0
};
static uint8_t const s_hs_config_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, 512),
};
#endif

static char const *s_string_desc[] = {
    (const char[]) { 0x09, 0x04 },
    CONFIG_USB_MSC_STR_MANUF,
    CONFIG_USB_MSC_STR_PROD,
    CONFIG_USB_MSC_STR_SERIAL,
    "USB MSC Storage",
};

#ifdef CONFIG_USB_MSC_STORAGE_SPIFLASH
static esp_err_t storage_init_flash(wl_handle_t *out_wl)
{
    const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
    if (!part) {
        ESP_LOGE(TAG, "FAT partition not found");
        return ESP_ERR_NOT_FOUND;
    }
    return wl_mount(part, out_wl);
}
#else
static esp_err_t storage_init_sdmmc(sdmmc_card_t **out_card)
{
    esp_err_t ret = ESP_OK;
    bool host_inited = false;
    sdmmc_card_t *card = NULL;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;

#if CONFIG_USB_MSC_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_ldo_config_t ldo_cfg = {.ldo_chan_id = CONFIG_USB_MSC_SD_LDO_CH};
    sd_pwr_ctrl_handle_t pwr_hdl = NULL;
    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_cfg, &pwr_hdl);
    if (ret != ESP_OK) goto clean;
    host.pwr_ctrl_handle = pwr_hdl;
#endif

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 4;
#ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
    slot.clk = CONFIG_USB_MSC_SD_CLK;
    slot.cmd = CONFIG_USB_MSC_SD_CMD;
    slot.d0  = CONFIG_USB_MSC_SD_D0;
    slot.d1  = CONFIG_USB_MSC_SD_D1;
    slot.d2  = CONFIG_USB_MSC_SD_D2;
    slot.d3  = CONFIG_USB_MSC_SD_D3;
#endif
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    card = malloc(sizeof(sdmmc_card_t));
    if (!card) { ret = ESP_ERR_NO_MEM; goto clean; }

    ret = host.init();
    if (ret != ESP_OK) goto clean;
    host_inited = true;

    ret = sdmmc_host_init_slot(host.slot, &slot);
    if (ret != ESP_OK) goto clean;

    while (sdmmc_card_init(&host, card)) {
        ESP_LOGW(TAG, "SD card missing, retry...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
    sdmmc_card_print_info(stdout, card);
    *out_card = card;
    return ESP_OK;

clean:
    if (host_inited) host.deinit();
    if (card) free(card);
#if CONFIG_USB_MSC_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_del_on_chip_ldo(host.pwr_ctrl_handle);
#endif
    return ret;
}
#endif

static void internal_msc_cb(tinyusb_msc_storage_handle_t handle, tinyusb_msc_event_t *event, void *arg)
{
    (void)handle; (void)arg;
    if (s_user_cb) {
        s_user_cb(event, s_user_cb_arg);
    }
}

esp_err_t usb_msc_storage_init(void)
{
    if (s_is_inited) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    tinyusb_msc_storage_config_t storage_cfg = {
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_APP,
        .fat_fs = {
            .base_path = NULL,
            .config.max_files = CONFIG_USB_MSC_FAT_MAX_FILE,
            .format_flags = 0,
        },
    };

#ifdef CONFIG_USB_MSC_STORAGE_SPIFLASH
    ESP_ERROR_CHECK(storage_init_flash(&s_wl_handle));
    storage_cfg.medium.wl_handle = s_wl_handle;
    ESP_ERROR_CHECK(tinyusb_msc_new_storage_spiflash(&storage_cfg, &s_storage_hdl));
#else
    ESP_ERROR_CHECK(storage_init_sdmmc(&s_sd_card));
    storage_cfg.medium.card = s_sd_card;
    ESP_ERROR_CHECK(tinyusb_msc_new_storage_sdmmc(&storage_cfg, &s_storage_hdl));
#endif

    // 注册内部回调转发用户事件
    ESP_ERROR_CHECK(tinyusb_msc_set_storage_callback(internal_msc_cb, NULL));

    // 初始化TinyUSB驱动
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.device = &s_desc_device;
    tusb_cfg.descriptor.full_speed_config = s_fs_config_desc;
    tusb_cfg.descriptor.string = s_string_desc;
    tusb_cfg.descriptor.string_count = sizeof(s_string_desc)/sizeof(s_string_desc[0]);
#if TUD_OPT_HIGH_SPEED
    tusb_cfg.descriptor.high_speed_config = s_hs_config_desc;
    tusb_cfg.descriptor.qualifier = &s_dev_qualifier;
#endif
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // 默认挂载到APP（单片机可读写）
    tinyusb_msc_set_storage_mount_point(s_storage_hdl, USB_MSC_MODE_APP);
    s_is_inited = true;
    ESP_LOGI(TAG, "USB MSC lib init success");
    return ESP_OK;
}

esp_err_t usb_msc_storage_set_mount(usb_msc_mount_mode_t mode)
{
    if (!s_is_inited || !s_storage_hdl) return ESP_ERR_INVALID_STATE;
    return tinyusb_msc_set_storage_mount_point(s_storage_hdl, mode);
}

esp_err_t usb_msc_storage_get_mount(usb_msc_mount_mode_t *out_mode)
{
    if (!s_is_inited || !s_storage_hdl) return ESP_ERR_INVALID_STATE;
    return tinyusb_msc_get_storage_mount_point(s_storage_hdl, out_mode);
}

esp_err_t usb_msc_storage_get_cap(uint32_t *out_sec_size, uint32_t *out_sec_cnt)
{
    if (!s_is_inited || !s_storage_hdl) return ESP_ERR_INVALID_STATE;
    ESP_ERROR_CHECK(tinyusb_msc_get_storage_sector_size(s_storage_hdl, out_sec_size));
    ESP_ERROR_CHECK(tinyusb_msc_get_storage_capacity(s_storage_hdl, out_sec_cnt));
    return ESP_OK;
}

esp_err_t usb_msc_register_event_cb(usb_msc_event_cb_t cb, void *user_arg)
{
    s_user_cb = cb;
    s_user_cb_arg = user_arg;
    return ESP_OK;
}

esp_err_t usb_msc_storage_deinit(void)
{
    if (!s_is_inited) return ESP_OK;
    ESP_ERROR_CHECK(tinyusb_msc_delete_storage(s_storage_hdl));
    ESP_ERROR_CHECK(tinyusb_driver_uninstall());
    s_storage_hdl = NULL;
#ifdef CONFIG_USB_MSC_STORAGE_SPIFLASH
    wl_unmount(s_wl_handle);
    s_wl_handle = WL_INVALID_HANDLE;
#else
    free(s_sd_card);
    s_sd_card = NULL;
#endif
    s_is_inited = false;
    ESP_LOGI(TAG, "USB MSC deinit success");
    return ESP_OK;
}