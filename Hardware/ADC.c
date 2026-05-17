#include "stm32f10x.h" // 设备头文件

/**
 * 函数：初始化ADC
 * 参数：无
 * 返回：无
 */
void AD_Init(void)
{
    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);  // 使能ADC1时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // 使能GPIOA时钟

    /* 配置ADC时钟 */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 时钟6分频：ADCCLK = 72MHz / 6 = 12MHz

    /* GPIO初始化 */
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure); // PA0-PA1初始化为模拟输入

    /* 转换序列和采样时间设置 */

    /* ADC初始化 */
    ADC_InitTypeDef ADC_InitStructure = {0};                            // ADC初始化结构体
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;                  // 独立模式
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;              // 数据右对齐
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 禁用外部触发
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;                 // 单次转换模式
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;                       // 禁用扫描模式
    ADC_InitStructure.ADC_NbrOfChannel = 1;                             // 通道数为1
    ADC_Init(ADC1, &ADC_InitStructure);                                 // 初始化ADC1

    /* 使能ADC */
    ADC_Cmd(ADC1, ENABLE); // 使能ADC1

    /* ADC校准 */
    ADC_ResetCalibration(ADC1); // 复位ADC校准
    while (ADC_GetResetCalibrationStatus(ADC1) == SET)
        ;
    ADC_StartCalibration(ADC1); // 开始ADC校准
    while (ADC_GetCalibrationStatus(ADC1) == SET)
        ;
}

/**
 * 函数：读取ADC转换结果
 * 参数：ADC_Channel - 指定ADC通道（ADC_Channel_0 ~ ADC_Channel_3）
 * 返回：ADC转换结果（0~4095）
 */
uint16_t AD_GetValue(uint8_t ADC_Channel)
{
    ADC_RegularChannelConfig(ADC1, ADC_Channel, 1, ADC_SampleTime_55Cycles5); // 转换前配置通道
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);                                   // 启动ADC转换
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
        ;                                // 等待转换完成
    return ADC_GetConversionValue(ADC1); // 返回ADC转换结果
}

/**
 * 快速近似 pow(x, 0.6549) 函数
 * 使用 exp(y*ln(x)) 的查表+线性插值近似，避免调用标准库 pow()
 * 仅适用于 x > 0 的场景
 */
static float fast_ln(float x)
{
    // 利用IEEE754浮点数特性的快速对数近似
    union
    {
        float f;
        uint32_t i;
    } vx = {x};
    float y = (float)(vx.i - 1064866805) * 8.262958e-8f;
    return y;
}

static float fast_exp(float x)
{
    // 快速指数近似
    union
    {
        float f;
        uint32_t i;
    } vx;
    vx.i = (uint32_t)(12102203.0f * x + 1064866805.0f);
    return vx.f;
}

static float fast_pow(float base, float exponent)
{
    if (base <= 0.0f)
        return 0.0f;
    return fast_exp(exponent * fast_ln(base));
}

static const float MQ2_R0 = 6.64f; // MQ2传感器基准电阻

float MQ2_GetData_PPM(void)
{
    float tempData = 0;

    for (uint8_t i = 0; i < 10; i++)
    {
        tempData += AD_GetValue(ADC_Channel_1);
    }
    tempData /= 10;

    float Vol = (tempData * 5.0f / 4096.0f);
    if (Vol < 0.01f)
        Vol = 0.01f; // 防止除零
    float RS = (5.0f - Vol) / (Vol * 0.5f);

    float ppm = fast_pow(11.5428f * MQ2_R0 / RS, 0.6549f);

    return ppm;
}
