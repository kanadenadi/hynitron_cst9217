/*
 * Hynitron CST9217 Touchscreen Driver
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/pinctrl/consumer.h>


#define HYN_REG_BASE            0xD000
#define HYN_REG_WORKMODE        0xD100
#define HYN_CMD_NORMAL          0xD109
#define HYN_STAT_DOWN           0x06

#define MAX_POINTS              5
#define MAX_I2C_RETRY           3

#define READ_BUF_LEN            (7 + (MAX_POINTS - 1) * 5)

struct cst9217_ts_data {
    struct i2c_client *client;
    struct input_dev *input_dev;
    int irq_gpio;
    int reset_gpio;
    int irq;
    u32 abs_x_max;
    u32 abs_y_max;
    struct regulator *vdd;
    struct mutex mutex;
};

#define HYN_INFO(fmt, ...) pr_info("[HYN_TS] " fmt, ##__VA_ARGS__)
#define HYN_ERR(fmt, ...) pr_err("[HYN_TS] Error: " fmt, ##__VA_ARGS__)
// #define DEBUG_RAW_DATA  // 如果需要查看原始16进制数据，取消注释此行


static int hyn_i2c_read(struct i2c_client *client, u16 reg, u8 *data, int len)
{
    struct i2c_msg msgs[2];
    u8 reg_buf[2];
    int ret;

    reg_buf[0] = (reg >> 8) & 0xFF;
    reg_buf[1] = reg & 0xFF;

    msgs[0].addr = client->addr;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = reg_buf;

    msgs[1].addr = client->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = data;

    ret = i2c_transfer(client->adapter, msgs, 2);
    return (ret == 2) ? 0 : -EIO;
}


static int hyn_i2c_write(struct i2c_client *client, u16 reg, u8 *data, int len)
{
    u8 *buf;
    int ret;
    struct i2c_msg msg;

    buf = kzalloc(len + 2, GFP_KERNEL);
    if (!buf) return -ENOMEM;

    buf[0] = (reg >> 8) & 0xFF;
    buf[1] = reg & 0xFF;
    if (len > 0 && data)
        memcpy(&buf[2], data, len);

    msg.addr = client->addr;
    msg.flags = 0;
    msg.len = len + 2;
    msg.buf = buf;

    ret = i2c_transfer(client->adapter, &msg, 1);
    kfree(buf);
    return (ret == 1) ? 0 : -EIO;
}


static int hyn_i2c_write_cmd(struct i2c_client *client, u16 reg)
{
    return hyn_i2c_write(client, reg, NULL, 0);
}


static void cst9217_report_touch(struct cst9217_ts_data *ts)
{
    u8 buf[READ_BUF_LEN] = {0};
    int ret, i;
    u8 point_num = 0;
    u8 finger_state;
    unsigned long active_ids = 0;
    

    u8 sync_data = 0xAB; 

    mutex_lock(&ts->mutex);


    ret = hyn_i2c_read(ts->client, HYN_REG_BASE, buf, READ_BUF_LEN);
    if (ret < 0) {
        HYN_ERR("I2C read failed\n");
        goto out_unlock;
    }

#ifdef DEBUG_RAW_DATA
    print_hex_dump(KERN_INFO, "[HYN_RAW] ", DUMP_PREFIX_NONE, 16, 1, buf, 16, false);
#endif


    if (buf[6] != 0xAB) {

        hyn_i2c_write(ts->client, 0xD000, &sync_data, 1); 
        goto out_unlock;
    }


    point_num = buf[5] & 0x7F;
    if (point_num > MAX_POINTS) point_num = MAX_POINTS;


    if (point_num > 0) {
        for (i = 0; i < point_num; i++) {

            int idx;
            int id;
            unsigned int x, y;


            idx = (i == 0) ? 0 : (7 + (i - 1) * 5);
            

            if (idx + 4 >= READ_BUF_LEN) break;


            id = (buf[idx] >> 4) & 0x0F;
            finger_state = buf[idx] & 0x0F;


            x = ((unsigned int)buf[idx+1] << 4) | ((buf[idx+3] >> 4) & 0x0F);
            y = ((unsigned int)buf[idx+2] << 4) | (buf[idx+3] & 0x0F);

            if (id >= MAX_POINTS) continue;


            if (finger_state == HYN_STAT_DOWN) {
                input_mt_slot(ts->input_dev, id);
                input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
                input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
                input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
                

                __set_bit(id, &active_ids);
            }
        }
    }

    
    for (i = 0; i < MAX_POINTS; i++) {
        if (!test_bit(i, &active_ids)) {
            input_mt_slot(ts->input_dev, i);
            input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
        }
    }


    input_mt_sync_frame(ts->input_dev);
    if (active_ids) {
        input_report_key(ts->input_dev, BTN_TOUCH, 1);
    } else {
        input_report_key(ts->input_dev, BTN_TOUCH, 0);
    }
    input_sync(ts->input_dev);

    
    hyn_i2c_write(ts->client, 0xD000, &sync_data, 1);

out_unlock:
    mutex_unlock(&ts->mutex);
}

static irqreturn_t cst9217_irq_handler(int irq, void *dev_id)
{
    struct cst9217_ts_data *ts = dev_id;
    cst9217_report_touch(ts);
    return IRQ_HANDLED;
}

static void cst9217_hw_reset(struct cst9217_ts_data *ts)
{
    if (gpio_is_valid(ts->reset_gpio)) {
        gpio_direction_output(ts->reset_gpio, 0);
        msleep(10);
        gpio_set_value(ts->reset_gpio, 1);
        msleep(50); 
    }
}

static int cst9217_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct device_node *np = client->dev.of_node;
    struct cst9217_ts_data *ts;
    u32 coords[2];
    u32 gpios[3];
    int ret;
    
    // SPRD
    struct pinctrl *p = devm_pinctrl_get(&client->dev); 
    if (!IS_ERR(p)) {
        struct pinctrl_state *s = pinctrl_lookup_state(p, "default");
        if (!IS_ERR(s)) pinctrl_select_state(p, s);
    }

    HYN_INFO("Probe Start (Stable)\n");

    ts = devm_kzalloc(&client->dev, sizeof(*ts), GFP_KERNEL);
    if (!ts) return -ENOMEM;
    
    ts->client = client;
    i2c_set_clientdata(client, ts);
    mutex_init(&ts->mutex);

    // 电源
    ts->vdd = regulator_get(NULL, "vddsdcore"); 
    if (!IS_ERR(ts->vdd)) {
        ret = regulator_set_voltage(ts->vdd, 2800000, 3000000);
        if (ret) HYN_ERR("Failed to set regulator voltage\n");
        
        ret = regulator_enable(ts->vdd);
        if (ret) HYN_ERR("Failed to enable regulator\n");
        
        msleep(20); 
    }

    // GPIO 解析
    if (of_property_read_u32_array(np, "hynitron,reset-gpio", gpios, 3) == 0)
        ts->reset_gpio = gpios[1];
    else
        ts->reset_gpio = -1;

    if (of_property_read_u32_array(np, "hynitron,irq-gpio", gpios, 3) == 0)
        ts->irq_gpio = gpios[1];
    else
        ts->irq_gpio = -1;

    // 复位
    if (gpio_is_valid(ts->reset_gpio)) {
        if (gpio_request(ts->reset_gpio, "hyn_rst") == 0) {
            cst9217_hw_reset(ts);
        }
    }

    // I2C 连通性测试
    {
        u8 dummy[4];
        if (hyn_i2c_read(client, 0xD000, dummy, 4) < 0) {
            HYN_ERR("I2C communication failed, trying reset...\n");
            cst9217_hw_reset(ts);
            msleep(100);
        }
    }

    // 发送初始化指令 0xD109
    ret = hyn_i2c_write_cmd(client, HYN_CMD_NORMAL);
    if (ret < 0) {
        HYN_ERR("Failed to send init cmd 0xD109\n");
    } else {
        HYN_INFO("Init cmd 0xD109 sent\n");
    }
    msleep(20);

    // 分辨率
    if (of_property_read_u32_array(np, "hynitron,display-coords", coords, 2) == 0) {
        ts->abs_x_max = coords[0];
        ts->abs_y_max = coords[1];
    } else {
        ts->abs_x_max = 720; 
        ts->abs_y_max = 1280;
    }

    // 注册 Input
    ts->input_dev = devm_input_allocate_device(&client->dev);
    if (!ts->input_dev) return -ENOMEM;
    
    ts->input_dev->name = "Hynitron cst9217 Touchscreen";
    ts->input_dev->id.bustype = BUS_I2C;
    
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->abs_x_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->abs_y_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_X, 0, ts->abs_x_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_Y, 0, ts->abs_y_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, 65535, 0, 0); 
    input_set_abs_params(ts->input_dev, ABS_MT_SLOT, 0, MAX_POINTS - 1, 0, 0);

    input_mt_init_slots(ts->input_dev, MAX_POINTS, INPUT_MT_DIRECT);
    input_set_capability(ts->input_dev, EV_KEY, BTN_TOUCH);

    ret = input_register_device(ts->input_dev);
    if (ret) return ret;

    // 申请中断
    if (gpio_is_valid(ts->irq_gpio)) {
        if (gpio_request(ts->irq_gpio, "hyn_irq") == 0) {
            gpio_direction_input(ts->irq_gpio);
            ts->irq = gpio_to_irq(ts->irq_gpio);
        }
    } else {
        ts->irq = client->irq;
    }
    
    // 使用上升沿
    ret = devm_request_threaded_irq(&client->dev, ts->irq, NULL,
                                    cst9217_irq_handler,
                                    IRQF_ONESHOT | IRQF_TRIGGER_RISING, 
                                    "hyn_ts", ts);
    if (ret) {
        HYN_ERR("Failed to request IRQ %d\n", ts->irq);
        return ret;
    }
    
    HYN_INFO("Probe success! IRQ: %d (RISING)\n", ts->irq);
    return 0;
}

static int cst9217_remove(struct i2c_client *client)
{
    struct cst9217_ts_data *ts = i2c_get_clientdata(client);
    if (gpio_is_valid(ts->reset_gpio)) gpio_free(ts->reset_gpio);
    if (gpio_is_valid(ts->irq_gpio)) gpio_free(ts->irq_gpio);
    return 0;
}

static const struct of_device_id cst9217_match[] = { { .compatible = "hynitron,hyn_ts" }, { } };
static const struct i2c_device_id cst9217_id[] = { { "hyn_ts", 0 }, { } };
MODULE_DEVICE_TABLE(of, cst9217_match);

static struct i2c_driver cst9217_driver = {
    .driver = { 
        .name = "hynitron_ts", 
        .of_match_table = cst9217_match,
    },
    .probe = cst9217_probe,
    .remove = cst9217_remove,
    .id_table = cst9217_id,
};

module_i2c_driver(cst9217_driver);
MODULE_LICENSE("GPL");