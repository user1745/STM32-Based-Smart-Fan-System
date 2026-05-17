#ifndef __OLEDMENU_H
#define __OLEDMENU_H

/* ========== 风扇状态封装结构体 ========== */
typedef struct
{
	uint8_t speed;			// 风扇速度 (0-99)
	int8_t direction;		// 旋转方向: 1顺时针, -1逆时针
	uint8_t shake;			// 摇头模式: 0关, 1开
	uint8_t timer_speed;	// 定时模式速度 (0-99)
	uint8_t timer_shake;	// 定时摇头模式: 0关, 1开
	uint8_t temp_threshold; // 温度阈值
	uint8_t humi_threshold; // 湿度阈值
	uint8_t conc_threshold; // 浓度阈值
} FanState_t;

extern FanState_t g_FanState; // 全局风扇状态

/* ========== 菜单接口 ========== */

// 一级主菜单
int Menu_Main(void);

// 二级菜单：手动模式
int Menu_Manual_Main(void);

int Menu_Manual_SetDir(void);
int Menu_Manual_SetSpeed(void);
int Menu_Manual_SetShake(void);

// 二级菜单：自动模式
int Menu_Auto_Main(void);

int Menu_Auto_SetTemp(void);
int Menu_Auto_SetHumi(void);
int Menu_Auto_SetGas(void);

// 二级菜单：定时模式
int Menu_Timer_Main(void);

int Menu_Timer_SetHour(void);
int Menu_Timer_SetMinute(void);
int Menu_Timer_SetSecond(void);
int Menu_Timer_SetDir(void);
int Menu_Timer_SetSpeed(void);
int Menu_Timer_SetShake(void);
int Menu_Timer_Execute(void);

// 二级菜单：蓝牙模式
int Menu_Bluetooth_Main(void);

/* ========== 辅助函数 ========== */
void Adjust_Value(uint8_t *value, int8_t direction, uint8_t min_val, uint8_t max_val, uint8_t step);
int8_t Handle_Input(void);
int8_t Handle_BT_Input(void);
void Menu_DrawEditPage(const char *labels[4], uint8_t values[4], uint8_t edit_line, uint8_t edit_value);

#endif
