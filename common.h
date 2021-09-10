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
    point4() {}; // �⺻ ������ �߰�
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
        float d = 1; // .mtl �� d�� ���� ������ �����ϹǷ� �׷� ��츦 ���� �ʱⰪ ����
        std::string name;

        MaterialData() {};
};

class Object
{
    public:
        // vertex
        std::vector<unsigned int> vertexPosIndices;
        std::vector<point3> vertexPos;
        std::vector<point3> normal;
        std::vector<point3> textureCoord;
        // color
        std::vector<point3> specular;
        std::vector<point3> ambient;
        std::vector<point4> diffuse;
        // texture
        GLint neighborNum[1024 * 1024] = { 0, }; // �� ���� ���� �̿��� ������ �����ϴ� �ؽ�ó
        GLint neighborIdxList[1024 * 1024] = { 0, }; // �̿��� �ε����� ���������� ����� �ؽ�ó
        GLfloat vertexNormalTex[1024 * 1024] = { 0, }; // �� ���� ��� normal ���� (�ؽ�ó ���ٿ�)
        GLint accumNeighborNum[1024 * 1024] = { 0, }; // ������ �̿��� ����
        GLfloat vertexPosTex[1024 * 1024] = { 0, }; // �̿��� ��ġ�� �����ϱ� ���� ��ġ ���� �ؽ�ó
};