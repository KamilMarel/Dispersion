#include "RenderPass/RenderPass.h"

class DeferredShadingPass : public RenderPass
{
public:
	DeferredShadingPass(unsigned int windowWidth, unsigned int windowHeight);
	~DeferredShadingPass();

	void execute(unsigned int amountOfObjectsToDraw, Model* objectsToDraw, Spotlight& sceneLight);
private:
	Framebuffer gBuffer;

	Shader cameraGeometryPassShader;
	Shader cameraLightingPassShader;
};
