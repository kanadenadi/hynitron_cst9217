Hynitron CST9217 触摸屏配置指南English Version | 中文版<a name="english-version"></a>English Version1. Device Tree (DTS) ConfigurationAdd the following node to your I2C bus section (e.g., &i2c1):DTShynitron@5a {
    compatible = "hynitron,hyn_ts";
    reg = <0x5a>;
    /* GPIO: <Bank Pin Flags> */
    hynitron,reset-gpio = <0x10 0x91 0x00>;
    hynitron,irq-gpio = <0x10 0x90 0x00>;
    hynitron,max-touch-number = <0x05>;
    hynitron,display-coords = <0x170 0x1c0>; /* 368x448 */
    vdd_name = "vddsdcore";
};
2. Parameter DefinitionsPropertyDescriptionregI2C slave address (Hex).reset-gpioReset pin index. 0x91 refers to Pin 145.irq-gpioInterrupt pin index. 0x90 refers to Pin 144.display-coordsScreen resolution in Hex (Width, Height).<a name="中文版"></a>中文版1. 设备树 (DTS) 配置在你的 I2C 总线节点（例如 &i2c1）下添加以下内容：DTShynitron@5a {
    compatible = "hynitron,hyn_ts";
    reg = <0x5a>;
    /* GPIO参数：<控制器索引 引脚号 标志位> */
    hynitron,reset-gpio = <0x10 0x91 0x00>;
    hynitron,irq-gpio = <0x10 0x90 0x00>;
    hynitron,max-touch-number = <0x05>;
    hynitron,display-coords = <0x170 0x1c0>; /* 分辨率 368x448 */
    vdd_name = "vddsdcore";
};
2. 参数定义说明属性名说明regI2C 从机地址（十六进制）。reset-gpio复位引脚索引。0x91 对应 145 号引脚。irq-gpio中断引脚索引。0x90 对应 144 号引脚。display-coords十六进制表示的分辨率（宽度, 高度）。3. Debugging / 调试方法查看内核日志:dmesg | grep HYN_TS测试事件上报:getevent -lt
