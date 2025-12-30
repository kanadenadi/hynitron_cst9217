Hynitron CST9217 触摸屏配置指南
[English Version](#english-version) | [中文版](#中文版)

---
<a name="english-version"></a>
1. Device Tree (DTS) Configuration
Add the following node to your I2C bus section (e.g., &i2c1):

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

Debugging Methods:
- Check kernel log for initialization and errors: dmesg | grep HYN_TS
- Test touch event reporting: getevent -lt

<a name="中文版"></a>
1. 设备树 (DTS) 配置
在你的 I2C 总线节点（例如 &i2c1）下添加以下内容：

hynitron@5a {
    compatible = "hynitron,hyn_ts";
    reg = <0x5a>;
    /* GPIO参数：<控制器索引 引脚号 标志位> */
    hynitron,reset-gpio = <0x10 0x91 0x00>;
    hynitron,irq-gpio = <0x10 0x90 0x00>;
    hynitron,max-touch-number = <0x05>;
    hynitron,display-coords = <0x170 0x1c0>; /* 分辨率 368x448 */
    vdd_name = "vddsdcore";
};

| 属性名             | 说明                                       |
| ------------------ | ------------------------------------------ |
| reg                | I2C 从机地址（十六进制）。                 |
| reset-gpio         | 复位引脚索引。0x91 对应 145 号引脚。       |
| irq-gpio           | 中断引脚索引。0x90 对应 144 号引脚。       |
| max-touch-number   | 最大支持的触摸点数量。                     |
| display-coords     | 分辨率（宽度, 高度，十六进制表示）。       |
| vdd_name           | 电源名称。                                 |

调试方法：
- 查看内核日志：dmesg | grep HYN_TS
- 测试触摸事件上报：getevent -lt
