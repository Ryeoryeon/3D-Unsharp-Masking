#define _CRT_SECURE_NO_WARNINGS
#include "common.h"

double getDist(point3 p1, point3 p2)
{
    float xDist = pow(p1.x - p2.x, 2);
    float yDist = pow(p1.y - p2.y, 2);
    float zDist = pow(p1.z - p2.z, 2);

    return sqrt(xDist + yDist + zDist);
}

int Object::initResource(GLuint programID)
{
    // (1) vertex position
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexPos.size() * sizeof(point3), &vertexPos[0], GL_STATIC_DRAW);

    // (2) vertex element
    glGenBuffers(1, &elementBuffer); // 버퍼 생성
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexPosIndices.size() * sizeof(unsigned int), &vertexPosIndices[0], GL_STATIC_DRAW);

    // (3) diffuse
    glGenBuffers(1, &diffuseID);
    glBindBuffer(GL_ARRAY_BUFFER, diffuseID);
    glBufferData(GL_ARRAY_BUFFER, vertexPos.size() * sizeof(point4), &diffuse[0], GL_STATIC_DRAW);

    // (4) ambient
    glGenBuffers(1, &ambientID);
    glBindBuffer(GL_ARRAY_BUFFER, ambientID);
    glBufferData(GL_ARRAY_BUFFER, vertexPos.size() * sizeof(point3), &ambient[0], GL_STATIC_DRAW);

    // (5) specular
    glGenBuffers(1, &specularID);
    glBindBuffer(GL_ARRAY_BUFFER, specularID);
    glBufferData(GL_ARRAY_BUFFER, vertexPos.size() * sizeof(point3), &specular[0], GL_STATIC_DRAW);

    // (6) normal (vertex avg normal)
    glGenBuffers(1, &NormalID);
    glBindBuffer(GL_ARRAY_BUFFER, NormalID);
    glBufferData(GL_ARRAY_BUFFER, normal.size() * sizeof(point3), &normal[0], GL_STATIC_DRAW);

    texLocation = glGetUniformLocation(programID, "neighborNum");
    tex2Location = glGetUniformLocation(programID, "neighborIdxList");
    tex3Location = glGetUniformLocation(programID, "accumNeighborNum");

    return 1;
}

void Object::draw(GLint uniformMvp, const glm::mat4 mvp, const int TEXTURESIZE)
{
    glUniformMatrix4fv(uniformMvp, 1, GL_FALSE, &mvp[0][0]);

    // *** pointer
    // (1) vertex position
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // (3) diffuse
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, diffuseID);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // (4) ambient
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, ambientID);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // (5) specular
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, specularID);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // (6) normal
    glEnableVertexAttribArray(4);
    glBindBuffer(GL_ARRAY_BUFFER, NormalID);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Texture
    // (1) : 이웃의 개수
    GLuint neighborNumTexID;
    glGenTextures(1, &neighborNumTexID);
    glBindTexture(GL_TEXTURE_2D, neighborNumTexID);

    // texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, TEXTURESIZE, TEXTURESIZE, 0, GL_RED_INTEGER, GL_INT, neighborNum);

    // (2) : 이웃의 인덱스
    GLuint neighborNumIdxTexID;
    glGenTextures(1, &neighborNumIdxTexID);
    glBindTexture(GL_TEXTURE_2D, neighborNumIdxTexID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, TEXTURESIZE, TEXTURESIZE, 0, GL_RED_INTEGER, GL_INT, neighborIdxList);

    // (3) : 누적된 이웃의 개수
    GLuint accumNeighborNumTexID;
    glGenTextures(1, &accumNeighborNumTexID);
    glBindTexture(GL_TEXTURE_2D, accumNeighborNumTexID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, TEXTURESIZE, TEXTURESIZE, 0, GL_RED_INTEGER, GL_INT, accumNeighborNum);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer); // 추가한 element 코드

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, neighborNumTexID);
    glUniform1i(texLocation, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, neighborNumIdxTexID);
    glUniform1i(tex2Location, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, accumNeighborNumTexID);
    glUniform1i(tex3Location, 2);

    glDrawElements(
        GL_TRIANGLES,
        vertexPosIndices.size(), // count
        GL_UNSIGNED_INT,
        (void*)0 // offset
    );
}

void Object::disable()
{
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);

    glDisable(GL_TEXTURE_2D);
}

void Object::deleteBuffers()
{
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &diffuseID);
    glDeleteBuffers(1, &ambientID);
    glDeleteBuffers(1, &specularID);
    glDeleteBuffers(1, &NormalID);

    glDeleteTextures(1, &neighborNumTexID);
    glDeleteTextures(1, &neighborNumIdxTexID);
    glDeleteTextures(1, &accumNeighborNumTexID);
}

glm::mat4 Object::boundingBox(const point3 destinationCent, const float scalingFactor)
{
    point3 boxCent;

    float maxCoordX = std::numeric_limits<float>::min();
    float maxCoordY = std::numeric_limits<float>::min();
    float maxCoordZ = std::numeric_limits<float>::min();

    float minCoordX = std::numeric_limits<float>::max();
    float minCoordY = std::numeric_limits<float>::max();
    float minCoordZ = std::numeric_limits<float>::max();

    int num = 0;

    // 바운딩 박스의 8개 좌표들 구하기
    for (int i = 0; i < vertexPos.size(); i++)
    {
        point3 temp = vertexPos[i];

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

    // 바운딩 박스의 중심의 좌표 구하기
    boxCent.x = (maxCoordX + minCoordX) * 0.5f;
    boxCent.y = (maxCoordY + minCoordY) * 0.5f;
    boxCent.z = (maxCoordZ + minCoordZ) * 0.5f;

    // 바운딩 박스의 좌표들
    std::vector<point4> boundingBoxCoordinate;
    boundingBoxCoordinate.push_back(point4(maxCoordX, maxCoordY, maxCoordZ, 1));
    boundingBoxCoordinate.push_back(point4(maxCoordX, maxCoordY, minCoordZ, 1));
    boundingBoxCoordinate.push_back(point4(maxCoordX, minCoordY, maxCoordZ, 1));
    boundingBoxCoordinate.push_back(point4(minCoordX, maxCoordY, maxCoordZ, 1));
    boundingBoxCoordinate.push_back(point4(maxCoordX, minCoordY, minCoordZ, 1));
    boundingBoxCoordinate.push_back(point4(minCoordX, maxCoordY, minCoordZ, 1));
    boundingBoxCoordinate.push_back(point4(minCoordX, minCoordY, maxCoordZ, 1));
    boundingBoxCoordinate.push_back(point4(minCoordX, minCoordY, minCoordZ, 1));

    // 바운딩 박스의 대각선 길이
    // maxCoordX를 기준으로 잡을 시, 절대 minCoordX와는 최대거리가 성립하지 않는다
    float boundingDist = getDist(point3(maxCoordX, maxCoordY, maxCoordZ), boxCent);

    float scalingSize = scalingFactor / boundingDist;
    glm::mat4 scalingMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scalingSize, scalingSize, scalingSize));
    glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-boxCent.x + destinationCent.x, -boxCent.y + +destinationCent.y, -boxCent.z + destinationCent.z));
    //glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-boxCent.x, -boxCent.y, -boxCent.z));
    glm::mat4 transformMatrix = scalingMatrix * translateMatrix;

    // 행렬과 좌표 곱 계산은 GPU에서
    return transformMatrix;
}

bool Object::loadObjMtl(const char* objName, const char* mtlName)
{
    // .mtl load
    FILE* fp2;
    fp2 = fopen(mtlName, "r");

    if (fp2 == NULL) {
        printf("Impossible to open the .mtl file !\n");
        return false;
    }

    std::vector<MaterialData> mtlData;
    bool first = true; // 첫번째 newmtl인가?
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
            if (first) // 아직 넣을 데이터가 없을 때
            {
                first = false;
                temp = MaterialData(); // temp 초기화
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

        else if (strcmp(lineHeader, "Ke") == 0) // Ke는 건너뛰기
            continue;

        else if (strcmp(lineHeader, "illum") == 0)
            continue;

        // 나머지 문자열일 경우, material의 이름을 포함하고 있는지 확인해보자
        else
        {
            //char* originalName;
            char originalName[128];
            strcpy(originalName, lineHeader); //원본 보존
            char* nameTemp = strtok(lineHeader, "_");

            if (strcmp(nameTemp, "material") == 0)
                temp.name = originalName;

            else
                continue;
        }
    }

    mtlData.push_back(temp);
    // 유효한 mtlData인지 검사하기
    int mtlSize = mtlData.size();
    for (int i = 0; i < mtlSize; ++i)
    {
        if (mtlData[i].name == "")
            exit(0);
    }

    // *********** .load
    FILE* fp;
    fp = fopen(objName, "r");

    if (fp == NULL) {
        printf("Impossible to open the .file !\n");
        return false;
    }

    int faceNum = 0;
    std::vector<unsigned int> vertexIndices;
    std::vector<unsigned int> textureIndices;
    std::vector<std::vector<unsigned int>> normalIndices;

    // *** 3D Unsharp Masking을 위해
    // vertex : normal 1:1 대응을 위해 (#average)
    std::map <std::pair<int, int>, int> checkOverlapNormal;
    std::vector<std::vector<int>> adjNeighborList; // 두 텍스처를 위한 인접 리스트
    std::vector<int> vertexPerFace; // 각 점에서 평균 노말 계산을 위해 대응되는 면의 개수
    // ***

    //std::vector<point3> verticesPosition;
    std::map <std::pair<int, int>, int> checkOverlap; // 텍스처에서 중복되는 점 저장을 피하기 위한 map
    std::vector<point3> verticesNormals; // normal의 값 그 자체들이 저장
    std::vector<point3> verticesTexCoord; // vt의 값 그 자체들이 저장

    int materialPointer = -1; // 넣어야 할 material // -1로 초기화해주는 이유는 예외처리를 위해
    bool faceReadingStart = false;

    while (1)
    {
        char lineHeader[128];
        //char originalName[128];

        int res = fscanf(fp, "%s", lineHeader);
        if (res == EOF)
            break;
        //strcpy(originalName, lineHeader); // 원본 보존

        // 첫 단어가 v인 경우, vertex를 읽는다
        if (strcmp(lineHeader, "v") == 0)
        {
            point3 vertex;
            fscanf(fp, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
            vertexPos.push_back(vertex);
        }

        if (strcmp(lineHeader, "usemtl") == 0)
        {
            int res = fscanf(fp, "%s", lineHeader); // 공백 한 번 더 읽기
            // 어떤 material을 부여할지 검사
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

        // 첫 단어가 vn이라면, normal을 읽는다
        else if (strcmp(lineHeader, "vn") == 0)
        {
            point3 normal;
            fscanf(fp, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            verticesNormals.push_back(normal);
        }

        // 첫 단어가 f라면, face를 읽는다
        else if (strcmp(lineHeader, "f") == 0)
        {
            unsigned int vertexPosIndex[3], normalIndex[3], texCoordIndex[3];
            std::vector<int> tempFaceList; // face table에 저장된 수들을 임시로 저장하는 벡터

            char str[128];
            fgets(str, sizeof(str), fp);
            str[strlen(str) - 1] = '\0';
            char* ptr = strtok(str, " //");
            int ptrSize = 0;

            while (ptr != NULL) // 자른 문자열이 나오지 않을 때까지 출력
            {
                tempFaceList.push_back(atoi(ptr));
                ptr = strtok(NULL, " //");
                ++ptrSize;
            }

            mtlData[materialPointer].Kd.w = mtlData[materialPointer].d;

            if (!faceReadingStart) // 처음으로 face를 읽어올 때, 인접 리스트와 색상 배열 크기 초기화
            {
                int vertexPosNum = vertexPos.size();
                adjNeighborList.resize(vertexPosNum);
                diffuse.resize(vertexPosNum);
                ambient.resize(vertexPosNum);
                specular.resize(vertexPosNum);
                //normalIndices.resize(vertexPosNum); // 기존 vn index 저장

                normal.assign(vertexPosNum, { 0, 0, 0 }); // face의 avg normal 저장
                vertexPerFace.assign(vertexPosNum, 0);
                faceReadingStart = true;
            }

            // f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 순으로 저장됨
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

            // f v1//vn1 v2//vn2 v3//vn3 순으로 저장됨
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

                // texture 좌표가 존재하지 않는 face의 경우, (종종 섞여있을 수 있다)
                // -> 유효하지 않도록 모두 1번째(같은) 인덱스로 삽입
                texCoordIndex[temp3++] = 1;
                texCoordIndex[temp3++] = 1;
                texCoordIndex[temp3++] = 1;
            }

            // f v1// v2// f3// 순으로 저장되는 형태
            else if (ptrSize == 3)
            {
                vertexPosIndex[0] = tempFaceList[0] - 1;
                vertexPosIndex[1] = tempFaceList[1] - 1;
                vertexPosIndex[2] = tempFaceList[2] - 1;

                // texture 좌표가 존재하지 않는 face의 경우, (종종 섞여있을 수 있다)
                // -> 유효하지 않도록 모두 1번째(같은) 인덱스로 삽입
                texCoordIndex[0] = 1;
                texCoordIndex[1] = 1;
                texCoordIndex[2] = 1;
            }

            else
            {
                std::cout << "This File can't be read by our simple parser : Try exporting with other options\n";
                return false;
            }

            // face normal vector 계산
            glm::vec3 forFaceNormalVec1;
            glm::vec3 forFaceNormalVec2;

            forFaceNormalVec1.x = vertexPos[vertexPosIndex[1]].x - vertexPos[vertexPosIndex[0]].x;
            forFaceNormalVec1.y = vertexPos[vertexPosIndex[1]].y - vertexPos[vertexPosIndex[0]].y;
            forFaceNormalVec1.z = vertexPos[vertexPosIndex[1]].z - vertexPos[vertexPosIndex[0]].z;

            forFaceNormalVec2.x = vertexPos[vertexPosIndex[2]].x - vertexPos[vertexPosIndex[0]].x;
            forFaceNormalVec2.y = vertexPos[vertexPosIndex[2]].y - vertexPos[vertexPosIndex[0]].y;
            forFaceNormalVec2.z = vertexPos[vertexPosIndex[2]].z - vertexPos[vertexPosIndex[0]].z;

            glm::vec3 faceNormal = glm::cross(forFaceNormalVec1, forFaceNormalVec2);
            faceNormal /= glm::length(faceNormal); // 중요!

            for (int i = 0; i < 3; ++i)
            {
                // vertex position indexing
                vertexPosIndices.push_back(vertexPosIndex[i]);

                // ---> vertex : vertex normal vector 1:1 대응을 위한 공통 부분 
                normal[vertexPosIndex[i]].x += faceNormal.x;
                normal[vertexPosIndex[i]].y += faceNormal.y;
                normal[vertexPosIndex[i]].z += faceNormal.z;

                // 점 세개에 대한 color
                diffuse[vertexPosIndex[i]] = mtlData[materialPointer].Kd;
                ambient[vertexPosIndex[i]] = mtlData[materialPointer].Ka;
                specular[vertexPosIndex[i]] = mtlData[materialPointer].Ks;

                // 호출되는 점에 대해 면의 개수 증가 (평균 작업 위해)
                ++vertexPerFace[vertexPosIndex[i]];
            }

            // 1-ring neighborhood를 위한 인접 리스트 삽입
            int tempVer0 = vertexPosIndex[0], tempVer1 = vertexPosIndex[1], tempVer2 = vertexPosIndex[2];

            // 아직 체크되지 않은 정점 관계라면 push_back();
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

        // 첫 단어가 l일 때

        //
    }

    // 3D Unsharp Masking을 위한 텍스처
    int vertexPosNum = vertexPos.size();
    //neighborNum.assign(verticesNum, 0);
    int neighborSize;
    int tempNeighborIdx = 0;
    int tempNormalIdx = 0;

    for (int i = 0; i < vertexPosNum; ++i)
    {
        neighborSize = adjNeighborList[i].size();
        neighborNum[i] = neighborSize;

        for (int j = 0; j < neighborSize; ++j)
        {
            neighborIdxList[tempNeighborIdx] = adjNeighborList[i][j];
            ++tempNeighborIdx;
        }

        // normal 평균 내주기
        normal[i].x /= vertexPerFace[i];
        normal[i].y /= vertexPerFace[i];
        normal[i].z /= vertexPerFace[i];

        // 사이즈 1 정규화
        float normalSize = sqrt(pow(normal[i].x, 2) + pow(normal[i].y, 2) + pow(normal[i].z, 2));
        normal[i].x /= normalSize;
        normal[i].y /= normalSize;
        normal[i].z /= normalSize;

        if (i == 0)
            continue;

        if (i == 1)
            accumNeighborNum[i] = neighborNum[i - 1];

        else
            accumNeighborNum[i] = accumNeighborNum[i - 1] + neighborNum[i - 1];
    }

    fclose(fp);
    fclose(fp2);

    return true;
}