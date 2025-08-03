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
    bool triggered_since_last_sync;
};

struct xy_clipper_config {
    int32_t threshold;
    int invert_x;
    int invert_y;
};


static int xy_clipper_handle_event(
    const struct device *dev, struct input_event *event, uint32_t param1,
    uint32_t param2, struct zmk_input_processor_state *state) {
    struct xy_clipper_data *data = dev->data;
    const struct xy_clipper_config *config = dev->config;
    int32_t threshold = config->threshold;
    bool invert_x = config->invert_x != 0;
    bool invert_y = config->invert_y != 0;

    switch (event->type) {
    case INPUT_EV_REL:
        if (event->code == INPUT_REL_X) {
            data->x += event->value;
            event->value = 0;
        } else if (event->code == INPUT_REL_Y) {
            data->y += event->value;
            event->value = 0;
        } else {
            return ZMK_INPUT_PROC_CONTINUE;
        }


        // Determine which axis, if any, has crossed the threshold and is dominant.
        bool x_triggered = abs(data->x) >= threshold;
        bool y_triggered = abs(data->y) >= threshold;

        if (x_triggered && (!y_triggered || abs(data->x) >= abs(data->y))) {
            // X is dominant or the only one triggered.
            event->code = INPUT_REL_X;
            int32_t val = data->x / threshold;
            event->value = invert_x ? -val : val;
            data->x %= threshold;
            data->y = 0; // Reset the non-dominant axis accumulator.
            data->triggered_since_last_sync = true;
            return ZMK_INPUT_PROC_CONTINUE;
        } else if (y_triggered) {
            // Y is dominant or the only one triggered.
            event->code = INPUT_REL_Y;
            int32_t val = data->y / threshold;
            event->value = invert_y ? -val : val;
            data->y %= threshold;
            data->x = 0; // Reset the non-dominant axis accumulator.
            data->triggered_since_last_sync = true;
            return ZMK_INPUT_PROC_CONTINUE;
        }
        // If neither accumulator is over the threshold, invalidate the event
        // and stop processing. This prevents it from being handled by other
        // processors or included in the HID report.
        event->code = 0xFFFF; // Use an invalid code.
        return ZMK_INPUT_PROC_STOP; // event->value has already been set to 0.

    case INPUT_EV_SYN:
        if (event->code == INPUT_SYN_REPORT) {
            if (data->triggered_since_last_sync) {
                data->triggered_since_last_sync = false;
                return ZMK_INPUT_PROC_CONTINUE;
            }
            return ZMK_INPUT_PROC_STOP;
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
      .triggered_since_last_sync = false, \
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