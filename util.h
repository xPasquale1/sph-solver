#pragma once

#include <chrono>

typedef unsigned char uchar;
typedef unsigned long uint;

struct vec2{
	float x;
	float y;
};

constexpr inline float length(vec2& v1, vec2& v2){
	float dx = v2.x-v1.x;
	float dy = v2.y-v1.y;
	return sqrt(dx*dx+dy*dy);
}

const uint64_t _modulus = static_cast<unsigned long>(pow(2, 32));
const uint64_t _multiplier = 1664525;
const uint64_t _increment = 1013904223;
uint64_t _x;
uint64_t nextrand(){
    _x = (_multiplier * _x + _increment) % _modulus;
    return _x;
}

class Timer{
    using clock = std::chrono::system_clock;
    clock::time_point m_time_point;
    uchar m_avg_idx = 0;
    float m_avg[8] = {0};
public:
    Timer() : m_time_point(clock::now()){}
    ~Timer(){}
    void start(void){
        m_time_point = clock::now();
    }
    float measure_s(void) const {
        const std::chrono::duration<float> difference = clock::now() - m_time_point;
        return difference.count();
    }
    float measure_ms(void) const {
        const std::chrono::duration<float, std::milli> difference = clock::now() - m_time_point;
        return difference.count();
    }
    float average_ms_qstring(void){
        float out = 0;
        const std::chrono::duration<float, std::milli> difference = clock::now() - m_time_point;
        m_avg[m_avg_idx++] = difference.count();
        if(m_avg_idx > 7) m_avg_idx = 0;
        for(uchar i=0; i < 8; ++i){out += *(m_avg+i);}
        return out/8.;
    }
};
