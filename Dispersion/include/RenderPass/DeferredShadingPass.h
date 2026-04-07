#include "RenderPass/RenderPass.h"

class DeferredShadingPass : public RenderPass
{
public:
	DeferredShadingPass(unsigned int windowWidth, unsigned int windowHeight);
	~DeferredShadingPass();

	void execute(SceneGraphNode* sceneRoot, glm::mat4& cameraView, glm::mat4& cameraProjection, Spotlight& sceneLight);
	std::vector<unsigned int> getOutputTextures();
private:
	Framebuffer gBuffer;

	Shader cameraGeometryPassShader;
	Shader cameraLightingPassShader;
};
