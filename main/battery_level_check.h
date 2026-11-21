#ifndef BATTERY_LEVEL_CHECK_H
#define BATTERY_LEVEL_CHECK_H

void configure_adc2(int battery_pin);
float battery_level_read(int battery_pin);

#endif