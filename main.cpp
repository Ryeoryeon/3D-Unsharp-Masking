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

std::vector<glm::vec3> lightPos = { glm::vec3(0, 5, 5) , glm::vec3(4, 4, 0), glm::vec3(3, 1, 3), glm::vec3(-2, 5, 3)};

// timer 함수 연관
float angle; // 회전 각도
int lightIdx = 0; // 조명의 번호

Object firstObj;
Object secondObj;
std::vector<int> activateObj = { 1, 1 }; // obj 활성화 변수

glm::mat4 Projection;
glm::mat4 View;
glm::mat4 Model;
glm::mat4 mvp;

glm::mat4 transformMatrix;
glm::mat4 transformMatrixSecond;

GLuint programID;

// VAO
//GLuint vao;

void init();
void transform();
void mydisplay();
void myreshape(int width, int height);
void openglToPngSave(int outputIdx);
GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path);

using namespace std;

void timer(int value)
{
    static int outputIdx = -1; // 빈 화면 건너뛰기

    glutPostRedisplay(); // 윈도우를 다시 그리도록 요청하는 함수
    glutTimerFunc(45, timer, 0);
    angle += glm::radians(45.0f);

    ++outputIdx;

    if (outputIdx != 0 && outputIdx <= 8)
        openglToPngSave(outputIdx - 1);

    // 360도 회전이 끝나면 다음 조명으로 변경 (0~11, 12~23..)
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
    //const char* objName = "bunny(withMtl).obj";
    //const char* objName = "bunny(moreFace_withMtl).obj";
    //const char* mtlName = "bunny.mtl";

    //const char* objName = "armadillo.obj";
    //const char* mtlName = "armadillo.mtl";

    const char* objName = "utahTeapot.obj";
    const char* mtlName = "utahTeapot.mtl";

    //const char* secondObjName = "utahTeapot.obj";
    //const char* secondMtlName = "pink.mtl";

    //const char* secondObjName = "bunny(withMtl).obj";
    //const char* secondMtlName = "blue.mtl";

    const char* secondObjName = "sphere.obj";
    const char* secondMtlName = "sphere.mtl";

    programID = LoadShaders("1ringNeiborhood_bilateral.vertexshader", "1ringNeiborhood.fragmentshader");
    //programID = LoadShaders("1ringNeiborhood.vertexshader", "1ringNeiborhood.fragmentshader");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glUseProgram(programID);

    if (activateObj[0])
    {
        firstObj.loadObjMtl(objName, mtlName);
        transformMatrix = firstObj.boundingBox(point3(0, 0, 0), 3.0f);
        firstObj.initResource(programID);
    }

    if (activateObj[1])
    {
        secondObj.loadObjMtl(secondObjName, secondMtlName);
        transformMatrixSecond = secondObj.boundingBox(point3(0, 0.5f, 1.0f), 2.0f); // 토끼 최적
        //transformMatrixSecond = secondObj.boundingBox(point3(0, 1.5f, 2.0f), 2.1f); // 주전자/구 최적
        secondObj.initResource(programID);
    }
}

void mydisplay()
{
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    transform();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint transformID = glGetUniformLocation(programID, "Transform");

    firstObj.draw(transformID, transformMatrix, TEXTURESIZE);
    firstObj.disable();

    secondObj.draw(transformID, transformMatrixSecond, TEXTURESIZE);
    secondObj.disable();

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

void openglToPngSave(int outputIdx)
{
    static int fileNo = 0;
    int bitsNum;
    GLubyte* bits; //RGB bits
    GLint captureImage[4]; //current viewport

    //get current viewport
    glGetIntegerv(GL_VIEWPORT, captureImage); // 이미지 크기 알아내기

    int rows = captureImage[3];
    int cols = captureImage[2];

    bitsNum = 3 * cols * rows;
    bits = new GLubyte[bitsNum]; // opengl에서 읽어오는 비트

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