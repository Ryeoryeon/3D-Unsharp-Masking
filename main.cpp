#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include "common.h"
#include <opencv2/opencv.hpp>

const int SCREENSIZE = 1024;
const int TEXTURESIZE = 1024;

static int tripleFace;
std::vector<glm::vec3> lightPos = { glm::vec3(0, 5, 5) , glm::vec3(4, 4, 0), glm::vec3(3, 1, 3), glm::vec3(-2, 5, 3)};

float SCALINGFACTOR = 1.8;
//float SCALINGFACTOR = 0.63f;
static double boundMaxDist;
static point3 boundingCent;

// timer �Լ� ����
float angle; // ȸ�� ����
int lightIdx = 0; // ������ ��ȣ

glm::mat4 Projection;
glm::mat4 View;
glm::mat4 Model;
glm::mat4 mvp;
glm::mat4 transformedMatrix;

GLuint programID;
GLuint VertexBufferID;
GLuint TextureCoordID;
GLuint diffuseColorBufferID;
GLuint ambientColorBufferID;
GLuint specularColorBufferID;
GLuint NormalBufferID;

GLuint vertexBuffer; // vertex buffer Object
GLuint elementBuffer; // element buffer

std::vector<point3> verticesPosition;
static std::vector<unsigned int> vertexPosIndices;
static std::vector<point3> vertexPerAvgNormal; // normals
static std::vector<point3> textureCoord;
static std::vector<point3> specularColors;
static std::vector<point3> ambientColors;
static std::vector<point4> diffuseColors;

// �ٿ�� �ڽ� ũ�� ���� ����
static std::vector<point3> transformedVertices;
static std::vector<point4> boundingBoxCoordinate;

// 3D Unsharp Masking�� ����
static std::vector<std::vector<int>> adjNeighborList; // �� �ؽ��ĸ� ���� ���� ����Ʈ
static GLint neighborNum[TEXTURESIZE * TEXTURESIZE] = {0, }; // �� ���� ���� �̿��� ������ �����ϴ� �ؽ���
static GLint neighborIdxList[TEXTURESIZE * TEXTURESIZE] = {0, }; // �̿��� �ε����� ���������� ����� �ؽ���
static GLfloat vertexNormalTex[TEXTURESIZE * TEXTURESIZE * 3] = {0, }; // �� ���� normal�� ����� ����ǵ��� (�ؽ�ó ���ٿ�)
static GLint accumNeighborNum[TEXTURESIZE * TEXTURESIZE] = {0, }; // ������ �̿��� ����
static GLfloat vertexPosTex[TEXTURESIZE * TEXTURESIZE * 3] = { 0, }; // �̿��� ��ġ�� �����ϱ� ���� ��ġ ���� �ؽ�ó
static std::map <std::pair<int, int>, int> checkOverlap; // �ؽ��Ŀ��� �ߺ��Ǵ� �� ������ ���ϱ� ���� map

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path);
void init();
void transform();
void mydisplay();
void myreshape(int width, int height);
bool loadObjMtl(const char* objName, const char* mtlName, int& faceNum);

double boundingBoxDist(point3& boxCent);
void boundingBox();
double getDist(point3 p1, point3 p2);
void openglToPngSave(int outputIdx);

using namespace std;

void timer(int value)
{
    static int outputIdx = -1; // �� ȭ�� �ǳʶٱ�

    glutPostRedisplay(); // �����츦 �ٽ� �׸����� ��û�ϴ� �Լ�
    glutTimerFunc(45, timer, 0);
    angle += glm::radians(45.0f);

    ++outputIdx;

    if (outputIdx != 0 && outputIdx <= 8)
        openglToPngSave(outputIdx - 1);

    // 360�� ȸ���� ������ ���� �������� ���� (0~11, 12~23..)
    else if (outputIdx != 0 && outputIdx % 8 == 0)
    {
        ++lightIdx;
        outputIdx = 0;

        if (lightIdx == lightPos.size())
            exit(0);
    }
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitWindowSize(SCREENSIZE, SCREENSIZE);
    glutInitWindowPosition(500, 0);
    glutCreateWindow("3D Unsharp Masking");
    glutInitDisplayMode(GLUT_DEPTH | GLUT_SINGLE | GLUT_RGBA);
    glutTimerFunc(0, timer, 0);
    glutDisplayFunc(mydisplay);
    glutReshapeFunc(myreshape);

    GLenum err = glewInit();
    if (err == GLEW_OK)
    {
        init();
        glutMainLoop();
    }
   
    return 0;
}

void init()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    //glClearColor(1.0f, 0.5f, 0.5f, 0.0f);
    // .obj, .mtl �ε� �ڵ�

    //const char* objName = "bunny(withMtl).obj";
    //const char* objName = "bunny(moreFace_withMtl).obj";
    //const char* mtlName = "bunny.mtl";

    //const char* objName = "armadillo.obj";
    //const char* mtlName = "armadillo.mtl";

    //const char* objName = "sphere.obj";
    //const char* mtlName = "sphere.mtl";

    const char* objName = "utahTeapot.obj";
    const char* mtlName = "utahTeapot.mtl";

    int faceNum = 0;
    bool res = loadObjMtl(objName, mtlName, faceNum);
    boundingBox();
    tripleFace = faceNum * 3;

    // �ʱ�ȭ �ڵ�
    /*
    glGenVertexArrays(1, &VAO);
    // 0. vertex array object ���ε�
    glBindVertexArray(VAO);
    */

    // 1. ����Ʈ�� ���ۿ� ����
    // element
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, verticesPosition.size() * sizeof(point3), &verticesPosition[0], GL_STATIC_DRAW);

    // vertex element
    glGenBuffers(1, &elementBuffer); // ���� ����
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexPosIndices.size() * sizeof(unsigned int), &vertexPosIndices[0], GL_STATIC_DRAW);

    // diffuse
    glGenBuffers(1, &diffuseColorBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, diffuseColorBufferID);
    glBufferData(GL_ARRAY_BUFFER, verticesPosition.size() * sizeof(point4), &diffuseColors[0], GL_STATIC_DRAW);

    // ambient
    glGenBuffers(1, &ambientColorBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, ambientColorBufferID);
    glBufferData(GL_ARRAY_BUFFER, verticesPosition.size() * sizeof(point3), &ambientColors[0], GL_STATIC_DRAW);

    // specular
    glGenBuffers(1, &specularColorBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, specularColorBufferID);
    glBufferData(GL_ARRAY_BUFFER, verticesPosition.size() * sizeof(point3), &specularColors[0], GL_STATIC_DRAW);

    // normal (face)
    // 1:1 vertex normal�� �ؽ�ó�� ����
    glGenBuffers(1, &NormalBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, NormalBufferID);
    glBufferData(GL_ARRAY_BUFFER, vertexPerAvgNormal.size() * sizeof(point3), &vertexPerAvgNormal[0], GL_STATIC_DRAW);
    //

    // *** 3D unsharp Masking
    // texture Coordinate
    if (textureCoord.size() != 0)
    {
        glGenBuffers(1, &TextureCoordID);
        glBindBuffer(GL_ARRAY_BUFFER, TextureCoordID);
        glBufferData(GL_ARRAY_BUFFER, textureCoord.size() * sizeof(point3), &textureCoord[0], GL_STATIC_DRAW);
    }

    // Texture
    // (1) : �̿��� ����
    GLuint neighborNumTexID;
    glGenTextures(1, &neighborNumTexID);
    glBindTexture(GL_TEXTURE_2D, neighborNumTexID);

    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, TEXTURESIZE, TEXTURESIZE, 0, GL_RED_INTEGER, GL_INT, neighborNum);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, TEXTURESIZE, TEXTURESIZE, 0, GL_RED, GL_UNSIGNED_BYTE, neighborNum);

    // (2) : �̿��� �ε���
    GLuint neighborNumIdxTexID;
    glGenTextures(1, &neighborNumIdxTexID);
    glBindTexture(GL_TEXTURE_2D, neighborNumIdxTexID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, TEXTURESIZE, TEXTURESIZE, 0, GL_RED_INTEGER, GL_INT, neighborIdxList);

    // (3) : vertex normal
    GLuint vertexPerNormalTexID;
    glGenTextures(1, &vertexPerNormalTexID);
    glBindTexture(GL_TEXTURE_2D, vertexPerNormalTexID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURESIZE, TEXTURESIZE, 0, GL_RGB, GL_FLOAT, vertexNormalTex);

    // (4) : ������ �̿��� ����
    GLuint accumNeighborNumTexID;
    glGenTextures(1, &accumNeighborNumTexID);
    glBindTexture(GL_TEXTURE_2D, accumNeighborNumTexID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, TEXTURESIZE, TEXTURESIZE, 0, GL_RED_INTEGER, GL_INT, accumNeighborNum);

    // (5) : �̿��� ������ ��ġ�� �����ϱ� ���� vertex position [�ӽ�]
    GLuint vertexPerPosTexID;
    glGenTextures(1, &vertexPerPosTexID);
    glBindTexture(GL_TEXTURE_2D, vertexPerPosTexID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURESIZE, TEXTURESIZE, 0, GL_RGB, GL_FLOAT, vertexPosTex);

    // 3. shader program
    programID = LoadShaders("1ringNeiborhood_bilateral.vertexshader", "1ringNeiborhood.fragmentshader");
    //programID = LoadShaders("1ringNeiborhood.vertexshader", "1ringNeiborhood.fragmentshader");

    GLuint texLocation = glGetUniformLocation(programID, "neighborNum");
    GLuint tex2Location = glGetUniformLocation(programID, "neighborIdxList");
    GLuint tex3Location = glGetUniformLocation(programID, "vertexNormalTex");
    GLuint tex4Location = glGetUniformLocation(programID, "accumNeighborNum");
    GLuint tex5Location = glGetUniformLocation(programID, "vertexPosTex"); // �̿��� �� ��ġ ������ ����

    // �ڵ�� �˰� ����
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glUseProgram(programID);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, neighborNumTexID);
    glUniform1i(texLocation, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, neighborNumIdxTexID);
    glUniform1i(tex2Location, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, vertexPerNormalTexID);
    glUniform1i(tex3Location, 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, accumNeighborNumTexID);
    glUniform1i(tex4Location, 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, vertexPerPosTexID);
    glUniform1i(tex5Location, 4);
}

void mydisplay()
{
    transform();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // *** need to fill in this part

    // 2. ������ ����
    // vertex
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    // diffuse
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, diffuseColorBufferID);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    // normal
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, NormalBufferID);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, 0, (void*)0);
    // ambient
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, ambientColorBufferID);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    // specular
    glEnableVertexAttribArray(4);
    glBindBuffer(GL_ARRAY_BUFFER, specularColorBufferID);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    // texture Coordinate
    glEnableVertexAttribArray(5);
    glBindBuffer(GL_ARRAY_BUFFER, TextureCoordID);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    

    glDrawElements(
        GL_TRIANGLES,
        vertexPosIndices.size(), // count
        GL_UNSIGNED_INT,
        (void*)0 // offset
    );

    //glDrawArrays(GL_TRIANGLES, 0, tripleFace);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);
    glDisableVertexAttribArray(5);

    // ***
    //glFlush();
    glutSwapBuffers();

}

void myreshape(int width, int height)
{
    glViewport(0, 0, width, height);

    Projection = glm::perspective(glm::radians(45.0f),
        (float)width / (float)height, 0.1f, 100.0f);

    View = glm::lookAt(
        glm::vec3(0, 2, 8), // Camera in World Space
        glm::vec3(0, 0, 0), // and looks at the origin
        glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    transform();
    //

}

void transform()
{
    Model = glm::mat4(1.0f);
    Model = glm::rotate(Model, angle, glm::vec3(0, 1, 0));
    mvp = Projection * View * Model;

    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
    GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
    glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &View[0][0]);
    GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
    glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model[0][0]);

    // ��ȯ ���
    GLuint TransformedMatrixID = glGetUniformLocation(programID, "Transform");
    glUniformMatrix4fv(TransformedMatrixID, 1, GL_FALSE, &transformedMatrix[0][0]);

    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
    glUniform3f(LightID, lightPos[lightIdx].x, lightPos[lightIdx].y, lightPos[lightIdx].z);
}

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path)
{
    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);

    if (VertexShaderStream.is_open())
    {
        std::stringstream sstr;
        sstr << VertexShaderStream.rdbuf();
        VertexShaderCode = sstr.str();
        VertexShaderStream.close();
    }
    else
    {
        printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
        getchar();
        return 0;
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if (FragmentShaderStream.is_open())
    {
        std::stringstream sstr;
        sstr << FragmentShaderStream.rdbuf();
        FragmentShaderCode = sstr.str();
        FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    printf("Compiling shader : %s\n", vertex_file_path);
    char const* VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);

    if (InfoLogLength > 0)
    {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        printf("%s\n", &VertexShaderErrorMessage[0]);
    }

    // Compile Fragment Shader
    printf("Compiling shader : %s\n", fragment_file_path);
    char const* FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);

    if (InfoLogLength > 0)
    {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        printf("%s\n", &FragmentShaderErrorMessage[0]);
    }

    // Link the program
    printf("Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
    }

    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

double boundingBoxDist(point3& boxCent)
{
    float maxCoordX = std::numeric_limits<float>::min();
    float maxCoordY = std::numeric_limits<float>::min();
    float maxCoordZ = std::numeric_limits<float>::min();

    float minCoordX = std::numeric_limits<float>::max();
    float minCoordY = std::numeric_limits<float>::max();
    float minCoordZ = std::numeric_limits<float>::max();

    int num = 0;

    // �ٿ�� �ڽ��� 8�� ��ǥ�� ���ϱ�
    for (int i = 0; i < verticesPosition.size(); i++)
    {
        point3 temp = verticesPosition[i];

        if (maxCoordX < temp.x)
            maxCoordX = temp.x;

        if (minCoordX > temp.x)
            minCoordX = temp.x;

        if (maxCoordY < temp.y)
            maxCoordY = temp.y;

        if (minCoordY > temp.y)
            minCoordY = temp.y;

        if (maxCoordZ < temp.z)
            maxCoordZ = temp.z;

        if (minCoordZ > temp.z)
            minCoordZ = temp.z;
    }

    // �ٿ�� �ڽ��� �߽��� ��ǥ ���ϱ�
    boxCent.x = (maxCoordX + minCoordX) * 0.5f;
    boxCent.y = (maxCoordY + minCoordY) * 0.5f;
    boxCent.z = (maxCoordZ + minCoordZ) * 0.5f;

    boundingBoxCoordinate.push_back(point4(maxCoordX, maxCoordY, maxCoordZ, 1));

    boundingBoxCoordinate.push_back(point4(maxCoordX, maxCoordY, minCoordZ, 1));
    boundingBoxCoordinate.push_back(point4(maxCoordX, minCoordY, maxCoordZ, 1));
    boundingBoxCoordinate.push_back(point4(minCoordX, maxCoordY, maxCoordZ, 1));

    boundingBoxCoordinate.push_back(point4(maxCoordX, minCoordY, minCoordZ, 1));
    boundingBoxCoordinate.push_back(point4(minCoordX, maxCoordY, minCoordZ, 1));
    boundingBoxCoordinate.push_back(point4(minCoordX, minCoordY, maxCoordZ, 1));

    boundingBoxCoordinate.push_back(point4(minCoordX, minCoordY, minCoordZ, 1));

    // �ٿ�� �ڽ��� �밢�� ����
    // maxCoordX�� �������� ���� ��, ���� minCoordX�ʹ� �ִ�Ÿ��� �������� �ʴ´�
    float maxDist = getDist(point3(maxCoordX, maxCoordY, maxCoordZ), boxCent);

    return maxDist;
}

double getDist(point3 p1, point3 p2)
{
    float xDist = pow(p1.x - p2.x, 2);
    float yDist = pow(p1.y - p2.y, 2);
    float zDist = pow(p1.z - p2.z, 2);

    return sqrt(xDist + yDist + zDist);
}

void boundingBox()
{
    // Bounding Box
    boundMaxDist = boundingBoxDist(boundingCent);

    float scalingSize = SCALINGFACTOR / boundMaxDist;
    glm::mat4 scalingMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scalingSize, scalingSize, scalingSize));
    glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-boundingCent.x, -boundingCent.y, -boundingCent.z));
    //glm::mat4 transformedMatrix = translateMatrix * scalingMatrix;
    transformedMatrix = scalingMatrix * translateMatrix;

    // ��İ� ��ǥ �� ����� GPU����
}

bool loadObjMtl(const char* objName, const char* mtlName, int &faceNum)
{
    // .mtl load
    FILE* fp2;
    fp2 = fopen(mtlName, "r");

    if (fp2 == NULL) {
        printf("Impossible to open the .mtl file !\n");
        return false;
    }

    std::vector<MaterialData> mtlData;
    bool first = true; // ù��° newmtl�ΰ�?
    MaterialData temp;

    while (1)
    {
        char lineHeader[128];

        int res = fscanf(fp2, "%s", lineHeader);

        if (res == EOF)
        {
            break;
        }

        if (strcmp(lineHeader, "newmtl") == 0)
        {
            if (first) // ���� ���� �����Ͱ� ���� ��
            {
                first = false;
                temp = MaterialData(); // temp �ʱ�ȭ
            }

            else
            {
                mtlData.push_back(temp);
                temp = MaterialData();
            }

            continue;

        }

        //                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               std::string lHeader;
        if (strcmp(lineHeader, "Kd") == 0) // diffuse color
        {
            fscanf(fp2, "%f %f %f\n", &temp.Kd.x, &temp.Kd.y, &temp.Kd.z);
        }

        else if (strcmp(lineHeader, "Ka") == 0) // ambient color
        {
            fscanf(fp2, "%f %f %f\n", &temp.Ka.x, &temp.Ka.y, &temp.Ka.z);
        }

        else if (strcmp(lineHeader, "Ks") == 0) // specular color
        {
            fscanf(fp2, "%f %f %f\n", &temp.Ks.x, &temp.Ks.y, &temp.Ks.z);
        }

        else if (strcmp(lineHeader, "d") == 0) // dissolve (transparency)
        {
            fscanf(fp2, "%f\n", &temp.d);
        }

        else if (strcmp(lineHeader, "Ns") == 0) // specular exponent
        {
            fscanf(fp2, "%d\n", &temp.Ns);
        }

        else if (strcmp(lineHeader, "Ke") == 0) // Ke�� �ǳʶٱ�
            continue;

        else if (strcmp(lineHeader, "illum") == 0)
            continue;

        // ������ ���ڿ��� ���, material�� �̸��� �����ϰ� �ִ��� Ȯ���غ���
        else
        {
            //char* originalName;
            char originalName[128];
            strcpy(originalName, lineHeader); //���� ����
            char* nameTemp = strtok(lineHeader, "_");

            if (strcmp(nameTemp, "material") == 0)
                temp.name = originalName;

            else
                continue;
        }
    }

    mtlData.push_back(temp);
    // ��ȿ�� mtlData���� �˻��ϱ�
    int mtlSize = mtlData.size();
    for (int i = 0; i < mtlSize; ++i)
    {
        if (mtlData[i].name == "")
            exit(0);
    }

    // *********** .obj load
    FILE* fp;
    fp = fopen(objName, "r");

    if (fp == NULL) {
        printf("Impossible to open the .obj file !\n");
        return false;
    }

    //std::vector<unsigned int> textureIndices, normalIndices;
    std::vector<unsigned int> vertexIndices;
    std::vector<unsigned int> textureIndices;
    //std::vector<unsigned int> normalIndices;
    std::vector<std::vector<unsigned int>> normalIndices;

    // vertex : normal 1:1 ������ ���� (#average)
    std::map <std::pair<int, int>, int> checkOverlapNormal;
    std::vector<int> vertexPerFace; // �� ������ ��� �븻 ����� ���� �����Ǵ� ���� ����

    //std::vector<point3> verticesPosition;
    std::vector<point3> verticesNormals; // normal�� �� �� ��ü���� ����
    std::vector<point3> verticesTexCoord; // vt�� �� �� ��ü���� ����

    int materialPointer = -1; // �־�� �� material // -1�� �ʱ�ȭ���ִ� ������ ����ó���� ����
    bool faceReadingStart = false;

    while (1)
    {
        char lineHeader[128];
        //char originalName[128];

        int res = fscanf(fp, "%s", lineHeader);
        if (res == EOF)
            break;
        //strcpy(originalName, lineHeader); // ���� ����

        // ù �ܾ v�� ���, vertex�� �д´�
        if (strcmp(lineHeader, "v") == 0)
        {
            point3 vertex;
            fscanf(fp, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
            verticesPosition.push_back(vertex);
        }

        if (strcmp(lineHeader, "usemtl") == 0)
        {
            int res = fscanf(fp, "%s", lineHeader); // ���� �� �� �� �б�
            // � material�� �ο����� �˻�
            char nameTemp[128];
            strcpy(nameTemp, lineHeader);
            std::string compareName = std::string(nameTemp);
            if (compareName == "material_0_24") {
                puts("");
            }
            for (int i = 0; i < mtlData.size(); i++)
            {
                if (compareName == mtlData[i].name)
                {
                    materialPointer = i;
                    break;
                }
            }
        }

        else if (strcmp(lineHeader, "vt") == 0)
        {
            point3 texCoord;
            fscanf(fp, "%f %f %f\n", &texCoord.x, &texCoord.y, &texCoord.z);
            verticesTexCoord.push_back(texCoord);
        }

        // ù �ܾ vn�̶��, normal�� �д´�
        else if (strcmp(lineHeader, "vn") == 0)
        {
            point3 normal;
            fscanf(fp, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            verticesNormals.push_back(normal);
        }

        // ù �ܾ f���, face�� �д´�
        else if (strcmp(lineHeader, "f") == 0)
        {
            unsigned int vertexPosIndex[3], normalIndex[3], texCoordIndex[3];
            std::vector<int> tempFaceList; // face table�� ����� ������ �ӽ÷� �����ϴ� ����

            char str[128];
            fgets(str, sizeof(str), fp);
            str[strlen(str) - 1] = '\0';
            char* ptr = strtok(str, " //");
            int ptrSize = 0;

            while (ptr != NULL) // �ڸ� ���ڿ��� ������ ���� ������ ���
            {
                tempFaceList.push_back(atoi(ptr));
                ptr = strtok(NULL, " //");
                ++ptrSize;
            }

            mtlData[materialPointer].Kd.w = mtlData[materialPointer].d;

            if (!faceReadingStart) // ó������ face�� �о�� ��, ���� ����Ʈ�� ���� �迭 ũ�� �ʱ�ȭ
            {
                int vertexPosNum = verticesPosition.size();
                adjNeighborList.resize(vertexPosNum);
                diffuseColors.resize(vertexPosNum);
                ambientColors.resize(vertexPosNum);
                specularColors.resize(vertexPosNum);
                //normalIndices.resize(vertexPosNum); // ���� vn index ����

                vertexPerAvgNormal.assign(vertexPosNum, { 0, 0, 0 }); // face�� avg normal ����
                vertexPerFace.assign(vertexPosNum, 0);
                faceReadingStart = true;
            }

            // f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ������ �����
            if (ptrSize == 9)
            {
                int temp1 = 0, temp2 = 0, temp3 = 0;

                for (int i = 0; i < 9; i++)
                {
                    if (i % 3 == 0)
                        vertexPosIndex[temp1++] = tempFaceList[i] - 1;

                    if (i % 3 == 1)
                        texCoordIndex[temp2++] = tempFaceList[i] - 1;

                    else if (i % 3 == 2)
                        normalIndex[temp3++] = tempFaceList[i] - 1;
                }
            }

            // f v1//vn1 v2//vn2 v3//vn3 ������ �����
            else if (ptrSize == 6)
            {
                int temp1 = 0;
                int temp2 = 0;
                int temp3 = 0;

                for (int i = 0; i < 6; i++)
                {
                    if (i % 2 == 0)
                        vertexPosIndex[temp1++] = tempFaceList[i] - 1;

                    else
                        normalIndex[temp2++] = tempFaceList[i] - 1;
                }

                // texture ��ǥ�� �������� �ʴ� face�� ���, (���� �������� �� �ִ�)
                // -> ��ȿ���� �ʵ��� ��� 1��°(����) �ε����� ����
                texCoordIndex[temp3++] = 1;
                texCoordIndex[temp3++] = 1;
                texCoordIndex[temp3++] = 1;
            }

            // f v1// v2// f3// ������ ����Ǵ� ����
            else if (ptrSize == 3)
            {
                vertexPosIndex[0] = tempFaceList[0] - 1;
                vertexPosIndex[1] = tempFaceList[1] - 1;
                vertexPosIndex[2] = tempFaceList[2] - 1;

                // texture ��ǥ�� �������� �ʴ� face�� ���, (���� �������� �� �ִ�)
                // -> ��ȿ���� �ʵ��� ��� 1��°(����) �ε����� ����
                texCoordIndex[0] = 1;
                texCoordIndex[1] = 1;
                texCoordIndex[2] = 1;
            }

            else
            {
                std::cout << "This File can't be read by our simple parser : Try exporting with other options\n";
                return false;
            }

            // face normal vector ���
            glm::vec3 forFaceNormalVec1;
            glm::vec3 forFaceNormalVec2;

            forFaceNormalVec1.x = verticesPosition[vertexPosIndex[1]].x - verticesPosition[vertexPosIndex[0]].x;
            forFaceNormalVec1.y = verticesPosition[vertexPosIndex[1]].y - verticesPosition[vertexPosIndex[0]].y;
            forFaceNormalVec1.z = verticesPosition[vertexPosIndex[1]].z - verticesPosition[vertexPosIndex[0]].z;

            forFaceNormalVec2.x = verticesPosition[vertexPosIndex[2]].x - verticesPosition[vertexPosIndex[0]].x;
            forFaceNormalVec2.y = verticesPosition[vertexPosIndex[2]].y - verticesPosition[vertexPosIndex[0]].y;
            forFaceNormalVec2.z = verticesPosition[vertexPosIndex[2]].z - verticesPosition[vertexPosIndex[0]].z;
            
            glm::vec3 faceNormal = glm::cross(forFaceNormalVec1, forFaceNormalVec2);
            faceNormal /= glm::length(faceNormal); // �߿�!

            for (int i = 0; i < 3; ++i)
            {
                // vertex position indexing
                vertexPosIndices.push_back(vertexPosIndex[i]);

                // ---> vertex : vertex normal vector 1:1 ������ ���� ���� �κ� 
                vertexPerAvgNormal[vertexPosIndex[i]].x += faceNormal.x;
                vertexPerAvgNormal[vertexPosIndex[i]].y += faceNormal.y;
                vertexPerAvgNormal[vertexPosIndex[i]].z += faceNormal.z;

                // �� ������ ���� color
                diffuseColors[vertexPosIndex[i]] = mtlData[materialPointer].Kd;
                ambientColors[vertexPosIndex[i]] = mtlData[materialPointer].Ka;
                specularColors[vertexPosIndex[i]] = mtlData[materialPointer].Ks;

                // ȣ��Ǵ� ���� ���� ���� ���� ���� (��� �۾� ����)
                ++vertexPerFace[vertexPosIndex[i]];
            }

            // 1-ring neighborhood�� ���� ���� ����Ʈ ����
            int tempVer0 = vertexPosIndex[0], tempVer1 = vertexPosIndex[1], tempVer2 = vertexPosIndex[2];

            // ���� üũ���� ���� ���� ������ push_back();
            if (checkOverlap[{tempVer0, tempVer1}] == 0 && checkOverlap[{tempVer1, tempVer0}] == 0)
            {
                checkOverlap[{tempVer0, tempVer1}] = 1;
                checkOverlap[{tempVer1, tempVer0}] = 1;
                adjNeighborList[vertexPosIndex[0]].push_back(vertexPosIndex[1]);
                adjNeighborList[vertexPosIndex[1]].push_back(vertexPosIndex[0]);
            }

            if (checkOverlap[{tempVer0, tempVer2}] == 0 && checkOverlap[{tempVer2, tempVer0}] == 0)
            {
                checkOverlap[{tempVer0, tempVer2}] = 1;
                checkOverlap[{tempVer2, tempVer0}] = 1;
                adjNeighborList[vertexPosIndex[0]].push_back(vertexPosIndex[2]);
                adjNeighborList[vertexPosIndex[2]].push_back(vertexPosIndex[0]);
            }

            if (checkOverlap[{tempVer1, tempVer2}] == 0 && checkOverlap[{tempVer2, tempVer1}] == 0)
            {
                checkOverlap[{tempVer1, tempVer2}] = 1;
                checkOverlap[{tempVer2, tempVer1}] = 1;
                adjNeighborList[vertexPosIndex[1]].push_back(vertexPosIndex[2]);
                adjNeighborList[vertexPosIndex[2]].push_back(vertexPosIndex[1]);
            }

            ++faceNum;
        }

        // ù �ܾ l�� ��

        //
    }

    // �ε��� ����
    // �� �ﰢ���� �������� ��� ��ȸ

    /*
    for (int i = 0; i < vertexPosIndices.size(); i++)
    {
        //unsigned int vertexIdx = vertexPosIndices[i];
        unsigned int normalIdx = normalIndices[i];
   
        //point3 vertex = verticesPosition[vertexIdx - 1];
        point3 normal = verticesNormals[normalIdx];

        //vertices.push_back(vertex);
        normals.push_back(normal);

        // �ؽ��� ��ǥ �ε���
        if (verticesTexCoord.size() != 0) // �ؽ�ó ��ǥ�� �ִ� ���� ���
        {
            unsigned int textureCoordIdx = textureIndices[i];
            point3 texture = verticesTexCoord[textureCoordIdx];
            textureCoord.push_back(texture);
        }
    }
    */

    // 3D Unsharp Masking�� ���� �ؽ���
    int vertexPosNum = verticesPosition.size();
    //neighborNum.assign(verticesNum, 0);
    int neighborSize;
    int tempNeighborIdx = 0;
    int tempNormalIdx = 0;

    for (int i = 0; i < vertexPosNum; ++i)
    {
        neighborSize = adjNeighborList[i].size();
        neighborNum[i] = neighborSize;
        if (i == 1)
            accumNeighborNum[i] = neighborNum[i-1];

        accumNeighborNum[i] = accumNeighborNum[i-1] + neighborNum[i-1];

        for (int j = 0; j < neighborSize; ++j)
        {
            neighborIdxList[tempNeighborIdx] = adjNeighborList[i][j];
            ++tempNeighborIdx;
        }

        // normal ��� ���ֱ�
        vertexPerAvgNormal[i].x /= vertexPerFace[i];
        vertexPerAvgNormal[i].y /= vertexPerFace[i];
        vertexPerAvgNormal[i].z /= vertexPerFace[i];

        // ������ 1 ����ȭ
        float normalSize = sqrt(pow(vertexPerAvgNormal[i].x, 2) + pow(vertexPerAvgNormal[i].y, 2) + pow(vertexPerAvgNormal[i].z, 2));
        vertexPerAvgNormal[i].x /= normalSize;
        vertexPerAvgNormal[i].y /= normalSize;
        vertexPerAvgNormal[i].z /= normalSize;

        // texture ����
        vertexNormalTex[tempNormalIdx] = vertexPerAvgNormal[i].x;
        ++tempNormalIdx;
        vertexNormalTex[tempNormalIdx] = vertexPerAvgNormal[i].y;
        ++tempNormalIdx;
        vertexNormalTex[tempNormalIdx] = vertexPerAvgNormal[i].z;
        ++tempNormalIdx;
    }

    fclose(fp);
    fclose(fp2);

    return true;
}

void openglToPngSave(int outputIdx)
{
    static int fileNo = 0;
    int bitsNum;
    GLubyte* bits; //RGB bits
    GLint captureImage[4]; //current viewport

    //get current viewport
    glGetIntegerv(GL_VIEWPORT, captureImage); // �̹��� ũ�� �˾Ƴ���

    int rows = captureImage[3];
    int cols = captureImage[2];

    bitsNum = 3 * cols * rows;
    bits = new GLubyte[bitsNum]; // opengl���� �о���� ��Ʈ

    //read pixel from frame buffer
    glFinish(); //finish all commands of OpenGL

    glPixelStorei(GL_PACK_ALIGNMENT, 1); //or glPixelStorei(GL_PACK_ALIGNMENT,4);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glReadPixels(0, 0, cols, rows, GL_BGR_EXT, GL_UNSIGNED_BYTE, bits);

    cv::Mat outputImage(rows, cols, CV_8UC3);
    int currentIdx;

    for (int i = 0; i < outputImage.rows; i++)
    {
        for (int j = 0; j < outputImage.cols; j++)
        {
            // stores image from top to bottom, left to right
            currentIdx = (rows - i - 1) * 3 * cols + j * 3; // +0

            outputImage.at<cv::Vec3b>(i, j)[0] = (uchar)(bits[currentIdx]);
            outputImage.at<cv::Vec3b>(i, j)[1] = (uchar)(bits[++currentIdx]); // +1
            outputImage.at<cv::Vec3b>(i, j)[2] = (uchar)(bits[++currentIdx]); // +2
        }
    }

    char filename[100];
    //sprintf(filename, "%c_output_%d_%d.bmp", RENDERMODE, idx, lightIdx);
    sprintf(filename, "output[%i]_%d.png", lightIdx, fileNo++);
    imwrite(filename, outputImage);


    delete[] bits;
}