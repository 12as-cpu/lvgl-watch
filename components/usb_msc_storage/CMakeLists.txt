idf_component_register(
    SRCS "usb_msc_storage.c"
    INCLUDE_DIRS "include"
    REQUIRES fatfs wear_levelling sdmmc driver esp_partition
    PRIV_REQUIRES espressif__esp_tinyusb espressif__tinyusb
)
idf_component_get_property(tusb_lib espressif__tinyusb COMPONENT_LIB)
target_include_directories(${tusb_lib} PRIVATE ${COMPONENT_DIR})