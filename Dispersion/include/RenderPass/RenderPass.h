#include "Framebuffer.h"
#include "SceneGraphNode.h"
#include "Spotlight.h"
#include "Camera.h"

class RenderPass
{
public:
    static unsigned int windowQuadVAO;
    static unsigned int windowQuadVBO;
	
	RenderPass(unsigned int windowWidth, unsigned int windowHeight);
	~RenderPass();

	virtual void execute(SceneGraphNode* sceneRoot, glm::mat4& cameraView, glm::mat4& cameraProjection, Spotlight& sceneLight) = 0;
	virtual std::vector<unsigned int> getOutputTextures() = 0;

protected:
	unsigned int windowWidth, windowHeight;

	void renderWindowQuad();
};