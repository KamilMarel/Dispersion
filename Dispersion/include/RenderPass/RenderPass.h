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

	virtual void execute(SceneGraphNode* sceneRoot, const glm::mat4& cameraView, const glm::mat4& cameraProjection, const Spotlight& sceneLight) = 0;
	virtual Framebuffer& getResult() = 0;

protected:
	unsigned int windowWidth, windowHeight;

	void renderWindowQuad();
};