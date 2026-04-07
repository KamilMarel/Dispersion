#include "RenderPass/RenderPass.h"

unsigned int RenderPass::windowQuadVAO = 0;
unsigned int RenderPass::windowQuadVBO = 0;

RenderPass::RenderPass(unsigned int windowWidth, unsigned int windowHeight)
{
	this->windowWidth = windowWidth;
	this->windowHeight = windowHeight;
}

RenderPass::~RenderPass()
{
}

void RenderPass::renderWindowQuad()
{
    if (RenderPass::windowQuadVAO == 0)
    {
        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &RenderPass::windowQuadVAO);
        glGenBuffers(1, &RenderPass::windowQuadVBO);
        glBindVertexArray(RenderPass::windowQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, RenderPass::windowQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(RenderPass::windowQuadVAO);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
}