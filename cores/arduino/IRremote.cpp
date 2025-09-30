#include "IRremote.h"

// ----------- IRrecv 实现 -----------
IRrecv::IRrecv(uint8_t recvpin)
    : _recvpin(recvpin), _lastLevel(1), _lastTime(0), _rawlen(0), _isReceiving(false), _lastEdgeTime(0)
{
}

void IRrecv::enableIRIn()
{
    pinMode(_recvpin, INPUT);
    _lastLevel = digitalRead(_recvpin);
    _lastTime = time_get();
    _rawlen = 0;
    _isReceiving = false;
}

void IRrecv::IRpoll()
{
    uint8_t level = digitalRead(_recvpin);
    uint32_t now = time_get();

    if (level != _lastLevel)
    {
        uint32_t duration = now - _lastTime;
        _lastTime = now;

        if (_rawlen < 100)
        {
            _rawbuf[_rawlen++] = duration;
        }

        // 检测到引导码，开始接收
        if (!_isReceiving && level == 0 && duration > 8000 && duration < 10000)
        {
            _isReceiving = true;
            _rawlen = 1;
            _rawbuf[0] = duration;
        }

        // 检测到帧结束（长时间无变化）
        if (_isReceiving && duration > 20000) // 优化：20ms
        {
            _isReceiving = false;
        }

        _lastLevel = level;
        _lastEdgeTime = now;
    }

    // 超时自动复位
    if ((now - _lastEdgeTime) > 20000) // 优化：20ms
    {
        resume();
        _isReceiving = false;
    }
}

bool IRrecv::decode(decode_results *results)
{
    int start = -1;
    // 1. 查找引导头
    for (int i = 0; i < _rawlen - 1; i++) {
        if (_rawbuf[i] > 8000 && _rawbuf[i] < 10000 &&
            _rawbuf[i+1] > 3500 && _rawbuf[i+1] < 5500) {
            // 找到引导头
            if (_rawlen - i >= 2 + 2 * NEC_BITS) {
                start = i;
                // 可break，也可继续查找最后一组
            }
        }
    }
    if (start == -1) return false; // 没找到合格引导头

    // 2. NEC解码
    uint32_t data = 0;
    int dataOffset = start + 2;
    for (uint8_t i = 0; i < NEC_BITS; i++) {
        uint32_t mark = _rawbuf[dataOffset++];
        uint32_t space = _rawbuf[dataOffset++];
        if (mark < 300 || mark > 800) return false;
        if (space > 1100 && space < 2200) {
            data = (data << 1) | 1;
        } else if (space > 200 && space < 1100) {
            data = (data << 1);
        } else {
            return false;
        }
    }
    results->value = data;
    results->bits = NEC_BITS;
    results->decode_type = 1; // NEC
    return true;
}

void IRrecv::resume()
{
    // Serial.println("resume called");
    _rawlen = 0;
    _isReceiving = false;
    _lastLevel = digitalRead(_recvpin);
    _lastTime = time_get();
    _lastEdgeTime = time_get();          // 重置最后边沿时间
    memset(_rawbuf, 0, sizeof(_rawbuf)); // 清空缓冲区
}

// ----------- IRsend 实现 -----------
IRsend::IRsend(uint8_t sendpin) : _sendpin(sendpin)
{
    pinMode(_sendpin, OUTPUT);
    digitalWrite(_sendpin, LOW);
}

void IRsend::mark(uint16_t usec)
{
    // 38kHz方波，软件实现
    uint32_t t = micros();
    while ((micros() - t) < usec)
    {
        digitalWrite(_sendpin, HIGH);
        delayMicroseconds(13); // 13us高
        digitalWrite(_sendpin, LOW);
        delayMicroseconds(13); // 13us低，约38kHz
    }
}

void IRsend::space(uint16_t usec)
{
    digitalWrite(_sendpin, LOW);
    delayMicroseconds(usec);
}

void IRsend::sendNEC(uint32_t data, uint8_t nbits)
{
    // 引导码
    mark(NEC_HDR_MARK);
    space(NEC_HDR_SPACE);
    // 数据
    for (int8_t i = nbits - 1; i >= 0; i--)
    {
        mark(NEC_BIT_MARK);
        if (data & (1UL << i))
        {
            space(NEC_ONE_SPACE);
        }
        else
        {
            space(NEC_ZERO_SPACE);
        }
    }
    // 结束
    mark(NEC_BIT_MARK);
    space(0);
}
