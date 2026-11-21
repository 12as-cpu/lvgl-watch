file(GLOB UI_SRCS "watch_time.c" "lv_port.c" "ui/*.c")
# file(GLOB UI_SRCS "lv_port.c")

idf_component_register(
    SRCS 
        "mqtt_chat.c"
        "sdcard_file.c" 
        "main.c" 
        "battery_level_check.c" 
        "wifi_connect.c"
        ${UI_SRCS}
        watch_time.c
        watch_config.c
        watch_creat_timer.c
        # watch_sleep.c
    INCLUDE_DIRS 
        "."
)

idf_component_get_property(tusb_lib espressif__tinyusb COMPONENT_LIB)
target_include_directories(${tusb_lib} PRIVATE ${COMPONENT_DIR})