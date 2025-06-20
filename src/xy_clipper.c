#define DT_DRV_COMPAT zmk_input_processor_xy_clipper

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zmk/input/input_processor.h>
#include <stdlib.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct xy_clipper_config {};

struct xy_clipper_data {
    int32_t x;
    int32_t y;
    bool has_x;
    bool has_y;
};

static int xy_clipper_process(
    const struct device *dev, struct input_event *event, uint32_t param1,
    uint32_t param2, struct zmk_input_processor_state *state) {
    struct xy_clipper_data *data = dev->data;

    switch (event->type) {
    case INPUT_EV_REL:
        if (event->code == INPUT_REL_X) {
            data->x = event->value;
            data->has_x = true;
        } else if (event->code == INPUT_REL_Y) {
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
        } else if (event->code == INPUT_REL_Y) {
            event->value = data->y;
        }

        return ZMK_INPUT_PROC_CONTINUE;

    default:
        return ZMK_INPUT_PROC_CONTINUE;
    }
}

static const struct xy_clipper_config xy_clipper_cfg = {};
static struct xy_clipper_data xy_clipper_data_inst = {
    .x = 0,
    .y = 0,
    .has_x = false,
    .has_y = false,
};

ZMK_INPUT_PROCESSOR_DEFINE(xy_clipper, xy_clipper_process);