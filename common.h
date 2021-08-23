#pragma once

struct point3
{
    point3(float a, float b, float c) : x(a), y(b), z(c) {};
    point3() {};
    float x;
    float y;
    float z;
};


struct point2
{
    point2(float a, float b) : x(a), y(b) {};
    point2() {};
    float x;
    float y;
    float z;
};

struct point4
{
    point4(float a, float b, float c) : x(a), y(b), z(c), w(1) {};
    point4(float a, float b, float c, float d) : x(a), y(b), z(c), w(d) {};
    point4() {}; // 기본 생성자 추가
    float x;
    float y;
    float z;
    float w;
};

class MaterialData
{
    public:
        point4 Kd; // diffuse
        point3 Ka; // ambient
        point3 Ks; // specular
        int Ns; // specular exponent
        float d = 1; // .mtl 중 d가 없는 파일이 존재하므로 그럴 경우를 위해 초기값 지정
        std::string name;

        MaterialData() {};
};