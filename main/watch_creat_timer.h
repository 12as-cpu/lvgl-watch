#ifndef WATCH_TIMER_H
#define WATCH_TIMER_H

#include "esp_err.h"

esp_err_t watch_timer_init(int interval_ms, char *timer_name, void (*Callback)(void *arg));
esp_err_t watch_timer_stop();

#endif /* WATCH_TIMER_H */