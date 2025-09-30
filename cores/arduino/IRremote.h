#ifndef IRremote_h
#define IRremote_h

#include "Arduino.h"

#ifdef __cplusplus
// NEC协议相关定义
#define NEC_BITS        32
#define NEC_HDR_MARK    9000
#define NEC_HDR_SPACE   4500
#define NEC_BIT_MARK    560
#define NEC_ONE_SPACE   1690
#define NEC_ZERO_SPACE  560
#define NEC_RPT_SPACE   2250

// 解码结果结构体
typedef struct {
    uint32_t value;      // 解码出的数据
    uint8_t  bits;       // 位数
    uint8_t  decode_type;// 协议类型（1=NEC）
} decode_results;

// 红外接收类
class IRrecv {
public:
    IRrecv(uint8_t recvpin);
    void enableIRIn();
    bool decode(decode_results *results);
    void resume();

    // 软采样，需在loop中周期性调用
    void IRpoll();

private:
    uint8_t _recvpin;
    uint8_t _lastLevel;
    uint32_t _lastTime;
    uint16_t _rawbuf[100];
    uint8_t _rawlen;
    bool _isReceiving;
    uint32_t _lastEdgeTime;
};

// 红外发送类
class IRsend {
public:
    IRsend(uint8_t sendpin);
    void sendNEC(uint32_t data, uint8_t nbits);
private:
    uint8_t _sendpin;
    void mark(uint16_t usec);
    void space(uint16_t usec);
};
#endif

#endif