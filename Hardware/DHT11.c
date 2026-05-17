#include "stm32f10x.h" // Device header
#include "Delay.h"
#include "DHT11.h"

void DHT11_GPIO_Init(void) // 初始化DHT11模块的DATA引脚
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(DHT11_Out_RCC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = DHT11_Out_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11, &GPIO_InitStructure);

    GPIO_SetBits(DHT11, DHT11_Out_Pin); // 先拉高复位
}

static void DHT11_Mode_IPU(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = DHT11_Out_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT11, &GPIO_InitStructure);
}

static void DHT11_Mode_Out_PP(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.GPIO_Pin = DHT11_Out_Pin;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(DHT11, &GPIO_InitStructure);
}

// 等待DHT11的回应
// 返回1:未检测到DHT11的存在
// 返回0:存在
uint8_t DHT11_Check(void)
{
    u8 retry = 0;
    DHT11_Mode_IPU();
    while (GPIO_ReadInputDataBit(DHT11, DHT11_Out_Pin) && retry < 100) // DHT11会拉低40~80us
    {
        retry++;
        Delay_us(1);
    };
    if (retry >= 100)
        return 1;
    else
        retry = 0;
    while (!GPIO_ReadInputDataBit(DHT11, DHT11_Out_Pin) && retry < 100) // DHT11拉低后会再次拉高40~80us
    {
        retry++;
        Delay_us(1);
    };
    if (retry >= 100)
        return 1;
    return 0;
}

// 从DHT11读取一个位
// 返回值：1/0
uint8_t DHT11_Read_Bit(void)
{
    u8 retry = 0;
    while (GPIO_ReadInputDataBit(DHT11, DHT11_Out_Pin) && retry < 100) // 等待变为低电平
    {
        retry++;
        Delay_us(1);
    }
    retry = 0;
    while (!GPIO_ReadInputDataBit(DHT11, DHT11_Out_Pin) && retry < 100) // 等待变高电平
    {
        retry++;
        Delay_us(1);
    }
    Delay_us(40); // 等待40us
    if (GPIO_ReadInputDataBit(DHT11, DHT11_Out_Pin))
        return 1;
    else
        return 0;
}

// 从DHT11读取一个字节
// 返回值：读到的数据
uint8_t DHT11_Read_Byte(void)
{
    u8 i, dat;
    dat = 0;
    for (i = 0; i < 8; i++)
    {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    return dat;
}

uint8_t Read_Byte(void)
{
    uint8_t i, temp = 0;

    for (i = 0; i < 8; i++)
    {
        while (DHT11_DATA_IN() == Bit_RESET)
            ;

        Delay_us(40);

        if (DHT11_DATA_IN() == Bit_SET)
        {
            while (DHT11_DATA_IN() == Bit_SET)
                ;

            temp |= (uint8_t)(0x01 << (7 - i));
        }
        else
        {
            temp &= (uint8_t)~(0x01 << (7 - i));
        }
    }
    return temp;
}

uint8_t Read_DHT11(DHT11_Data_TypeDef *DHT11_Data)
{
    DHT11_Mode_Out_PP(); // 先将DATA引脚设为输出，准备启动DHT11
    DHT11_DATA_OUT(LOW); // 先拉低，发送开始信号
    Delay_ms(20);

    DHT11_DATA_OUT(HIGH); // 后拉高，延时等待

    Delay_us(13);

    DHT11_Mode_IPU(); // 再将DATA引脚设为输入，准备接收响应信号

    if (DHT11_Check() == 0) // 如果接收到低电平，则得到响应
    {
        //		while(DHT11_DATA_IN() == Bit_RESET);//等待响应结束

        //		while(DHT11_DATA_IN() == Bit_SET);//等待高电平，出现则响应成功

        //		DHT11_Data -> humi_int = Read_Byte();//开始读取

        //		DHT11_Data -> humi_deci = Read_Byte();

        //		DHT11_Data -> temp_int = Read_Byte();

        //		DHT11_Data -> temp_deci = Read_Byte();

        //		DHT11_Data -> check_sum= Read_Byte();

        DHT11_Data->humi_int = DHT11_Read_Byte(); // 开始读取

        DHT11_Data->humi_deci = DHT11_Read_Byte();

        DHT11_Data->temp_int = DHT11_Read_Byte();

        DHT11_Data->temp_deci = DHT11_Read_Byte();

        DHT11_Data->check_sum = DHT11_Read_Byte();

        DHT11_Mode_Out_PP();  // 重新设为输出，为下一次读取做准备
        DHT11_DATA_OUT(HIGH); // 设为高电平，为下一次开启DHT11做准备

        if (DHT11_Data->check_sum ==
            DHT11_Data->humi_int + DHT11_Data->humi_deci + DHT11_Data->temp_int + DHT11_Data->temp_deci) // 检验读取数据是否正确
            return SUCCESS;
        else
            return ERROR;
    }
    else
    {
        return ERROR;
    }
}
