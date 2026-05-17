#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "DHT11.h"
#include "Encoder.h"
#include "Key.h"
#include "LED.h"
#include "Beep.h"
#include "HC05.h"
#include "Motor.h"
#include "Usart.h"
#include "Timer.h"
#include "oledmenu.h"
#include "servo.h"
#include "ADC.h"

int main(void)
{
    __set_PRIMASK(0); // <--必须加：解除所有中断屏蔽

    int menu;

    // 初始化所有外设
    OLED_Init();
    LED_Init();
    Key_Init();
    DHT11_GPIO_Init();
    Encoder_Init();
    Beep_Init();
    HC05_Init();
    Motor_Init();
    Usart_Init();
    Servo_Init();
    Timer_Init();
    AD_Init();

    // 初始化舵机角度为90度（中值），风扇摆正归位
    Servo_SetAngle(90);

    // 初始化完成，执行启动声光提示
    Beep_Start();
    LED1_ON();
    Delay_ms(250);
    Beep_Stop();
    LED1_OFF();

    menu = Menu_Main(); // 初始进入主菜单
    while (1)
    {
        switch (menu)
        {
        case 1:
            menu = Menu_Manual_Main(); // 返回下一个模式或0
            break;
        case 2:
            menu = Menu_Auto_Main();
            break;
        case 3:
            menu = Menu_Timer_Main();
            break;
        case 4:
            menu = Menu_Bluetooth_Main();
            break;
        default:
            menu = Menu_Main(); // 如果返回0（按返回键）或无效值，重新进入主菜单
            break;
        }
    }
}
