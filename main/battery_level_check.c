#include "esp_adc/adc_oneshot.h"

adc_oneshot_unit_handle_t adc1_handle;

void configure_adc2(int battery_pin)
{
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    adc_oneshot_chan_cfg_t channel_config = {
        .atten = 3,
        .bitwidth = ADC_BITWIDTH_12};
    adc_oneshot_config_channel(adc1_handle, battery_pin, &channel_config);
}
float battery_level_read(int battery_pin)
{
    int raw_value;
    adc_oneshot_read(adc1_handle, battery_pin, &raw_value);
    float out_raw;
    out_raw = (float)raw_value;
    return out_raw * 0.0016117216117216; // 3.3 / 4095 * 2
}
