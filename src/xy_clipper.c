#define DT_DRV_COMPAT zmk_input_processor_xy_clipper

#include "drivers/input_processor.h"
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <stdlib.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct xy_clipper_config {};

struct xy_clipper_data {
    int32_t x;
    int32_t y;
    bool has_x;
    bool has_y;
};

static int xy_clipper_handle_event(
    const struct device *dev, struct input_event *event, uint32_t param1,
    uint32_t param2, struct zmk_input_processor_state *state) {
    struct xy_clipper_data *data = dev->data;

    switch (event->type) {
    case INPUT_EV_REL:
        if (event->code == INPUT_REL_X) {
            LOG_DBG("Before clip: code=X, value=%d", event->value);
            data->x = event->value;
            data->has_x = true;
        } else if (event->code == INPUT_REL_Y) {
            LOG_DBG("Before clip: code=Y, value=%d", event->value);
            data->y = event->value;
            data->has_y = true;
        } else {
            return ZMK_INPUT_PROC_CONTINUE;
        }

        if (data->has_x && data->has_y) {
            if (abs(data->x) < abs(data->y)) {
                data->x = 0;
            } else {
                data->y = 0;
            }
            data->has_x = false;
            data->has_y = false;
        }

        if (event->code == INPUT_REL_X) {
            event->value = data->x;
            LOG_DBG("After clip: code=X, value=%d", event->value);
        } else if (event->code == INPUT_REL_Y) {
            event->value = data->y;
            LOG_DBG("After clip: code=Y, value=%d", event->value);
        }

        return ZMK_INPUT_PROC_CONTINUE;

    default:
        return ZMK_INPUT_PROC_CONTINUE;
    }
}

static struct zmk_input_processor_driver_api xy_clipper_driver_api = {
    .handle_event = xy_clipper_handle_event,
};

#define XY_CLIPPER_INST(n) \
  static struct xy_clipper_data xy_clipper_data_##n = { \
      .x = 0, \
      .y = 0, \
      .has_x = false, \
      .has_y = false, \
  }; \
  static const struct xy_clipper_config xy_clipper_config_##n = {}; \
  DEVICE_DT_INST_DEFINE(n, NULL, NULL, \
                        &xy_clipper_data_##n, &xy_clipper_config_##n, \
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                        &xy_clipper_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XY_CLIPPER_INST)