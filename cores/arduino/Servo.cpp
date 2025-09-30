#include "Servo.h"

#define SERVO_MAX_CHANNELS    12
#define SERVO_REFRESH_INTERVAL      20000 // us

typedef struct {
    uint8_t pin;
    uint8_t isActive;
    uint16_t pulse; // us
} servo_t;

static servo_t servos[SERVO_MAX_CHANNELS];
static volatile int8_t current_channel = -1;
static volatile uint32_t next_compare = 0;

// 舵机脉宽缓冲区和激活标志
volatile uint16_t servo_pulse_buffer[SERVO_MAX_CHANNELS] = {0};
volatile uint8_t servo_active[SERVO_MAX_CHANNELS] = {0};

// 引脚映射表（与 main.cpp 保持一致）
static const uint16_t pwm_pin_table[SERVO_MAX_CHANNELS] = {
    GPIO_Pin_0, GPIO_Pin_1, GPIO_Pin_3, GPIO_Pin_4,
    GPIO_Pin_5, GPIO_Pin_6, GPIO_Pin_7, GPIO_Pin_8,
    GPIO_Pin_9, GPIO_Pin_10, GPIO_Pin_11, GPIO_Pin_12};

static uint8_t servo_count = 0; // 当前已用通道数

// 多路PWM输出引脚初始化
static void Servo_GPIO_Init(uint8_t arduino_pin)
{
    if (arduino_pin < 2 || arduino_pin > 13)
        return;
    uint8_t idx = arduino_pin - 2;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = pwm_pin_table[idx];
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOB, pwm_pin_table[idx]); // 初始化为低电平
}

void Servo_Timer_Init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF; // 最大周期
    TIM_TimeBaseStructure.TIM_Prescaler = 144-1; // 1MHz, 1us/tick
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    TIM_SetCounter(TIM1, 0);
    TIM_ITConfig(TIM1, TIM_IT_CC1, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure = {0};
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM1, ENABLE);
}

// 中断服务函数（非阻塞多通道舵机核心）
void TIM1_CC_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_CC1) == SET)
    {
        TIM_ClearITPendingBit(TIM1, TIM_IT_CC1);

        static uint32_t last_cycle_start = 0;
        uint32_t now = TIM_GetCounter(TIM1);

        // 上一个通道拉低
        if (current_channel >= 0 && current_channel < SERVO_MAX_CHANNELS && servos[current_channel].isActive) {
            uint8_t idx = servos[current_channel].pin - 2;
            GPIO_ResetBits(GPIOB, pwm_pin_table[idx]);
        }

        // 查找下一个激活通道
        int next_ch = current_channel + 1;
        while (next_ch < SERVO_MAX_CHANNELS && !servos[next_ch].isActive) next_ch++;

        if (next_ch < SERVO_MAX_CHANNELS) {
            // 拉高当前通道
            uint8_t idx = servos[next_ch].pin - 2;
            GPIO_SetBits(GPIOB, pwm_pin_table[idx]);
            // 设置下次比较中断时间
            next_compare = now + servos[next_ch].pulse;
            TIM_SetCompare1(TIM1, next_compare);
            current_channel = next_ch;
        } else {
            // 所有通道输出完，等待刷新周期
            last_cycle_start += SERVO_REFRESH_INTERVAL;
            next_compare = last_cycle_start;
            TIM_SetCompare1(TIM1, next_compare);
            current_channel = -1;
        }
    }
}

Servo::Servo() : channel(0xFF), minPulse(MIN_PULSE_WIDTH), maxPulse(MAX_PULSE_WIDTH) {}

uint8_t Servo::attach(int pin) {
    return attach(pin, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
}

uint8_t Servo::attach(int pin, int min, int max)
{
    if (pin < 2 || pin > 13) return 0xFF;
    uint8_t ch = pin - 2;
    channel = ch;
    servos[ch].pin = pin;
    servos[ch].isActive = 1;
    servos[ch].pulse = DEFAULT_PULSE_WIDTH;
    Servo_GPIO_Init(pin);
    Servo_Timer_Init();
    return ch;
}

void Servo::detach() {
    if (channel < SERVO_MAX_CHANNELS) {
        servo_active[channel] = 0;
    }
}

void Servo::write(int value) {
    if (value < 200) {
        // 角度模式
        if (value < 0) value = 0;
        if (value > 180) value = 180;
        int us = minPulse + (maxPulse - minPulse) * value / 180;
        writeMicroseconds(us);
    } else {
        // 直接脉宽
        writeMicroseconds(value);
    }
}

void Servo::writeMicroseconds(int value) {
    if (channel < SERVO_MAX_CHANNELS) {
        if (value < MIN_PULSE_WIDTH) value = MIN_PULSE_WIDTH;
        if (value > MAX_PULSE_WIDTH) value = MAX_PULSE_WIDTH;
        servos[channel].pulse = value;
    }
}

int Servo::read() {
    if (channel < SERVO_MAX_CHANNELS) {
        int us = servo_pulse_buffer[channel];
        return (us - minPulse) * 180 / (maxPulse - minPulse);
    }
    return 0;
}

int Servo::readMicroseconds() {
    if (channel < SERVO_MAX_CHANNELS) {
        return servo_pulse_buffer[channel];
    }
    return 0;
}

bool Servo::attached() {
    if (channel < SERVO_MAX_CHANNELS) {
        return servo_active[channel] != 0;
    }
    return false;
}