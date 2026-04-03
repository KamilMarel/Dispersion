#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"

#include <string>
#include <vector>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
    glm::vec3 NormalInnerIntersectionPoint;
    float distanceAlongNormal;
};

struct Texture {
    unsigned int id;
    string type;
    string path;
};

class Mesh {
public:
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<Texture>      textures;
    unsigned int VAO;

    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        vector<glm::vec3> normalInnerIntersectionPoints = getNormalInnerIntersectionPoints(vertices, indices);

        for (int vertexIndex = 0; vertexIndex < vertices.size(); vertexIndex++)
        {

            this->vertices[vertexIndex].NormalInnerIntersectionPoint = normalInnerIntersectionPoints[vertexIndex];
            this->vertices[vertexIndex].distanceAlongNormal = glm::distance(normalInnerIntersectionPoints[vertexIndex], vertices[vertexIndex].Position);
        }

        setupMesh();
    }

    void Draw(Shader& shader)
    {
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;
        for (unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            string number;
            string name = textures[i].type;
            if (name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if (name == "texture_specular")
                number = std::to_string(specularNr++);
            else if (name == "texture_normal")
                number = std::to_string(normalNr++);
            else if (name == "texture_height")
                number = std::to_string(heightNr++);

            glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE0);
    }

private:
    unsigned int VBO, EBO;
    float epsilon = std::numeric_limits<float>::epsilon();

    std::vector<glm::vec3> getNormalInnerIntersectionPoints(std::vector<Vertex>& vertices, vector<unsigned int>& indices)
    {
        std::vector<glm::vec3> normalInnerIntersectionPoints;

        for (int vertexIndex = 0; vertexIndex < vertices.size(); vertexIndex++)
        {
            glm::vec3 origin = vertices[vertexIndex].Position;
            glm::vec3 direction = glm::normalize(-vertices[vertexIndex].Normal);

            glm::vec3 intersectionPoint(0.0f, 0.0f, 0.0f);
            bool firstIntersectionPointFound = false;

            for (int firstIndiceIndex = 0; firstIndiceIndex < indices.size(); firstIndiceIndex += 3)
            {
                glm::vec3 p1 = vertices[indices[firstIndiceIndex]].Position;
                glm::vec3 p2 = vertices[indices[firstIndiceIndex + 1]].Position;
                glm::vec3 p3 = vertices[indices[firstIndiceIndex + 2]].Position;

                glm::vec3 getPointOutput;
                if (getRayTriangleIntersectionPoint(getPointOutput, origin, direction, p1, p2, p3))
                {
                    if (!firstIntersectionPointFound)
                    {
                        firstIntersectionPointFound = true;
                        intersectionPoint = getPointOutput;
                    }
                    else if (glm::distance(origin, getPointOutput) <
                             glm::distance(origin, intersectionPoint))
                    {
                        intersectionPoint = getPointOutput;
                    }
                }
            }
            normalInnerIntersectionPoints.push_back(intersectionPoint);
        }
        return normalInnerIntersectionPoints;
    }

    bool getRayTriangleIntersectionPoint(glm::vec3& outPoint,
                                         glm::vec3& rayOrigin,
                                         glm::vec3& rayVector,
                                         glm::vec3& triangleVertexA,
                                         glm::vec3& triangleVertexB,
                                         glm::vec3& triangleVertexC)
    {
        glm::vec3 edge1 = triangleVertexB - triangleVertexA;
        glm::vec3 edge2 = triangleVertexC - triangleVertexA;
        glm::vec3 rayCrossEdge2 = glm::cross(rayVector, edge2);
        float det = glm::dot(edge1, rayCrossEdge2);

        // The ray is parallel to the triangle
        if (det > -epsilon && det < epsilon)
        {
            return false;
        }

        float invDet = 1.0f / det;
        glm::vec3 s = rayOrigin - triangleVertexA;
        float u = invDet * glm::dot(s, rayCrossEdge2);

        if ((u < 0 && abs(u) > epsilon) || (u > 1 && abs(u - 1) > epsilon))
        {
            return false;
        }

        glm::vec3 sCrossEdge1 = glm::cross(s, edge1);
        float v = invDet * glm::dot(rayVector, sCrossEdge1);

        if ((v < 0 && abs(v) > epsilon) || (u + v > 1 && abs(u + v - 1) > epsilon))
        {
            return false;
        }

        float t = invDet * glm::dot(edge2, sCrossEdge1);

        // Ray intersection
        if (t > epsilon)
        {
            outPoint = rayOrigin + (rayVector * t);
            return true;
        }
        // There is a line intersection
        else
        {
            return false;
        }
    }

    void setupMesh()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, NormalInnerIntersectionPoint));

        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, distanceAlongNormal));

        glBindVertexArray(0);
    }
};
#endif