#ifndef COMMON_H
#define COMMON_H
#pragma once

#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <map>
#include <iostream>

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

class Object
{
    private:
        GLuint diffuseID, ambientID, specularID, NormalID, vertexBuffer, elementBuffer;
        // texture
        GLuint neighborNumTexID, neighborNumIdxTexID, accumNeighborNumTexID;
        GLuint texLocation, tex2Location, tex3Location;
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
        GLint neighborNum[1024 * 1024] = { 0, }; // 각 점에 대한 이웃의 개수를 저장하는 텍스처
        GLint neighborIdxList[1024 * 1024] = { 0, }; // 이웃의 인덱스가 연속적으로 저장된 텍스처
        GLint accumNeighborNum[1024 * 1024] = { 0, }; // 누적된 이웃의 개수

    public:
        //functions
        int initResource(GLuint shaderProgram);
        void draw(GLint uniformMvp, glm::mat4 mvp, const int TEXTURESIZE);
        void disable();
        void deleteBuffers();
        bool loadObjMtl(const char* objName, const char* mtlName);
        glm::mat4 boundingBox(const point3 destinationCent, const float scalingFactor);
};

#endif