#include "RenderPass/RenderPass.h"

class DeferredShadingPass : public RenderPass
{
public:
	DeferredShadingPass(unsigned int windowWidth, unsigned int windowHeight);
	~DeferredShadingPass();

	void execute(SceneGraphNode* sceneRoot, const glm::mat4& cameraView, const glm::mat4& cameraProjection, const Spotlight& sceneLight);
	Framebuffer& getResult();
private:
	Framebuffer gBuffer;
	Framebuffer resultBuffer;

	Shader cameraGeometryPassShader;
	Shader cameraLightingPassShader;
};
