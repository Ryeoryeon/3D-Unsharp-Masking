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

const int SCREENSIZE = 512;
const int TEXTURESIZE = 32;

static int tripleFace;
glm::vec3 lightPos = glm::vec3(3, 2, 1);

float SCALINGFACTOR = 1.4;
static double boundMaxDist;
static point3 boundingCent;

glm::mat4 Projection;
glm::mat4 View;
glm::mat4 Model;
glm::mat4 mvp;

GLuint programID;
GLuint VertexBufferID;
GLuint TextureCoordID;
GLuint diffuseColorBufferID;
GLuint ambientColorBufferID;
GLuint specularColorBufferID;
GLuint NormalBufferID;
GLuint NeighborNumID;
GLuint NeighborIdxID;

GLuint VBO; // vertex buffer Object
GLuint VAO; // vertex array Object

static std::vector<point3> vertices;
static std::vector<point3> normals;
static std::vector<point3> textureCoord;
static std::vector<point3> specularColors;
static std::vector<point3> ambientColors;
static std::vector<point4> diffuseColors;

// �ٿ�� �ڽ� ũ�� ���� ����
static std::vector<point3> transformedVertices;
static std::vector<point4> boundingBoxCoordinate;

// 3D Unsharp Masking�� ����
static std::vector<std::vector<int>> adjNeighborList; // �� �ؽ��ĸ� ���� ���� ����Ʈ
static GLubyte neighborNum[TEXTURESIZE * TEXTURESIZE] = {0, }; // �� ���� ���� �̿��� ������ �����ϴ� �ؽ���
static GLuint neighborIdxList[TEXTURESIZE * TEXTURESIZE] = {0, }; // �̿��� �ε����� ���������� ����� �ؽ���
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

using namespace std;

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitWindowSize(SCREENSIZE, SCREENSIZE);
    glutInitWindowPosition(500, 0);
    glutCreateWindow("3D Unsharp Masking");
    glutInitDisplayMode(GLUT_DEPTH | GLUT_SINGLE | GLUT_RGBA);
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
    // .obj, .mtl �ε� �ڵ�
    const char* objName = "model_normalized.obj";
    const char* mtlName = "model_normalized.mtl";   

    //const char* objName = "bunny(withMtl).obj";
    //const char* mtlName = "bunny.mtl";
    //memset(neighborNum, 255, sizeof(neighborNum));

    int faceNum = 0;
    bool res = loadObjMtl(objName, mtlName, faceNum);
    boundingBox();
    tripleFace = faceNum * 3;

    // �ʱ�ȭ �ڵ�
    glGenVertexArrays(1, &VAO);
    // 0. vertex array object ���ε�
    glBindVertexArray(VAO);

    // 1. ����Ʈ�� ���ۿ� ����
    // vertex
    glGenBuffers(1, &VBO); // ���� ����
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, (transformedVertices.size() * sizeof(point3)), &transformedVertices[0], GL_STATIC_DRAW);

    // diffuse
    glGenBuffers(1, &diffuseColorBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, diffuseColorBufferID);
    glBufferData(GL_ARRAY_BUFFER, (tripleFace * sizeof(point4)), &diffuseColors[0], GL_STATIC_DRAW);

    // ambient
    glGenBuffers(1, &ambientColorBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, ambientColorBufferID);
    glBufferData(GL_ARRAY_BUFFER, (tripleFace * sizeof(point3)), &ambientColors[0], GL_STATIC_DRAW);

    // specular
    glGenBuffers(1, &specularColorBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, specularColorBufferID);
    glBufferData(GL_ARRAY_BUFFER, (tripleFace * sizeof(point3)), &specularColors[0], GL_STATIC_DRAW);

    // normal
    glGenBuffers(1, &NormalBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, NormalBufferID);
    glBufferData(GL_ARRAY_BUFFER, (normals.size() * sizeof(point3)), &normals[0], GL_STATIC_DRAW);
    //
    // *** 3D unsharp Masking
    // texture Coordinate
    glGenBuffers(1, &TextureCoordID);
    glBindBuffer(GL_ARRAY_BUFFER, TextureCoordID);
    glBufferData(GL_ARRAY_BUFFER, (textureCoord.size() * sizeof(point3)), &textureCoord[0], GL_STATIC_DRAW);

    // Texture
    GLuint neighborNumTexID;
    glGenTextures(1, &neighborNumTexID);
    glBindTexture(GL_TEXTURE_2D, neighborNumTexID);

    GLuint neighborNumIdxID;
    glGenTextures(1, &neighborNumIdxID);
    glBindTexture(GL_TEXTURE_2D, neighborNumIdxID);

    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //memset(neighborNum, 255, sizeof(neighborNum));
    //neighborNum[0] = 50;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, TEXTURESIZE, TEXTURESIZE, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, neighborNum);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, TEXTURESIZE, TEXTURESIZE, 0, GL_LUMINANCE, GL_UNSIGNED_INT, neighborIdxList);

    // 3. shader program
    //programID = LoadShaders("vshader.vertexshader", "fshader.fragmentshader");
    programID = LoadShaders("texture.vertexshader", "texture.fragmentshader");
    glUniform1i(glGetUniformLocation(programID, "neighborNum"), 0);
    glUniform1i(glGetUniformLocation(programID, "neighborIdxList"), 1);

    // �ڵ�� �˰� ����
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glUseProgram(programID);

}

void mydisplay()
{
    transform();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // *** need to fill in this part

    // 2. ������ ����
    // vertex
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
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

    glDrawArrays(GL_TRIANGLES, 0, tripleFace);
    glDisableVertexAttribArray(0);

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
        glm::vec3(3, 4, 3), // Camera in World Space
        //glm::vec3(3, 4, 3), // Camera in World Space
        glm::vec3(0, 0, 0), // and looks at the origin
        glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    transform();
    //

}

void transform()
{
    Model = glm::mat4(1.0f);
    mvp = Projection * View * Model;

    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
    GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
    glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &View[0][0]);
    GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
    glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model[0][0]);

    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
    glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

    /*
    GLuint NeighborNumSizeID = glGetUniformLocation(programID, "NeighborNumSize");
    glUniform1i(NeighborNumSizeID, neighborNum.size());

    GLuint NeighborIdxSizeID = glGetUniformLocation(programID, "NeighborIdxListSize");
    glUniform1i(NeighborIdxSizeID, neighborNum.size());
    */

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
    for (int i = 0; i < vertices.size(); i++)
    {
        point3 temp = vertices[i];

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
    glm::mat4 transformedMatrix = translateMatrix * scalingMatrix;

    // homogeneous coordinate
    for (int i = 0; i < vertices.size(); ++i)
    {
        point4 temp_homo = point4(vertices[i].x, vertices[i].y, vertices[i].z); // (x,y.z.1 ��ȯ)
        float temp_output_homo[4] = { 0, };

        for (int j = 0; j < 3; ++j)
        {
            // ����!
            // glm::mat4���� ������غ��� [0]~[3]�� ���� �ش��ϴ� ������ ��� �־���. (x,y,z,w�� �࿡ �ش��ϴ� ����)
            if (j == 0)
            {
                temp_output_homo[j] += transformedMatrix[0].x * temp_homo.x;
                temp_output_homo[j] += transformedMatrix[1].x * temp_homo.y;
                temp_output_homo[j] += transformedMatrix[2].x * temp_homo.z;
                temp_output_homo[j] += transformedMatrix[3].x * temp_homo.w;
            }

            else if (j == 1)
            {
                temp_output_homo[j] += transformedMatrix[0].y * temp_homo.x;
                temp_output_homo[j] += transformedMatrix[1].y * temp_homo.y;
                temp_output_homo[j] += transformedMatrix[2].y * temp_homo.z;
                temp_output_homo[j] += transformedMatrix[3].y * temp_homo.w;
            }

            else  // j == 2
            {
                temp_output_homo[j] += transformedMatrix[0].z * temp_homo.x;
                temp_output_homo[j] += transformedMatrix[1].z * temp_homo.y;
                temp_output_homo[j] += transformedMatrix[2].z * temp_homo.z;
                temp_output_homo[j] += transformedMatrix[3].z * temp_homo.w;
            }
        }

        point3 output_homo = point3(temp_output_homo[0], temp_output_homo[1], temp_output_homo[2]);
        transformedVertices.push_back(output_homo);
    }
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

        else if (strcmp(lineHeader, "illum") == 0) // Ke�� �ǳʶٱ�
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

    // .obj load
    FILE* fp;
    fp = fopen(objName, "r");

    if (fp == NULL) {
        printf("Impossible to open the .obj file !\n");
        return false;
    }

    std::vector<unsigned int> vertexIndices, textureIndices, normalIndices;

    std::vector<point3> tempVertices;
    std::vector<point3> tempNormals;
    std::vector<point3> tempTextureCoord;

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
            tempVertices.push_back(vertex);
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
            tempTextureCoord.push_back(texCoord);
        }

        // ù �ܾ vn�̶��, normal�� �д´�
        else if (strcmp(lineHeader, "vn") == 0)
        {
            point3 normal;
            fscanf(fp, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            tempNormals.push_back(normal);
        }

        // ù �ܾ f���, face�� �д´�
        else if (strcmp(lineHeader, "f") == 0)
        {
            unsigned int vertexIndex[3], normalIndex[3], textureCoordIndex[3];
            std::vector<int> tempFaceList; // face table�� ����� ������ �ӽ÷� �����ϴ� ����

            char str[128];
            fgets(str, sizeof(str), fp);
            char* ptr = strtok(str, " //");
            int ptrSize = 0;

            while (ptr != NULL) // �ڸ� ���ڿ��� ������ ���� ������ ���
            {
                tempFaceList.push_back(atoi(ptr));
                ptr = strtok(NULL, " //");
                ++ptrSize;
            }
            
            mtlData[materialPointer].Kd.w = mtlData[materialPointer].d;

            // f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ������ �����
            if (ptrSize == 9)
            {
                int temp1 = 0, temp2 = 0, temp3 = 0;

                for (int i = 0; i < 9; i++)
                {
                    if (i % 3 == 0)
                        vertexIndex[temp1++] = tempFaceList[i];

                    if (i % 3 == 1)
                        textureCoordIndex[temp2++] = tempFaceList[i];
                    
                    else if (i % 3 == 2)
                        normalIndex[temp3++] = tempFaceList[i];
                }

                // �� ������ ���� color
                diffuseColors.push_back(mtlData[materialPointer].Kd);
                diffuseColors.push_back(mtlData[materialPointer].Kd);
                diffuseColors.push_back(mtlData[materialPointer].Kd);

                ambientColors.push_back(mtlData[materialPointer].Ka);
                ambientColors.push_back(mtlData[materialPointer].Ka);
                ambientColors.push_back(mtlData[materialPointer].Ka);

                specularColors.push_back(mtlData[materialPointer].Ks);
                specularColors.push_back(mtlData[materialPointer].Ks);
                specularColors.push_back(mtlData[materialPointer].Ks);

                // index�� 9���� ���� ���� ����Ǿ�� �ϹǷ� ��ġ�� ���⿡!
                /*
                textureIndices.push_back(textureCoordIndex[0]);
                textureIndices.push_back(textureCoordIndex[1]);
                textureIndices.push_back(textureCoordIndex[2]);
                */
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
                        vertexIndex[temp1++] = tempFaceList[i];

                    else
                        normalIndex[temp2++] = tempFaceList[i];
                }

                // �� �� ���� ���� color

                diffuseColors.push_back(mtlData[materialPointer].Kd);
                diffuseColors.push_back(mtlData[materialPointer].Kd);
                diffuseColors.push_back(mtlData[materialPointer].Kd);

                ambientColors.push_back(mtlData[materialPointer].Ka);
                ambientColors.push_back(mtlData[materialPointer].Ka);
                ambientColors.push_back(mtlData[materialPointer].Ka);

                specularColors.push_back(mtlData[materialPointer].Ks);
                specularColors.push_back(mtlData[materialPointer].Ks);
                specularColors.push_back(mtlData[materialPointer].Ks);

                // texture ��ǥ�� �������� �ʴ� face�� ���, (0,0,0)���� ����
                textureCoordIndex[temp3++] = 1;
                textureCoordIndex[temp3++] = 1;
                textureCoordIndex[temp3++] = 1;
            }

            else
            {
                std::cout << "This File can't be read by our simple parser : Try exporting with other options\n";
                return false;
            }

            vertexIndices.push_back(vertexIndex[0]);
            vertexIndices.push_back(vertexIndex[1]);
            vertexIndices.push_back(vertexIndex[2]);

            normalIndices.push_back(normalIndex[0]);
            normalIndices.push_back(normalIndex[1]);
            normalIndices.push_back(normalIndex[2]);

            textureIndices.push_back(textureCoordIndex[0]);
            textureIndices.push_back(textureCoordIndex[1]);
            textureIndices.push_back(textureCoordIndex[2]);

            if (!faceReadingStart) // ó������ face�� �о�� ��, ���� ����Ʈ ũ�� �ʱ�ȭ
                adjNeighborList.resize(tempVertices.size() + 1);

            // 1-ring neighborhood�� ���� ���� ����Ʈ ����
            int tempVer0 = vertexIndex[0], tempVer1 = vertexIndex[1], tempVer2 = vertexIndex[2];
            // ���� üũ���� ���� ���� ������ push_back();
            if (checkOverlap[{tempVer0, tempVer1}] == 0 && checkOverlap[{tempVer1, tempVer0}] == 0)
            {
                checkOverlap[{tempVer0, tempVer1}] = 1;
                checkOverlap[{tempVer1, tempVer0}] = 1;
                adjNeighborList[vertexIndex[0]].push_back(vertexIndex[1]);
                adjNeighborList[vertexIndex[1]].push_back(vertexIndex[0]);
            }

            if (checkOverlap[{tempVer0, tempVer2}] == 0 && checkOverlap[{tempVer2, tempVer0}] == 0)
            {
                checkOverlap[{tempVer0, tempVer2}] = 1;
                checkOverlap[{tempVer2, tempVer0}] = 1;
                adjNeighborList[vertexIndex[0]].push_back(vertexIndex[2]);
                adjNeighborList[vertexIndex[2]].push_back(vertexIndex[0]);
            }

            if (checkOverlap[{tempVer1, tempVer2}] == 0 && checkOverlap[{tempVer2, tempVer1}] == 0)
            {
                checkOverlap[{tempVer1, tempVer2}] = 1;
                checkOverlap[{tempVer2, tempVer1}] = 1;
                adjNeighborList[vertexIndex[1]].push_back(vertexIndex[2]);
                adjNeighborList[vertexIndex[2]].push_back(vertexIndex[1]);
            }

            ++faceNum;
        }

        // ù �ܾ l�� ��

        //
    }

    // �ε��� ����
    // �� �ﰢ���� �������� ��� ��ȸ
    for (int i = 0; i < vertexIndices.size(); i++)
    {
        unsigned int vertexIdx = vertexIndices[i];
        unsigned int normalIdx = normalIndices[i];
        // obj�� 1���� ����������, C++�� index�� 0���� �����ϱ� ������
        point3 vertex = tempVertices[vertexIdx - 1];
        point3 normal = tempNormals[normalIdx - 1];

        vertices.push_back(vertex);
        normals.push_back(normal);

        // �ؽ��� ��ǥ �ε���
        unsigned int textureCoordIdx = textureIndices[i];
        point3 texture = tempTextureCoord[textureCoordIdx - 1];
        textureCoord.push_back(texture);
    }

    // �ؽ��� ��ǥ �ε���
    /*
    for (int i = 0; i < textureIndices.size(); i++)
    {
        unsigned int textureCoordIdx = textureIndices[i];
        point3 texture = tempTextureCoord[textureCoordIdx - 1];
        textureCoord.push_back(texture);
    }
    */

    //textureCoord = tempTextureCoord;


    // 3D Unsharp Masking�� ���� �ؽ���
    int verticesNum = tempVertices.size() + 1; // ���� 0������ �����ϹǷ� �ӽ� +1
    //neighborNum.assign(verticesNum, 0);
    int neighborSize;
    int tempNextIdx = 0;

    for (int i = 0; i < verticesNum; ++i)
    {
        neighborSize = adjNeighborList[i].size();
        neighborNum[i] = neighborSize * 10;

        for (int j = 0; j < neighborSize; ++j)
        {
            neighborIdxList[tempNextIdx] = adjNeighborList[i][j];
            ++tempNextIdx;
        }
            //neighborIdxList.push_back(adjNeighborList[i][j]);
    }

    fclose(fp);
    fclose(fp2);

    return true;
}