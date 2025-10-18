# ZMK Feature: XY Clipper

This is a ZMK input processor module to clip relative XY movement deltas from sensors like trackballs or optical sensors.

## Overview

The `xy_clipper` input processor is designed to stabilize pointer movement, making it ideal for tasks like scrolling with a trackball.

It works by accumulating X and Y movements until one of them crosses a `threshold`. Once a threshold is crossed, it determines a "dominant" axis. By default, this processor **prioritizes the Y-axis** to improve vertical scrolling accuracy. The Y-axis movement will be considered dominant unless the accumulated X-axis movement is more than twice as large.

When one axis is determined to be dominant, the movement of the other axis is zeroed out, preventing unintended diagonal movement.

This behavior is useful for:
- Stabilizing vertical or horizontal scrolling.
- Drawing straight lines with a pointer.

## Properties

The behavior of the `xy_clipper` is controlled by properties in the device tree.

- `threshold` (int, optional but recommended):
  - **Activation Threshold:** The amount of accumulated movement required on an axis to trigger an output event.
  - **Sensitivity Scaling:** The output value is divided by this threshold, so a larger threshold will result in less sensitive (slower) pointer movement.
- `invert-x` (boolean, optional): Set to `true` to invert the direction of X-axis movement.
- `invert-y` (boolean, optional): Set to `true` to invert the direction of Y-axis movement.

## Usage

### 1. Add to West Manifest

To include this module in your firmware, you must add it to your `config/west.yml` manifest file. If you have forked this repository, replace the `remote` and `revision` with your own.

```yaml
manifest:
  remotes:
    - name: iwk7273
      url-base: https://github.com/iwk7273
  projects:
    - name: zmk-feature-xy_clipper
      remote: iwk7273
      revision: main
```

After updating the file, you need to sync the new module.

- If you are building your firmware using **GitHub Actions**, this is handled automatically when you push your changes.
- If you are building **locally**, run `west update` from your ZMK firmware directory to download the module:
  ```sh
  west update
  ```

### 2. Enable in `prj.conf`

Add the following line to your `prj.conf` file:
```conf
CONFIG_XY_CLIPPER=y
```

### 3. Configure in Device Tree Overlay

In your shield's `.overlay` file, define the `xy_clipper` processor and assign it to an input listener.

```dts
/ {
    // Example: Assign the clipper to a trackball listener
    trackball_listener {
        compatible = "zmk,input-listener";
        device = <&trackball>; // Your input device
        input-processors = <&xy_clipper>;
    };
};

/ {
    input_processors {
        xy_clipper: xy_clipper {
            compatible = "zmk,input-processor-xy-clipper";
            #input-processor-cells = <0>;

            // The movement needed to trigger an event.
            // Also acts as a sensitivity divider. Higher value = less sensitive.
            threshold = <20>;

            // Set to true (1) to invert an axis, false (0) to keep normal.
            invert-x = <0>;
            invert-y = <0>;
        };
    };
};
```
