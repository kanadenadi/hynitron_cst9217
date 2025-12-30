Hynitron CST9217 Touchscreen Configuration Guide

---
<a name="english-version"></a>
1. Device Tree (DTS) Configuration
Add the following node to your I2C bus section (e.g., &i2c1):

   ```dts
   hynitron@5a {
    compatible = "hynitron,hyn_ts";
    reg = <0x5a>;
    /* GPIO: <Bank Pin Flags> */
    hynitron,reset-gpio = <0x10 0x91 0x00>;
    hynitron,irq-gpio = <0x10 0x90 0x00>;
    hynitron,max-touch-number = <0x05>;
    hynitron,display-coords = <0x170 0x1c0>; /* 368x448 */
    vdd_name = "vddsdcore";
   };

| Property       | Description                                   |
| -------------- | --------------------------------------------- |
| reg            | I2C slave address (Hex).                      |
| reset-gpio     | Reset pin index. 0x91 refers to Pin 145.     |
| irq-gpio       | Interrupt pin index. 0x90 refers to Pin 144. |
| max-touch-number | Maximum number of simultaneous touches.      |
| display-coords | Screen resolution in Hex (Width, Height).    |
| vdd_name       | Power supply name.                            |

Tips:
  Check kernel log for initialization and errors: dmesg | grep HYN_TS
  Test touch event reporting: getevent -lt
