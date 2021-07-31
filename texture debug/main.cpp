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

const int SCREENSIZE = 512;
const int TEXTURESIZE = 128;

static int triangleNum = 0;
static GLuint programID;
static GLuint VertexBufferID;
static GLuint TextureCoordID;

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path);
bool loadObjMtl(const char* objName, const char* mtlName, int& faceNum);

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

// texture ����
static std::vector<std::vector<int>> adjNeighborList;
static GLubyte neighborNum[TEXTURESIZE * TEXTURESIZE] = { 0, };
static GLubyte neighborIdxList[TEXTURESIZE * TEXTURESIZE] = { 0, };
static std::map <std::pair<int, int>, int> checkOverlap;

// �ʿ� ������ ���� �Լ��� ������ ���� ����
static std::vector<point3> vertices;
static std::vector<point3> normals;
static std::vector<point3> specularColors;
static std::vector<point3> ambientColors;
static std::vector<point4> diffuseColors;
//

void init()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // model load
    const char* objName = "bunny(withMtl).obj";
    const char* mtlName = "bunny.mtl";
    memset(neighborNum, 255, sizeof(neighborNum));
    
    int faceNum = 0;
    bool res = loadObjMtl(objName, mtlName, faceNum);

    float squareVertices[] = {
         1.0f,  1.0f, 0.0f,   
         1.0f, -1.0f, 0.0f, 
        -1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
    };

    float textureCoord[] = {
     1.0f,  1.0f,
     1.0f,  0.0f,
     0.0f,  0.0f,
     0.0f,  1.0f
    };

    triangleNum = 2;

    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glGenBuffers(1, &VertexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &TextureCoordID);
    glBindBuffer(GL_ARRAY_BUFFER, TextureCoordID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoord), textureCoord, GL_STATIC_DRAW);

    // 2���� �ﰢ���� ����
    GLuint ElementID;
    glGenBuffers(1, &ElementID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Texture
    GLuint TextureID;
    glGenTextures(1, &TextureID);
    glBindTexture(GL_TEXTURE_2D, TextureID);

    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //GLubyte textureColor[TEXTURESIZE][TEXTURESIZE] = {0,};
    //GLuint textureColor[TEXTURESIZE][TEXTURESIZE] = {0,};

    GLubyte textureColor[TEXTURESIZE * TEXTURESIZE * 3] = {0,};
    //memset(textureColor, 255, sizeof(textureColor));

    textureColor[0] = 255;
    textureColor[1] = 0;
    textureColor[2] = 0;

    textureColor[3] = 0;
    textureColor[4] = 255;
    textureColor[5] = 0;

    textureColor[6] = 0;
    textureColor[7] = 0;
    textureColor[8] = 255;

    // �迭�� ����
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, TEXTURESIZE, TEXTURESIZE, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, neighborIdxList);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, TEXTURESIZE, TEXTURESIZE, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, neighborNum);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURESIZE, TEXTURESIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, textureColor);
    //
    
    programID = LoadShaders("texture.vertexshader", "texture.fragmentshader");
    glUniform1i(glGetUniformLocation(programID, "myTexture"), 0);
    glUseProgram(programID);
}

void mydisplay() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, TextureCoordID);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawElements(GL_TRIANGLES, (triangleNum * 3), GL_UNSIGNED_INT, 0);
    //glDrawArrays(GL_TRIANGLES, 0, 3);
    // Starting from vertex 0; 3 vertices -> 1 triangle
    glDisableVertexAttribArray(0);
    glFlush();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitWindowSize(SCREENSIZE, SCREENSIZE);
    glutInitWindowPosition(500, 0);
    glutCreateWindow("Tutorial 02");
    glutInitDisplayMode(GLUT_DEPTH | GLUT_SINGLE | GLUT_RGBA);
    glutDisplayFunc(mydisplay);

    GLenum err = glewInit();
    if (err == GLEW_OK) {
        init();
        glutMainLoop();
    }
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

bool loadObjMtl(const char* objName, const char* mtlName, int& faceNum)
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

    std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;

    std::vector<point3> temp_vertices;
    std::vector<point3> temp_normals;
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
            temp_vertices.push_back(vertex);
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

        // ù �ܾ vt��� uv�� �д´� 
        /*
         else if (strcmp(lineHeader, "vt") == 0)
        {
            glm::vec2 uv;
            fscanf(fp, "%f %f\n", &uv.x, &uv.y);
            temp_uvs.push_back(uv);
        }
        */


        // ù �ܾ vn�̶��, normal�� �д´�
        else if (strcmp(lineHeader, "vn") == 0)
        {
            point3 normal;
            fscanf(fp, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            temp_normals.push_back(normal);
        }

        // ù �ܾ f���, face�� �д´�
        else if (strcmp(lineHeader, "f") == 0)
        {
            unsigned int vertexIndex[3], normalIndex[3];
            std::vector<int> temp_facelist; // face table�� ����� ������ �ӽ÷� �����ϴ� ����

            char str[128];
            fgets(str, sizeof(str), fp);
            char* ptr = strtok(str, " //");
            int ptrSize = 0;

            while (ptr != NULL) // �ڸ� ���ڿ��� ������ ���� ������ ���
            {
                temp_facelist.push_back(atoi(ptr));
                ptr = strtok(NULL, " //");
                ++ptrSize;
            }

            mtlData[materialPointer].Kd.w = mtlData[materialPointer].d;

            // f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ������ �����
            if (ptrSize == 9)
            {
                int temp1 = 0;
                int temp2 = 0;

                for (int i = 0; i < 9; i++)
                {
                    if (i % 3 == 0)
                    {
                        vertexIndex[temp1++] = temp_facelist[i];
                    }

                    else if (i % 3 == 2)
                    {
                        normalIndex[temp2++] = temp_facelist[i];
                    }
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

            }

            // f v1//vn1 v2//vn2 v3//vn3 ������ �����
            else if (ptrSize == 6)
            {
                int temp1 = 0;
                int temp2 = 0;

                for (int i = 0; i < 6; i++)
                {
                    if (i % 2 == 0)
                    {
                        vertexIndex[temp1++] = temp_facelist[i];
                    }

                    else
                    {
                        normalIndex[temp2++] = temp_facelist[i];
                    }
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

            if (!faceReadingStart) // ó������ face�� �о�� ��, ���� ����Ʈ ũ�� �ʱ�ȭ
                adjNeighborList.resize(temp_vertices.size() + 1);

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
        point3 vertex = temp_vertices[vertexIdx - 1];
        point3 normal = temp_normals[normalIdx - 1];

        vertices.push_back(vertex);
        normals.push_back(normal);
    }

    // 3D Unsharp Masking�� ���� �ؽ���
    int verticesNum = temp_vertices.size() + 1; // ���� 0������ �����ϹǷ� �ӽ� +1
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