#include "stm32f10x.h" // Device header
#include "oledmenu.h"
#include "OLED.h"
#include "Key.h"
#include "Encoder.h"
#include "LED.h"
#include "Motor.h"
#include "servo.h"
#include "Timer.h"
#include "Delay.h"
#include "DHT11.h"
#include "ADC.h"
#include "HC05.h"
#include "Beep.h"
#include <stdio.h>

/* ========== 菜单项数定义 ========== */
#define MAIN_MENU_ITEMS 3	// 旋钮可选菜单项数（手动、自动、定时），蓝牙模式由蓝牙指令触发
#define MANUAL_MENU_ITEMS 4 // 手动模式菜单项数
#define AUTO_MENU_ITEMS 4	// 自动模式菜单项数
#define TIMER_MENU_ITEMS 8	// 定时模式菜单项数

/* ========== 风扇参数限制 ========== */
#define MAX_FAN_SPEED 99  // 最大风速
#define MIN_FAN_SPEED 0	  // 最小风速
#define SPEED_INCREMENT 5 // 速度调节增量
#define MAX_HOUR 24		  // 小时最大值
#define MAX_MINUTE 60	  // 分钟最大值
#define MAX_SECOND 60	  // 秒数最大值

/* ========== 阈值参数限制 ========== */
#define TEMP_THRESHOLD_MIN 15 // 温度阈值最小值
#define TEMP_THRESHOLD_MAX 50 // 温度阈值最大值
#define HUMI_THRESHOLD_MIN 20 // 湿度阈值最小值
#define HUMI_THRESHOLD_MAX 90 // 湿度阈值最大值
#define CONC_THRESHOLD_MIN 1  // 浓度阈值最小值
#define CONC_THRESHOLD_MAX 30 // 浓度阈值最大值

/* ========== OLED显示参数 ========== */
#define OLED_LINE_0 0		   // 第0行Y坐标
#define OLED_LINE_1 16		   // 第1行Y坐标
#define OLED_LINE_2 32		   // 第2行Y坐标
#define OLED_LINE_3 48		   // 第3行Y坐标
#define OLED_DISPLAY_WIDTH 128 // OLED显示宽度
#define OLED_LINE_SPACING 16   // OLED行间距

/* ========== 控制参数 ========== */
#define SERVO_CENTER_ANGLE 90	// 舵机中心角度
#define EXHAUST_SPEED 75		// 排烟速度（快速反转）
#define MIN_MOTOR_SPEED 20		// 最低电机转速
#define ALARM_DURATION_MS 400	// 警报持续时间(ms)
#define SERVO_SHAKE_DELAY_MS 25 // 舵机摇头延时(ms)
#define BLUETOOTH_MODE_RETURN 1 // 蓝牙模式返回值

uint8_t flag = 1; // 一级菜单标志位

/* ========== 风扇状态封装 ========== */
FanState_t g_FanState =
	{
		.speed = 0,			  // 风扇速度 (0-99)
		.direction = 1,		  // 旋转方向: 1顺时针（正转吹风）, -1逆时针（反转排烟）
		.shake = 0,			  // 摇头模式: 0关, 1开
		.timer_speed = 0,	  // 定时模式速度 (0-99)
		.timer_shake = 0,	  // 定时摇头模式: 0关, 1开
		.temp_threshold = 30, // 温度阈值
		.humi_threshold = 50, // 湿度阈值
		.conc_threshold = 15, // 浓度阈值
};

extern uint8_t hour, minute, second; // 定时设置

DHT11_Data_TypeDef DHT11_Data = {0}; // DHT11数据结构体

/* ========== 辅助函数 ========== */

/**
 * @brief  通用4行菜单绘制函数（带脏标记优化）
 * @param  lines: 4个字符串指针数组，分别对应4行内容
 * @param  highlight: 当前高亮行索引 (0~3)
 * @note   仅在 highlight 变化时重绘，减少OLED I2C通信开销
 */
static uint8_t s_last_highlight = 0xFF; // 脏标记：初始为无效值

static void Menu_DrawPage4(char *lines[4], uint8_t highlight)
{
	if (highlight == s_last_highlight)
		return; // 未变化，跳过重绘

	for (uint8_t i = 0; i < 4; i++)
	{
		OLED_ShowString(0, i * OLED_LINE_SPACING, lines[i], OLED_8X16);
	}
	OLED_ReverseArea(0, highlight * OLED_LINE_SPACING, OLED_DISPLAY_WIDTH, OLED_LINE_SPACING);
	OLED_Update();

	s_last_highlight = highlight;
}

/**
 * @brief  通用编辑页面绘制函数，绘制4行标签+数值并高亮指定编辑值
 * @param  labels: 4个标签字符串数组（如 "旋转方向:"），纯文本行传 NULL 则不显示数值
 * @param  values: 4个对应数值
 * @param  edit_line: 当前编辑行 (0-3)，该行的数值区域将被反色高亮
 * @param  edit_value: 当前编辑值，用于确定高亮宽度（<10为8px，>=10为16px）
 */
void Menu_DrawEditPage(const char *labels[4], uint8_t values[4], uint8_t edit_line, uint8_t edit_value)
{
	char buf[32];
	for (uint8_t i = 0; i < 4; i++)
	{
		snprintf(buf, sizeof(buf), "%s%d                        ", labels[i], (int8_t)values[i]);
		OLED_ShowString(0, i * OLED_LINE_SPACING, buf, OLED_8X16);
	}
	uint8_t reverse_width = (edit_value < 10) ? 8 : 16;
	OLED_ReverseArea(72, edit_line * OLED_LINE_SPACING, reverse_width, OLED_LINE_SPACING);
	OLED_Update();
}

/**
 * @brief  菜单索引环形切换辅助函数
 * @param  flag: 当前菜单标志位
 * @param  direction: 方向增量，1表示下一项，-1表示上一项
 * @param  max_items: 菜单最大项数
 * @note   该函数用于在 1~max_items 范围内循环切换
 */
static inline void Navigate_Menu(uint8_t *flag, int8_t direction, uint8_t max_items)
{
	*flag += direction;
	if (*flag > max_items)
		*flag = 1;
	if (*flag == 0)
		*flag = max_items;
}

/**
 * @brief  按指定步进调整数值并限制在给定范围内
 * @param  value: 待调整的数值指针
 * @param  direction: 调整方向，正数表示增加，负数表示减少
 * @param  min_val: 最小允许值
 * @param  max_val: 最大允许值
 * @param  step: 每次调整的步进
 */
void Adjust_Value(uint8_t *value, int8_t direction, uint8_t min_val, uint8_t max_val, uint8_t step)
{
	if (direction > 0)
	{
		if (*value + step >= max_val)
			*value = max_val;
		else
			*value += step;
	}
	else
	{
		if (*value <= min_val + step)
			*value = min_val;
		else
			*value -= step;
	}
}

/**
 * @brief  统一处理按键、编码器和蓝牙串口输入
 * @retval  1表示上移，-1表示下移，2表示确认，0表示无输入
 */
int8_t Handle_Input(void)
{
	int8_t NUM = Encoder_Get();
	HC05_GetData(RxData);

	if (RxSTA == 0)
	{
		RxSTA = 1;
		if (RxData[0] == 'U')
			return 1;
		if (RxData[0] == 'D')
			return -1;
		if (RxData[0] == 'Y')
			return 2;
	}

	if (NUM == 1)
		return 1;
	if (NUM == -1)
		return -1;
	if (Key_GetNum() == 1)
		return 2;

	return 0;
}

/**
 * @brief  仅处理蓝牙串口输入，用于模式切换相关界面
 * @retval  3表示切换到蓝牙模式，1表示上移，-1表示下移，2表示确认，0表示无输入
 */
int8_t Handle_BT_Input(void)
{
	HC05_GetData(RxData);
	if (RxSTA == 0)
	{
		RxSTA = 1;
		if (RxData[0] == 'M')
			return 3;
		if (RxData[0] == 'U')
			return 1;
		if (RxData[0] == 'D')
			return -1;
		if (RxData[0] == 'Y')
			return 2;
	}
	return 0;
}

/**
 * @brief  一级主菜单，负责在手动、自动、定时和蓝牙模式之间切换
 * @retval  返回选中的菜单标志位
 */
int Menu_Main(void)
{
	int8_t NUM;

	s_last_highlight = 0xFF; // 进入菜单时重置脏标记，强制首次重

	if (flag > MAIN_MENU_ITEMS) // 如果从蓝牙模式返回，重置为手动模式
	{
		flag = 1;
	}

	while (1)
	{
		// 统一硬件复位：关闭舵机、风扇、蜂鸣器、报警灯
		Servo_SetAngle(SERVO_CENTER_ANGLE);
		Motor_SetSpeed(0);
		Beep_Stop();
		LED1_OFF();

		NUM = Encoder_Get();
		HC05_GetData(RxData);
		if (RxSTA == 0)
		{
			RxSTA = 1;
			if (RxData[0] == 'A') // 蓝牙输入A直接进入蓝牙模式
			{
				flag = MAIN_MENU_ITEMS + 1; // 蓝牙模式
				return flag;
			}
		}
		if (NUM == (+1)) // 下一项
			Navigate_Menu(&flag, 1, MAIN_MENU_ITEMS);

		if (NUM == (-1)) // 上一项
			Navigate_Menu(&flag, -1, MAIN_MENU_ITEMS);

		if (Key_GetNum() == 1) // 确认
		{
			OLED_Clear();
			OLED_Update();
			return flag;
		}

		// 绘制主菜单（使用通用绘制函数，带脏标记优化）
		if (flag >= 1 && flag <= MAIN_MENU_ITEMS) // 旋钮选择只在前3个模式间循环
		{
			char *main_lines[4] = {
				"手动模式                        ",
				"自动模式                        ",
				"定时模式                        ",
				"蓝牙模式                        "};
			Menu_DrawPage4(main_lines, flag - 1);
		}
	}
}

/****
 * @brief 手动模式二级菜单函数
 * @param 无
 * @retval
 */
int Menu_Manual_Main(void)
{
	uint8_t menu3 = 0;
	uint8_t flag_1 = 1;
	int8_t NUM;

	s_last_highlight = 0xFF; // 进入菜单时重置脏标记，强制首次重绘

	while (1)
	{
		NUM = Encoder_Get();
		HC05_GetData(RxData);
		if (RxSTA == 0)
		{
			RxSTA = 1;
			switch (RxData[0])
			{
			case 'M':
				OLED_Clear();
				OLED_Update();
				return 2; // 切换到自动模式
			case 'U':	  // 下一项
				Navigate_Menu(&flag_1, 1, MANUAL_MENU_ITEMS);
				break;
			case 'D': // 上一项
				Navigate_Menu(&flag_1, -1, MANUAL_MENU_ITEMS);
				break;
			case 'Y': // 确认
				OLED_Clear();
				OLED_Update();
				menu3 = flag_1;
				break;
			}
		}

		if (NUM == (+1)) // 下一项
			Navigate_Menu(&flag_1, 1, MANUAL_MENU_ITEMS);

		if (NUM == (-1)) // 上一项
			Navigate_Menu(&flag_1, -1, MANUAL_MENU_ITEMS);

		if (Key_GetNum() == 1) // 确认
		{
			OLED_Clear();
			OLED_Update();
			menu3 = flag_1;
		}

		switch (menu3)
		{
		case 1:
			return 0;
		case 2:
			menu3 = Menu_Manual_SetDir();
			s_last_highlight = 0xFF;
			break;
		case 3:
			menu3 = Menu_Manual_SetSpeed();
			s_last_highlight = 0xFF;
			break;
		case 4:
			menu3 = Menu_Manual_SetShake();
			s_last_highlight = 0xFF;
			break;
		}

		char line0[32], line1[32], line2[32], line3[32];
		snprintf(line0, sizeof(line0), "返回                                ");
		snprintf(line1, sizeof(line1), "旋转方向:%d                        ", g_FanState.direction);
		snprintf(line2, sizeof(line2), "速度调节:%d                        ", g_FanState.speed);
		snprintf(line3, sizeof(line3), "摇头模式:%d                        ", g_FanState.shake);
		char *lines[4] = {line0, line1, line2, line3};
		Menu_DrawPage4(lines, flag_1 - 1);
	}
}

/****
 * @brief 手动模式三级菜单函数-----旋转方向
 * @param 无
 * @retval
 */
int Menu_Manual_SetDir(void)
{
	while (1)
	{
		switch (Handle_Input())
		{
		case 1: // 上
			g_FanState.direction = +1;
			break;
		case -1: // 下
			g_FanState.direction = -1;
			break;
		case 2: // 确认
			return 0;
		}

		{
			const char *labels[4] = {"返回              ", "旋转方向:", "速度调节:", "摇头模式:"};
			uint8_t vals[4] = {0, (uint8_t)g_FanState.direction, g_FanState.speed, g_FanState.shake};
			Menu_DrawEditPage(labels, vals, 1, (uint8_t)g_FanState.direction);
		}

		Motor_SetSpeed(g_FanState.speed * g_FanState.direction);
	}
}

/****
 * @brief 手动模式三级菜单函数-----速度调节
 * @param 无
 * @retval
 */
int Menu_Manual_SetSpeed(void)
{
	while (1)
	{
		switch (Handle_Input())
		{
		case 1: // 上
			Adjust_Value(&g_FanState.speed, 1, MIN_FAN_SPEED, MAX_FAN_SPEED, SPEED_INCREMENT);
			break;
		case -1: // 下
			Adjust_Value(&g_FanState.speed, -1, MIN_FAN_SPEED, MAX_FAN_SPEED, SPEED_INCREMENT);
			break;
		case 2: // 确认
			return 0;
		}

		{
			const char *labels[4] = {"返回              ", "旋转方向:", "速度调节:", "摇头模式:"};
			uint8_t vals[4] = {0, (uint8_t)g_FanState.direction, g_FanState.speed, g_FanState.shake};
			Menu_DrawEditPage(labels, vals, 2, g_FanState.speed);
		}

		Motor_SetSpeed(g_FanState.speed * g_FanState.direction);
	}
}

/****
 * @brief 手动模式三级菜单函数-----摇头模式
 * @param 无
 * @retval
 */
int Menu_Manual_SetShake(void)
{
	while (1)
	{
		switch (Handle_Input())
		{
		case 1: // 上
			g_FanState.shake = 1;
			break;
		case -1: // 下
			g_FanState.shake = 0;
			break;
		case 2: // 确认
			return 0;
		}

		{
			const char *labels[4] = {"返回              ", "旋转方向:", "速度调节:", "摇头模式:"};
			uint8_t vals[4] = {0, (uint8_t)g_FanState.direction, g_FanState.speed, g_FanState.shake};
			Menu_DrawEditPage(labels, vals, 3, g_FanState.shake);
		}

		if (g_FanState.shake == 1)
			Servo_Shake();
		else
			Servo_SetAngle(SERVO_CENTER_ANGLE);
	}
}

/****
 * @brief 自动模式二级菜单函数
 * @param 无
 * @retval
 */
int Menu_Auto_Main(void)
{
	uint8_t menu3 = 0;
	uint8_t flag_1 = 1;
	int8_t NUM;

	while (1)
	{
		NUM = Encoder_Get();
		HC05_GetData(RxData);
		if (RxSTA == 0)
		{
			RxSTA = 1;
			switch (RxData[0])
			{
			case 'M':
				OLED_Clear();
				OLED_Update();
				return 3; // 切换到定时模式
			case 'U':	  // 下一项
				Navigate_Menu(&flag_1, 1, AUTO_MENU_ITEMS);
				break;
			case 'D': // 上一项
				Navigate_Menu(&flag_1, -1, AUTO_MENU_ITEMS);
				break;
			case 'Y': // 确认
				OLED_Clear();
				OLED_Update();
				menu3 = flag_1;
				break;
			}
		}
		if (NUM == (+1)) // 下一项
			Navigate_Menu(&flag_1, 1, AUTO_MENU_ITEMS);

		if (NUM == (-1)) // 上一项
			Navigate_Menu(&flag_1, -1, AUTO_MENU_ITEMS);

		if ((Read_DHT11(&DHT11_Data) == SUCCESS) && (flag_1 <= 3))
		{
			OLED_Printf(0, 16, OLED_8X16, "温度:%d°                        ", DHT11_Data.temp_int);
			OLED_Printf(64, 16, OLED_8X16, "湿度:%d                        ", DHT11_Data.humi_int);
			OLED_ShowString(120, 16, "%                                    ", OLED_8X16);
			OLED_Update();
		}

		// 读取MQ2浓度（缓存结果，避免重复采样）
		float ppm_value = MQ2_GetData_PPM();

		// 非阻塞报警状态机
		static uint8_t alarm_active = 0;
		static uint16_t alarm_counter = 0;

		if (ppm_value >= g_FanState.conc_threshold)
		{
			Motor_SetSpeed(-EXHAUST_SPEED); // 快速反转进行排烟
			if (!alarm_active)
			{
				Beep_Start();
				LED1_ON();
				alarm_active = 1;
				alarm_counter = 0;
			}
			else
			{
				alarm_counter++;
				if (alarm_counter >= 40) // 约400ms后关闭声光（每次循环约10ms）
				{
					Beep_Stop();
					LED1_OFF();
					alarm_active = 0;
				}
			}
		}
		else
		{
			if (alarm_active)
			{
				Beep_Stop();
				LED1_OFF();
				alarm_active = 0;
			}

			if ((DHT11_Data.temp_int >= g_FanState.temp_threshold) || (DHT11_Data.humi_int >= g_FanState.humi_threshold))
			{
				if ((DHT11_Data.temp_int - g_FanState.temp_threshold) >= (DHT11_Data.humi_int - g_FanState.humi_threshold))
				{
					if ((10 * DHT11_Data.temp_int - 250) < MIN_MOTOR_SPEED)
					{
						Motor_SetSpeed(MIN_MOTOR_SPEED);
					}
					else
					{
						Motor_SetSpeed(10 * DHT11_Data.temp_int - 250);
					}
				}
				else
				{
					if ((2 * DHT11_Data.humi_int - 100) < MIN_MOTOR_SPEED)
					{
						Motor_SetSpeed(MIN_MOTOR_SPEED);
					}
					else
					{
						Motor_SetSpeed(2 * DHT11_Data.humi_int - 100);
					}
				}
			}
			else
			{
				Motor_SetSpeed(0);
			}
		}

		if (flag_1 == 4)
		{
			OLED_Printf(0, OLED_LINE_0, OLED_8X16, "检测浓度:%.1fppm                        ", ppm_value);
			OLED_Update();
		}
		if (Key_GetNum() == 1) // 确认
		{
			OLED_Clear();
			OLED_Update();
			menu3 = flag_1;
		}

		switch (menu3)
		{
		case 1:
			return 0;
		case 2:
			menu3 = Menu_Auto_SetTemp();
			break;
		case 3:
			menu3 = Menu_Auto_SetHumi();
			break;
		case 4:
			menu3 = Menu_Auto_SetGas();
			break;
		}

		switch (flag_1)
		{
		case 1:
		{
			OLED_ShowString(0, OLED_LINE_0, "返回                                ", OLED_8X16);
			OLED_Printf(0, OLED_LINE_2, OLED_8X16, "温度阈值:%d                        ", g_FanState.temp_threshold);
			OLED_Printf(0, OLED_LINE_3, OLED_8X16, "湿度阈值:%d                        ", g_FanState.humi_threshold);
			OLED_ReverseArea(0, OLED_LINE_0, OLED_DISPLAY_WIDTH, OLED_LINE_SPACING);
			OLED_Update();
			break;
		}
		case 2:
		{
			OLED_ShowString(0, OLED_LINE_0, "返回                                ", OLED_8X16);
			OLED_Printf(0, OLED_LINE_2, OLED_8X16, "温度阈值:%d                        ", g_FanState.temp_threshold);
			OLED_Printf(0, OLED_LINE_3, OLED_8X16, "湿度阈值:%d                        ", g_FanState.humi_threshold);
			OLED_ReverseArea(0, OLED_LINE_2, OLED_DISPLAY_WIDTH, OLED_LINE_SPACING);
			OLED_Update();
			break;
		}
		case 3:
		{
			OLED_ShowString(0, OLED_LINE_0, "返回                                ", OLED_8X16);
			OLED_Printf(0, OLED_LINE_2, OLED_8X16, "温度阈值:%d                        ", g_FanState.temp_threshold);
			OLED_Printf(0, OLED_LINE_3, OLED_8X16, "湿度阈值:%d                        ", g_FanState.humi_threshold);
			OLED_ReverseArea(0, OLED_LINE_3, OLED_DISPLAY_WIDTH, OLED_LINE_SPACING);
			OLED_Update();
			break;
		}
		case 4:
		{
			OLED_Printf(0, OLED_LINE_1, OLED_8X16, "浓度阈值:%d                        ", g_FanState.conc_threshold);
			OLED_ShowString(0, OLED_LINE_2, "                                          ", OLED_8X16);
			OLED_ShowString(0, OLED_LINE_3, "                                          ", OLED_8X16);
			OLED_ReverseArea(0, OLED_LINE_1, OLED_DISPLAY_WIDTH, OLED_LINE_SPACING);
			OLED_Update();
			break;
		}
		}
	}
}

/****
 * @brief 自动模式三级菜单函数-----温度阈值调节
 * @param 无
 * @retval
 */
int Menu_Auto_SetTemp(void)
{
	while (1)
	{
		if (Read_DHT11(&DHT11_Data) == SUCCESS)
		{
			OLED_Printf(0, 16, OLED_8X16, "温度:%d°                        ", DHT11_Data.temp_int);
			OLED_Printf(64, 16, OLED_8X16, "湿度:%d                        ", DHT11_Data.humi_int);
			OLED_ShowString(120, 16, "%                                    ", OLED_8X16);
			OLED_Update();
		}

		switch (Handle_Input())
		{
		case 1: // 上
			Adjust_Value(&g_FanState.temp_threshold, 1, TEMP_THRESHOLD_MIN, TEMP_THRESHOLD_MAX, 1);
			break;
		case -1: // 下
			Adjust_Value(&g_FanState.temp_threshold, -1, TEMP_THRESHOLD_MIN, TEMP_THRESHOLD_MAX, 1);
			break;
		case 2: // 确认
			return 0;
		}

		OLED_ShowString(0, 0, "返回                                ", OLED_8X16);
		OLED_Printf(0, 32, OLED_8X16, "温度阈值:%d                        ", g_FanState.temp_threshold);
		OLED_Printf(0, 48, OLED_8X16, "湿度阈值:%d                        ", g_FanState.humi_threshold);
		if (DHT11_Data.temp_int < 10)
		{
			OLED_ReverseArea(72, 32, 8, 16);
		}
		else
		{
			OLED_ReverseArea(72, 32, 16, 16);
		}
		OLED_Update();
	}
}

/****
 * @brief 自动模式三级菜单函数-----湿度阈值调节
 * @param 无
 * @retval
 */
int Menu_Auto_SetHumi(void)
{
	while (1)
	{
		if (Read_DHT11(&DHT11_Data) == SUCCESS)
		{
			OLED_Printf(0, 16, OLED_8X16, "温度:%d°                        ", DHT11_Data.temp_int);
			OLED_Printf(64, 16, OLED_8X16, "湿度:%d                        ", DHT11_Data.humi_int);
			OLED_ShowString(120, 16, "%                                    ", OLED_8X16);
			OLED_Update();
		}

		switch (Handle_Input())
		{
		case 1: // 上
			Adjust_Value(&g_FanState.humi_threshold, 1, HUMI_THRESHOLD_MIN, HUMI_THRESHOLD_MAX, 1);
			break;
		case -1: // 下
			Adjust_Value(&g_FanState.humi_threshold, -1, HUMI_THRESHOLD_MIN, HUMI_THRESHOLD_MAX, 1);
			break;
		case 2: // 确认
			return 0;
		}

		OLED_ShowString(0, 0, "返回                                ", OLED_8X16);
		OLED_Printf(0, 32, OLED_8X16, "温度阈值:%d                        ", g_FanState.temp_threshold);
		OLED_Printf(0, 48, OLED_8X16, "湿度阈值:%d                        ", g_FanState.humi_threshold);
		if (DHT11_Data.humi_int < 10)
		{
			OLED_ReverseArea(72, 48, 8, 16);
		}
		else
		{
			OLED_ReverseArea(72, 48, 16, 16);
		}
		OLED_Update();
	}
}

/****
 * @brief 自动模式三级菜单函数-----浓度阈值调节
 * @param 无
 * @retval
 */
int Menu_Auto_SetGas(void)
{
	float PPM;

	while (1)
	{
		PPM = MQ2_GetData_PPM();
		OLED_Printf(0, 0, OLED_8X16, "检测浓度:%.1fppm                        ", PPM); // 千万注意不能超过15ppm
		OLED_Update();

		switch (Handle_Input())
		{
		case 1: // 上
			Adjust_Value(&g_FanState.conc_threshold, 1, CONC_THRESHOLD_MIN, CONC_THRESHOLD_MAX, 1);
			break;
		case -1: // 下
			Adjust_Value(&g_FanState.conc_threshold, -1, CONC_THRESHOLD_MIN, CONC_THRESHOLD_MAX, 1);
			break;
		case 2: // 确认
			return 0;
		}

		OLED_Printf(0, 16, OLED_8X16, "浓度阈值:%d                        ", g_FanState.conc_threshold);
		OLED_ShowString(0, 32, "                                          ", OLED_8X16);
		OLED_ShowString(0, 48, "                                          ", OLED_8X16);
		if (g_FanState.conc_threshold < 10)
		{
			OLED_ReverseArea(72, 16, 8, 16);
		}
		else
		{
			OLED_ReverseArea(72, 16, 16, 16);
		}
		OLED_Update();
	}
}

/****
 * @brief 定时模式二级菜单函数
 * @param 无
 * @retval
 */
int Menu_Timer_Main(void)
{
	uint8_t menu_3 = 0;
	uint8_t flag_1 = 1;
	int8_t NUM;

	s_last_highlight = 0xFF; // 进入菜单时重置脏标记，强制首次重绘

	while (1)
	{
		Servo_SetAngle(SERVO_CENTER_ANGLE);
		Motor_SetSpeed(0);
		NUM = Encoder_Get();
		HC05_GetData(RxData);
		if (RxSTA == 0)
		{
			RxSTA = 1;
			switch (RxData[0])
			{
			case 'M':
				OLED_Clear();
				OLED_Update();
				return 1; // 切换回手动模式
			case 'U':	  // 下一项
				Navigate_Menu(&flag_1, 1, TIMER_MENU_ITEMS);
				break;
			case 'D': // 上一项
				Navigate_Menu(&flag_1, -1, TIMER_MENU_ITEMS);
				break;
			case 'Y': // 确认
				OLED_Clear();
				OLED_Update();
				menu_3 = flag_1;
				break;
			}
		}
		if (NUM == (+1)) // 下一项
			Navigate_Menu(&flag_1, 1, TIMER_MENU_ITEMS);

		if (NUM == (-1)) // 上一项
			Navigate_Menu(&flag_1, -1, TIMER_MENU_ITEMS);

		if (Key_GetNum() == 1) // 确认
		{
			OLED_Clear();
			OLED_Update();
			menu_3 = flag_1;
		}

		switch (menu_3)
		{
		case 1:
			return 0;
		case 2:
			menu_3 = Menu_Timer_SetHour();
			s_last_highlight = 0xFF;
			break;
		case 3:
			menu_3 = Menu_Timer_SetMinute();
			s_last_highlight = 0xFF;
			break;
		case 4:
			menu_3 = Menu_Timer_SetSecond();
			s_last_highlight = 0xFF;
			break;
		case 5:
			menu_3 = Menu_Timer_SetDir();
			s_last_highlight = 0xFF;
			break;
		case 6:
			menu_3 = Menu_Timer_SetSpeed();
			s_last_highlight = 0xFF;
			break;
		case 7:
			menu_3 = Menu_Timer_SetShake();
			s_last_highlight = 0xFF;
			break;
		case 8:
			menu_3 = Menu_Timer_Execute();
			s_last_highlight = 0xFF;
			break;
		}

		char line0[32], line1[32], line2[32], line3[32];
		char *lines[4] = {line0, line1, line2, line3};
		uint8_t highlight = (flag_1 - 1) % 4;

		if (flag_1 <= 4)
		{
			snprintf(line0, sizeof(line0), "返回                                ");
			snprintf(line1, sizeof(line1), "设置小时:%d                        ", hour);
			snprintf(line2, sizeof(line2), "设置分钟:%d                        ", minute);
			snprintf(line3, sizeof(line3), "设置秒数:%d                        ", second);
		}
		else
		{
			snprintf(line0, sizeof(line0), "设置方向:%d                        ", g_FanState.direction);
			snprintf(line1, sizeof(line1), "设置速度:%d                        ", g_FanState.timer_speed);
			snprintf(line2, sizeof(line2), "设置摇头:%d                        ", g_FanState.timer_shake);
			snprintf(line3, sizeof(line3), "确定                                    ");
		}

		Menu_DrawPage4(lines, highlight);
	}
}

/****
 * @brief 定时模式三级菜单函数-----小时调节
 * @param 无
 * @retval
 */
int Menu_Timer_SetHour(void)
{
	while (1)
	{
		switch (Handle_Input())
		{
		case 1: // 上
			Adjust_Value(&hour, 1, 0, MAX_HOUR, 1);
			break;
		case -1: // 下
			Adjust_Value(&hour, -1, 0, MAX_HOUR, 1);
			break;
		case 2: // 确认
			return 0;
		}

		{
			const char *labels[4] = {"返回              ", "设置小时:", "设置分钟:", "设置秒数:"};
			uint8_t vals[4] = {0, hour, minute, second};
			Menu_DrawEditPage(labels, vals, 1, hour);
		}
	}
}

/****
 * @brief 定时模式三级菜单函数-----分钟调节
 * @param 无
 * @retval
 */
int Menu_Timer_SetMinute(void)
{
	while (1)
	{
		switch (Handle_Input())
		{
		case 1: // 上
			Adjust_Value(&minute, 1, 0, MAX_MINUTE, 1);
			break;
		case -1: // 下
			Adjust_Value(&minute, -1, 0, MAX_MINUTE, 1);
			break;
		case 2: // 确认
			return 0;
		}

		{
			const char *labels[4] = {"返回              ", "设置小时:", "设置分钟:", "设置秒数:"};
			uint8_t vals[4] = {0, hour, minute, second};
			Menu_DrawEditPage(labels, vals, 2, minute);
		}
	}
}

/****
 * @brief 定时模式三级菜单函数-----秒数调节
 * @param 无
 * @retval
 */
int Menu_Timer_SetSecond(void)
{
	while (1)
	{
		switch (Handle_Input())
		{
		case 1: // 上
			Adjust_Value(&second, 1, 0, MAX_SECOND, 1);
			break;
		case -1: // 下
			Adjust_Value(&second, -1, 0, MAX_SECOND, 1);
			break;
		case 2: // 确认
			return 0;
		}

		{
			const char *labels[4] = {"返回              ", "设置小时:", "设置分钟:", "设置秒数:"};
			uint8_t vals[4] = {0, hour, minute, second};
			Menu_DrawEditPage(labels, vals, 3, second);
		}
	}
}

/****
 * @brief 定时模式三级菜单函数-----设置方向
 * @param 无
 * @retval
 */
int Menu_Timer_SetDir(void)
{
	while (1)
	{
		switch (Handle_Input())
		{
		case 1: // 上
			g_FanState.direction = +1;
			break;
		case -1: // 下
			g_FanState.direction = -1;
			break;
		case 2: // 确认
			return 0;
		}

		{
			const char *labels[4] = {"设置方向:", "设置速度:", "设置摇头:", "确定              "};
			uint8_t vals[4] = {(uint8_t)g_FanState.direction, g_FanState.timer_speed, g_FanState.timer_shake, 0};
			Menu_DrawEditPage(labels, vals, 0, (uint8_t)g_FanState.direction);
		}
	}
}

/****
 * @brief 定时模式三级菜单函数-----设置速度
 * @param 无
 * @retval
 */
int Menu_Timer_SetSpeed(void)
{
	while (1)
	{
		switch (Handle_Input())
		{
		case 1: // 上
			Adjust_Value(&g_FanState.timer_speed, 1, MIN_FAN_SPEED, MAX_FAN_SPEED, SPEED_INCREMENT);
			break;
		case -1: // 下
			Adjust_Value(&g_FanState.timer_speed, -1, MIN_FAN_SPEED, MAX_FAN_SPEED, SPEED_INCREMENT);
			break;
		case 2: // 确认
			return 0;
		}

		{
			const char *labels[4] = {"设置方向:", "设置速度:", "设置摇头:", "确定              "};
			uint8_t vals[4] = {(uint8_t)g_FanState.direction, g_FanState.timer_speed, g_FanState.timer_shake, 0};
			Menu_DrawEditPage(labels, vals, 1, g_FanState.timer_speed);
		}
	}
}

/****
 * @brief 定时模式三级菜单函数-----摇头模式
 * @param 无
 * @retval
 */
int Menu_Timer_SetShake(void)
{
	while (1)
	{
		switch (Handle_Input())
		{
		case 1: // 上
			g_FanState.timer_shake = 1;
			break;
		case -1: // 下
			g_FanState.timer_shake = 0;
			break;
		case 2: // 确认
			return 0;
		}

		{
			const char *labels[4] = {"设置方向:", "设置速度:", "设置摇头:", "确定              "};
			uint8_t vals[4] = {(uint8_t)g_FanState.direction, g_FanState.timer_speed, g_FanState.timer_shake, 0};
			Menu_DrawEditPage(labels, vals, 2, g_FanState.timer_shake);
		}
	}
}

/****
 * @brief  TIM1更新中断服务函数，处理定时模式倒计时及结束复位
 * @param  无
 * @retval 无
 */
void TIM1_UP_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
	{
		if (second > 0)
		{
			second--;
		}
		else // second == 0
		{
			if (minute > 0)
			{
				minute--;
				second = 59;
			}
			else // minute == 0
			{
				if (hour > 0)
				{
					hour--;
					minute = 59;
					second = 59;
				}
				else // hour == 0, minute == 0, second == 0
				{
					g_FanState.direction = 1;
					g_FanState.timer_speed = 0;
					g_FanState.timer_shake = 0;
					TIM_Cmd(TIM1, DISABLE);
				}
			}
		}
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	}
}

/****
 * @brief  定时模式执行界面，启动定时器并根据设定参数（速度、方向、摇头）驱动风扇运行
 * @param  无
 * @retval 返回0，表示用户按下确认键或接收到退出指令，返回上一级菜单
 */
int Menu_Timer_Execute(void)
{
	TIM_Cmd(TIM1, ENABLE);

	while (1)
	{
		if (Handle_Input() != 0) // 任意输入（按键、旋钮或蓝牙）即退出
		{
			return 0;
		}

		// 更新显示
		OLED_Printf(0, OLED_LINE_0, OLED_8X16, "旋转方向:%d      ", g_FanState.direction);
		OLED_Printf(0, OLED_LINE_1, OLED_8X16, "旋转速度:%d      ", g_FanState.timer_speed);
		OLED_Printf(0, OLED_LINE_2, OLED_8X16, "摇头模式:%d      ", g_FanState.timer_shake);
		OLED_ShowString(0, OLED_LINE_3, "倒计时:  :  :     ", OLED_8X16);
		OLED_ShowNum(56, OLED_LINE_3, hour, 2, OLED_8X16);
		OLED_ShowNum(80, OLED_LINE_3, minute, 2, OLED_8X16);
		OLED_ShowNum(104, OLED_LINE_3, Timer_Get(), 2, OLED_8X16);
		OLED_Update();

		// 控制电机
		Motor_SetSpeed(g_FanState.timer_speed * g_FanState.direction);

		// 控制舵机
		if (g_FanState.timer_shake == 1)
		{
			Servo_Shake();					// 现在是非阻塞的
			Delay_ms(SERVO_SHAKE_DELAY_MS); // 控制摇头速度
		}
		else
		{
			Servo_SetAngle(SERVO_CENTER_ANGLE);
		}
	}
}

/**
 * @brief  蓝牙模式入口界面，播放提示音并等待模式切换指令
 * @retval  1表示返回手动模式
 */
int Menu_Bluetooth_Main(void)
{
	Beep_Start();
	LED1_ON();
	Delay_ms(ALARM_DURATION_MS);
	Beep_Stop();
	LED1_OFF();

	// 静态内容只绘制一次，避免每帧清屏闪烁
	OLED_Clear();
	OLED_ShowString(49, 16, "点击                                          ", OLED_8X16);
	OLED_ShowString(33, 32, "模式切换                                          ", OLED_8X16);
	OLED_Update();

	while (1)
	{
		HC05_GetData(RxData);
		if (RxSTA == 0)
		{
			RxSTA = 1;
			if (RxData[0] == 'M')
			{
				OLED_Clear();
				OLED_Update();
				return 1; // 切换回手动模式
			}
		}
	}
}
