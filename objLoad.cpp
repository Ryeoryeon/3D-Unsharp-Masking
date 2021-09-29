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
    glGenBuffers(1, &elementBuffer); // ���� ����
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
    // (1) : �̿��� ����
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

    // (2) : �̿��� �ε���
    GLuint neighborNumIdxTexID;
    glGenTextures(1, &neighborNumIdxTexID);
    glBindTexture(GL_TEXTURE_2D, neighborNumIdxTexID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, TEXTURESIZE, TEXTURESIZE, 0, GL_RED_INTEGER, GL_INT, neighborIdxList);

    // (3) : ������ �̿��� ����
    GLuint accumNeighborNumTexID;
    glGenTextures(1, &accumNeighborNumTexID);
    glBindTexture(GL_TEXTURE_2D, accumNeighborNumTexID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, TEXTURESIZE, TEXTURESIZE, 0, GL_RED_INTEGER, GL_INT, accumNeighborNum);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer); // �߰��� element �ڵ�

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

    // �ٿ�� �ڽ��� 8�� ��ǥ�� ���ϱ�
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

    // �ٿ�� �ڽ��� �߽��� ��ǥ ���ϱ�
    boxCent.x = (maxCoordX + minCoordX) * 0.5f;
    boxCent.y = (maxCoordY + minCoordY) * 0.5f;
    boxCent.z = (maxCoordZ + minCoordZ) * 0.5f;

    // �ٿ�� �ڽ��� ��ǥ��
    std::vector<point4> boundingBoxCoordinate;
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
    float boundingDist = getDist(point3(maxCoordX, maxCoordY, maxCoordZ), boxCent);

    float scalingSize = scalingFactor / boundingDist;
    glm::mat4 scalingMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scalingSize, scalingSize, scalingSize));
    glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-boxCent.x + destinationCent.x, -boxCent.y + +destinationCent.y, -boxCent.z + destinationCent.z));
    //glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-boxCent.x, -boxCent.y, -boxCent.z));
    glm::mat4 transformMatrix = scalingMatrix * translateMatrix;

    // ��İ� ��ǥ �� ����� GPU����
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

    // *** 3D Unsharp Masking�� ����
    // vertex : normal 1:1 ������ ���� (#average)
    std::map <std::pair<int, int>, int> checkOverlapNormal;
    std::vector<std::vector<int>> adjNeighborList; // �� �ؽ�ó�� ���� ���� ����Ʈ
    std::vector<int> vertexPerFace; // �� ������ ��� �븻 ����� ���� �����Ǵ� ���� ����
    // ***

    //std::vector<point3> verticesPosition;
    std::map <std::pair<int, int>, int> checkOverlap; // �ؽ�ó���� �ߺ��Ǵ� �� ������ ���ϱ� ���� map
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
            vertexPos.push_back(vertex);
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
                int vertexPosNum = vertexPos.size();
                adjNeighborList.resize(vertexPosNum);
                diffuse.resize(vertexPosNum);
                ambient.resize(vertexPosNum);
                specular.resize(vertexPosNum);
                //normalIndices.resize(vertexPosNum); // ���� vn index ����

                normal.assign(vertexPosNum, { 0, 0, 0 }); // face�� avg normal ����
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

            forFaceNormalVec1.x = vertexPos[vertexPosIndex[1]].x - vertexPos[vertexPosIndex[0]].x;
            forFaceNormalVec1.y = vertexPos[vertexPosIndex[1]].y - vertexPos[vertexPosIndex[0]].y;
            forFaceNormalVec1.z = vertexPos[vertexPosIndex[1]].z - vertexPos[vertexPosIndex[0]].z;

            forFaceNormalVec2.x = vertexPos[vertexPosIndex[2]].x - vertexPos[vertexPosIndex[0]].x;
            forFaceNormalVec2.y = vertexPos[vertexPosIndex[2]].y - vertexPos[vertexPosIndex[0]].y;
            forFaceNormalVec2.z = vertexPos[vertexPosIndex[2]].z - vertexPos[vertexPosIndex[0]].z;

            glm::vec3 faceNormal = glm::cross(forFaceNormalVec1, forFaceNormalVec2);
            faceNormal /= glm::length(faceNormal); // �߿�!

            for (int i = 0; i < 3; ++i)
            {
                // vertex position indexing
                vertexPosIndices.push_back(vertexPosIndex[i]);

                // ---> vertex : vertex normal vector 1:1 ������ ���� ���� �κ� 
                normal[vertexPosIndex[i]].x += faceNormal.x;
                normal[vertexPosIndex[i]].y += faceNormal.y;
                normal[vertexPosIndex[i]].z += faceNormal.z;

                // �� ������ ���� color
                diffuse[vertexPosIndex[i]] = mtlData[materialPointer].Kd;
                ambient[vertexPosIndex[i]] = mtlData[materialPointer].Ka;
                specular[vertexPosIndex[i]] = mtlData[materialPointer].Ks;

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

    // 3D Unsharp Masking�� ���� �ؽ�ó
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

        // normal ��� ���ֱ�
        normal[i].x /= vertexPerFace[i];
        normal[i].y /= vertexPerFace[i];
        normal[i].z /= vertexPerFace[i];

        // ������ 1 ����ȭ
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