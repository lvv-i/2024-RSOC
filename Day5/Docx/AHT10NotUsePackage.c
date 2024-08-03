// 不使用软件包的情况下
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define AHT_I2C_BUS_NAME "i2c1"  // AHT20 挂载的I2C总线
#define AHT_ADDR 0x38            // AHT20 I2C地址
#define AHT_CALIBRATION_CMD 0xBE // AHT20 初始化命令
#define AHT_NORMAL_CMD 0xA8      // AHT20 正常工作模式命令
#define AHT_GET_DATA_CMD 0xAC    // AHT20 获取结果命令

// I2C_BUS设备指针，用于等会寻找与记录AHT挂载的I2C总线
struct rt_i2c_bus_device *i2c_bus = RT_NULL;

// AHT命令的空参数
rt_uint8_t Parm_Null[2] = {0, 0};

// 写命令（主机向从机传输数据）
rt_err_t write_reg(struct rt_i2c_bus_device *Device, rt_uint8_t reg, rt_uint8_t *data)
{
    // 代写入的数据
    // 数组大小为3的原因：buf[0]--命令（即上面的AHT_CALIBRATION_CMD、AHT_NORMAL_CMD、AHT_GET_DATA_CMD
    //                   buf[1]/buf[2]为命令后跟的参数，AHT有些命令后面需要加上参数，具体可查看数据手册
    rt_uint8_t buf[3];

    // 记录数组大小
    rt_uint8_t buf_size;

    // I2C传输的数据结构体
    struct rt_i2c_msg msgs;

    buf[0] = reg;
    if (data != RT_NULL)
    {
        buf[1] = data[0];
        buf[2] = data[1];
        buf_size = 3;
    }
    else
    {
        buf_size = 1;
    }

    msgs.addr = AHT_ADDR;   // 消息要发送的地址：即AHT地址
    msgs.flags = RT_I2C_WR; // 消息的标志位：读还是写，是否需要忽视ACK回应，是否需要发送停止位，是否需要发送开始位(用于拼接数据使用)...
    msgs.buf = buf;         // 消息的缓冲区：待发送/接收的数组
    msgs.len = buf_size;    // 消息的缓冲区大小：待发送/接收的数组的大小

    // 传输信息
    // 这里i2c.core层提供给我们三个API去进行I2C的数据传递：
    /*
     * 1.发送API
     * rt_size_t rt_i2c_master_send(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             const rt_uint8_t         *buf,
                             rt_uint32_t               count)

       2.接收API
       rt_size_t rt_i2c_master_recv(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             rt_uint8_t               *buf,
                             rt_uint32_t               count)
       3.传输API
       rt_size_t rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg         msgs[],
                          rt_uint32_t               num)
      * 实际上1跟2最后都会调用回3，大家可以按照自己需求进行调用
    */
    if (rt_i2c_transfer(Device, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return RT_ERROR;
    }
}

// 读数据（从机向主机返回数据）
rt_err_t read_reg(struct rt_i2c_bus_device *Device, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr = AHT_ADDR;   // 消息要发送的地址：即AHT地址
    msgs.flags = RT_I2C_RD; // 消息的标志位：读还是写，是否需要忽视ACK回应，是否需要发送停止位，是否需要发送开始位(用于拼接数据使用)...
    msgs.buf = buf;         // 消息的缓冲区：待发送/接收的数组
    msgs.len = len;         // 消息的缓冲区大小：待发送/接收的数组的大小

    // 传输函数，上面有介绍
    if (rt_i2c_transfer(Device, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return RT_ERROR;
    }
}

// 读取AHT的温湿度数据
void read_temp_humi(float *Temp_Data, float *Humi_Data)
{
    // 根据数据手册我们可以看到要读取一次数据需要使用到的数组大小为6
    rt_uint8_t Data[6];

    write_reg(i2c_bus, AHT_GET_DATA_CMD, Parm_Null); // 发送一个读取命令，让AHT进行一次数据采集
    rt_thread_mdelay(500);                           // 等待采集
    read_reg(i2c_bus, 6, Data);                      // 读取数据

    // 根据数据手册进行数据处理
    *Humi_Data = (Data[1] << 12 | Data[2] << 4 | (Data[3] & 0xf0) >> 4) * 100.0 / (1 << 20);
    *Temp_Data = ((Data[3] & 0x0f) << 16 | Data[4] << 8 | Data[5]) * 200.0 / (1 << 20) - 50;
}

// AHT进行初始化
void AHT_Init(const char *name)
{
    // 寻找AHT的总线设备
    i2c_bus = rt_i2c_bus_device_find(name);

    if (i2c_bus == RT_NULL)
    {
        rt_kprintf("Can't Find I2C_BUS Device"); // 找不到总线设备
    }
    else
    {
        write_reg(i2c_bus, AHT_NORMAL_CMD, Parm_Null); // 设置为正常工作模式
        rt_thread_mdelay(400);

        rt_uint8_t Temp[2]; // AHT_CALIBRATION_CMD需要的参数
        Temp[0] = 0x08;
        Temp[1] = 0x00;
        write_reg(i2c_bus, AHT_CALIBRATION_CMD, Temp);
        rt_thread_mdelay(400);
    }
}

// AHT设备测试线程
void AHT_test(void)
{
    float humidity, temperature;

    AHT_Init(AHT_I2C_BUS_NAME); // 进行设备初始化

    while (1)
    {
        read_temp_humi(&temperature, &humidity); // 读取数据
        rt_kprintf("humidity   : %d.%d %%\n", (int)humidity, (int)(humidity * 10) % 10);
        rt_kprintf("temperature: %d.%d\n", (int)temperature, (int)(temperature * 10) % 10);
        rt_thread_mdelay(1000);
    }
}
MSH_CMD_EXPORT(AHT_test, AHT_test); // 将命令到出到MSH列表
