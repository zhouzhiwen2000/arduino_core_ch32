#ifndef wifi_h
#define wifi_h

#include "Arduino.h"
#include "debug.h"

#ifdef __cplusplus

#define USART_BAUDRATE 115200 // AI-WB2-12S默认波特率
#define RX_BUF_SIZE 1024       // 接收缓冲区大小

// 供中断访问的全局缓冲区（参考Servo的做法）
extern volatile u8 wifi_rx_buffer[RX_BUF_SIZE];
extern volatile u16 wifi_rx_len;
extern volatile u8 wifi_rx_flag;

class Wifi
{
public:
    Wifi();
    void init(u32 baudrate = USART_BAUDRATE); // 初始化WiFi模块
    void sendString(u8 *str);                 // 发送字符串
    // 原函数声明（不修改，兼容旧调用）
    u8 sendATCmd(u8 *cmd, u8 *resp, u32 timeout);
    // 新增重载函数（带接收数据参数）
    u8 sendATCmd(u8 *cmd, u8 *resp, u32 timeout, u8 *recvData, u16 recvBufSize);
    bool isReceived();    // 检查是否有接收数据
    u8 *getRxBuffer();    // 获取接收缓冲区
    void clearRxBuffer(); // 清空接收缓冲区

private:
    // 私有成员改为使用全局缓冲区
};

void USART2_Init(u32 baudrate); // USART2初始化
void USART2_IRQHandler(void);   // USART2中断服务函数

#endif

#endif