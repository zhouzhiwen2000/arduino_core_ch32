#ifndef Servo_h
#define Servo_h

#include "Arduino.h"
#include "debug.h"

#ifdef __cplusplus
#define SERVO_MAX_CHANNELS    12
#define MIN_PULSE_WIDTH       500     // 最小脉宽，微秒
#define MAX_PULSE_WIDTH      2500     // 最大脉宽，微秒
#define DEFAULT_PULSE_WIDTH  500     // 默认脉宽，微秒
#define REFRESH_INTERVAL    20000     // 刷新周期，微秒

class Servo
{
public:
    Servo();
    uint8_t attach(int pin);                         // 绑定舵机到某个引脚
    uint8_t attach(int pin, int min, int max);       // 绑定并设置最小最大脉宽
    void detach();                                   // 解绑舵机
    void write(int value);                           // 写角度或脉宽
    void writeMicroseconds(int value);               // 写脉宽
    int read();                                      // 读角度
    int readMicroseconds();                          // 读脉宽
    bool attached();                                 // 是否已绑定
private:
    uint8_t channel;                                 // 通道号
    int minPulse;
    int maxPulse;
};

void Servo_Timer_Init();                             // 定时器初始化
void Servo_GPIO_Init(uint8_t channel);               // GPIO初始化

// 供中断调用的脉宽数组
extern volatile uint16_t servo_pulse_buffer[SERVO_MAX_CHANNELS];
extern volatile uint8_t servo_active[SERVO_MAX_CHANNELS];
#endif

#endif