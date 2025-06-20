# ZMK Feature: XY Clipper

This is a ZMK input processor module to clip relative XY movement deltas from sensors like trackballs or optical sensors.

## Overview

The `xy_clipper` input processor compares relative X and Y movement values and clips (zeroes) the smaller one.  
This helps enforce input strictly in the horizontal or vertical direction, making it especially useful for trackball-based scrolling.

For example:
- Stabilize vertical or horizontal scrolls
- Prevent diagonal input when only one axis of movement is intended

## Usage

### Enable in your board config (`prj.conf`)

```conf
CONFIG_XY_CLIPPER=y
```

### Device tree overlay

In your shield `.overlay` file, register the `xy_clipper` input processor:

```dts
/ {
    trackball_listener {
        compatible = "zmk,input-listener";
        device = <&trackball>;
        input-processors = <&xy_clipper>;
    };
};

/ {
    input_processors {
        xy_clipper: xy_clipper {
            compatible = "zmk,input-processor-xy-clipper";
            #input-processor-cells = <0>;
        };
    };
};
```
