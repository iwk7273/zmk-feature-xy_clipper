#define DT_DRV_COMPAT zmk_input_processor_xy_clipper

#include "drivers/input_processor.h"
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <stdlib.h>
#include <limits.h>
#ifdef CONFIG_ZMK_CUSTOM_CONFIG
#include <zmk/custom_feature.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);


/* Note: 'threshold' must be defined in the device tree for each instance of xy_clipper */
struct xy_clipper_data {
    int32_t x;
    int32_t y;
    int32_t effective_threshold;
    int32_t last_threshold;
    int8_t last_invert_x;
    int8_t last_invert_y;
};

struct xy_clipper_config {
    int32_t threshold;
    int invert_x;
    int invert_y;
};


static int32_t xy_clipper_get_threshold(const struct device *dev, struct xy_clipper_data *data,
                                        const struct xy_clipper_config *config) {
    int32_t threshold = config->threshold;
#ifdef CONFIG_ZMK_CUSTOM_CONFIG
    threshold = (int32_t)zmk_custom_config_scroll_div_value();
    data->effective_threshold = 0;
#endif

    if (data->effective_threshold > 0) {
        return data->effective_threshold;
    }

    if (threshold <= 0) {
        LOG_WRN("%s: invalid threshold %d, clamping to 1", dev->name, threshold);
        threshold = 1;
    }

    data->effective_threshold = threshold;
    return data->effective_threshold;
}


static int xy_clipper_handle_event(
    const struct device *dev, struct input_event *event, uint32_t param1,
    uint32_t param2, struct zmk_input_processor_state *state) {
    struct xy_clipper_data *data = dev->data;
    const struct xy_clipper_config *config = dev->config;
    int32_t threshold = xy_clipper_get_threshold(dev, data, config);
    bool invert_x = config->invert_x != 0;
    bool invert_y = config->invert_y != 0;
#ifdef CONFIG_ZMK_CUSTOM_CONFIG
    invert_x = zmk_custom_config_scroll_h_rev();
    invert_y = zmk_custom_config_scroll_v_rev();
#endif
    if (threshold != data->last_threshold || invert_x != data->last_invert_x ||
        invert_y != data->last_invert_y) {
        LOG_INF("xy_clipper cfg threshold=%d invert_x=%d invert_y=%d",
                threshold, invert_x, invert_y);
        data->last_threshold = threshold;
        data->last_invert_x = invert_x;
        data->last_invert_y = invert_y;
    }

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
        LOG_DBG("xy_clipper acc x=%d y=%d thr=%d x_trig=%d y_trig=%d",
                data->x, data->y, threshold, x_triggered, y_triggered);

        // Prioritize Y-axis and emphasize its value by 2x in the dominance comparison.
        if (y_triggered && (!x_triggered || (abs(data->y) * 2) >= abs(data->x))) {
            // Y is dominant or the only one triggered.
            event->code = INPUT_REL_Y;
            int32_t val = data->y / threshold;
            event->value = invert_y ? -val : val;
            data->y %= threshold;
            data->x = 0; // Reset the non-dominant axis accumulator.
            LOG_DBG("xy_clipper choose Y val=%d rem_y=%d reset_x=0", event->value, data->y);
            return ZMK_INPUT_PROC_CONTINUE;
        } else if (x_triggered) {
            // X is dominant or the only one triggered.
            event->code = INPUT_REL_X;
            int32_t val = data->x / threshold;
            event->value = invert_x ? -val : val;
            data->x %= threshold;
            data->y = 0; // Reset the non-dominant axis accumulator.
            LOG_DBG("xy_clipper choose X val=%d rem_x=%d reset_y=0", event->value, data->x);
            return ZMK_INPUT_PROC_CONTINUE;
        }
        LOG_DBG("xy_clipper stop (no trigger)");
        return ZMK_INPUT_PROC_STOP; // event->value has already been set to 0.

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
      .effective_threshold = 0, \
      .last_threshold = INT32_MIN, \
      .last_invert_x = -1, \
      .last_invert_y = -1, \
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
