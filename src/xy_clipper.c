#define DT_DRV_COMPAT zmk_input_processor_xy_clipper

#include "drivers/input_processor.h"
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <stdlib.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* Note: 'threshold' must be defined in the device tree for each instance of xy_clipper */
struct xy_clipper_data {
    int32_t x;
    int32_t y;
    bool has_x;
    bool has_y;
};

struct xy_clipper_config {
    int32_t threshold;
    bool invert_x;
    bool invert_y;
};

static int xy_clipper_handle_event(
    const struct device *dev, struct input_event *event, uint32_t param1,
    uint32_t param2, struct zmk_input_processor_state *state) {
    struct xy_clipper_data *data = dev->data;
    const struct xy_clipper_config *config = dev->config;
    int32_t threshold = config->threshold;
    bool invert_x = config->invert_x;
    bool invert_y = config->invert_y;

    switch (event->type) {
    case INPUT_EV_REL:
        if (event->code == INPUT_REL_X) {
            if (event->value == 0) {
                return ZMK_INPUT_PROC_STOP;
            }
            data->x += event->value;
            data->has_x = true;
        } else if (event->code == INPUT_REL_Y) {
            if (event->value == 0) {
                return ZMK_INPUT_PROC_STOP;
            }
            data->y += event->value;
            data->has_y = true;
        } else {
            return ZMK_INPUT_PROC_CONTINUE;
        }

        int32_t out_x = 0, out_y = 0;

        if (abs(data->x) >= threshold && data->x != 0) {
            out_x = invert_x ? -((data->x > 0) ? 1 : -1) : ((data->x > 0) ? 1 : -1);
            data->x = 0;
            data->y = 0;
        } else if (abs(data->y) >= threshold && data->y != 0) {
            out_y = invert_y ? -((data->y > 0) ? 1 : -1) : ((data->y > 0) ? 1 : -1);
            data->x = 0;
            data->y = 0;
        } else {
            return ZMK_INPUT_PROC_STOP;
        }

        if (event->code == INPUT_REL_X) {
            event->value = out_x;
        } else if (event->code == INPUT_REL_Y) {
            event->value = out_y;
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
  static const struct xy_clipper_config xy_clipper_config_##n = { \
      .threshold = DT_INST_PROP(n, threshold), \
      .invert_x = DT_INST_PROP(n, invert_x), \
      .invert_y = DT_INST_PROP(n, invert_y), \
  }; \
  DEVICE_DT_INST_DEFINE(n, NULL, NULL, \
                        &xy_clipper_data_##n, &xy_clipper_config_##n, \
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                        &xy_clipper_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XY_CLIPPER_INST)