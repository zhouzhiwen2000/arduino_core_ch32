#include "wifi.h"
#include <string.h>
#include "mytime.h"

// 全局缓冲区初始化（供中断和类访问）
volatile u8  wifi_rx_buffer[RX_BUF_SIZE] = {0};
volatile u16 wifi_rx_len = 0;
volatile u8  wifi_rx_flag = 0;

// 静态实例指针
static Wifi *wifiInstance = NULL;

Wifi::Wifi()
{
    clearRxBuffer();
    wifiInstance = this;  // 保存实例指针供中断使用
}

void Wifi::init(u32 baudrate)
{
    USART2_Init(baudrate);
}

void Wifi::sendString(u8 *str)
{
    while (*str) {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
        USART_SendData(USART2, *str++);
    }
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
}

// 原函数（兼容旧调用，内部转发到新函数）
u8 Wifi::sendATCmd(u8 *cmd, u8 *resp, u32 timeout)
{
    // 旧调用不关心接收数据，传 NULL 和 0 即可
    return sendATCmd(cmd, resp, timeout, NULL, 0);
}

// 新函数（带接收数据参数）
u8 Wifi::sendATCmd(u8 *cmd, u8 *resp, u32 timeout, u8 *recvData, u16 recvBufSize)
{
    clearRxBuffer();
    u32 time = timeout;
    Serial.printf("Send: \r\n%s\r\n", cmd);

    sendString(cmd);
    sendString((u8*)"\r\n");

    while (time--) {
        if (wifi_rx_flag) {
            // 复制接收数据到输出缓冲区（如果参数有效）
            if (recvData != NULL && recvBufSize > 0) {
                u16 copyLen = (wifi_rx_len < recvBufSize - 1) ? wifi_rx_len : (recvBufSize - 1);
                memcpy(recvData, (u8*)wifi_rx_buffer, copyLen);
                recvData[copyLen] = '\0'; // 确保字符串结束
            }

            // 检查期望响应
            if (strstr((char*)wifi_rx_buffer, (char*)resp) != NULL) {
                Serial.printf("Receive: \r\n%s\r\n", wifi_rx_buffer);
                wifi_rx_flag = 0;
                return 0; // 成功
            }
            wifi_rx_flag = 0;
        }
        delay(1);
    }

    // 超时处理：复制已接收数据（如果参数有效）
    if (wifi_rx_len > 0 && recvData != NULL && recvBufSize > 0) {
        u16 copyLen = (wifi_rx_len < recvBufSize - 1) ? wifi_rx_len : (recvBufSize - 1);
        memcpy(recvData, (u8*)wifi_rx_buffer, copyLen);
        recvData[copyLen] = '\0';
    }

    Serial.printf("Timeout waiting for response: %s\r\n", resp);
    return 1; // 超时或失败
}

bool Wifi::isReceived()
{
    return wifi_rx_flag != 0;
}

u8* Wifi::getRxBuffer()
{
    return (u8*)wifi_rx_buffer;
}

void Wifi::clearRxBuffer()
{
    memset((u8*)wifi_rx_buffer, 0, RX_BUF_SIZE);
    wifi_rx_len = 0;
    wifi_rx_flag = 0;
}

void USART2_Init(u32 baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* USART2 TX-->PA.2   RX-->PA.3 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* USART参数配置 */
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2, &USART_InitStructure);

    /* 使能接收中断 */
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    /* 配置中断优先级 */
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART2, ENABLE);
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        u8 data = USART_ReceiveData(USART2);
        wifi_rx_buffer[wifi_rx_len++] = data;

        // 遇到回车换行认为接收完成或缓冲区满
        if ((wifi_rx_len >= 2 && 
             wifi_rx_buffer[wifi_rx_len - 2] == '\r' && 
             wifi_rx_buffer[wifi_rx_len - 1] == '\n') || 
            wifi_rx_len >= RX_BUF_SIZE - 1) {
            wifi_rx_buffer[wifi_rx_len] = '\0';  // 字符串结束符
            wifi_rx_flag = 1;
        }
    }
}