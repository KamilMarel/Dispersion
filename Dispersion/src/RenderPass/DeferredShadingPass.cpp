#include "RenderPass/DeferredShadingPass.h"

DeferredShadingPass::DeferredShadingPass(unsigned int windowWidth, unsigned int windowHeight) : 
	RenderPass(windowWidth, windowHeight), 
	gBuffer(windowWidth, windowHeight),
	cameraGeometryPassShader("shaders/RenderPass/cameraGeometryPass.vert", "shaders/RenderPass/cameraGeometryPass.frag"),
	cameraLightingPassShader("shaders/RenderPass/cameraLightingPass.vert", "shaders/RenderPass/cameraLightingPass.frag")
{
	gBuffer.addTextureColorAttachment(); // position
	gBuffer.addTextureColorAttachment(); // normals
	gBuffer.addTextureColorAttachment(); // albedo

	cameraGeometryPassShader.use();
	cameraGeometryPassShader.setInt("texture_diffuse1", 0);

	cameraLightingPassShader.use();
	cameraLightingPassShader.setInt("gPosition", 0);
	cameraLightingPassShader.setInt("gNormal", 1);
	cameraLightingPassShader.setInt("gAlbedo", 2);
	cameraLightingPassShader.setInt("shadowMap", 3);
	cameraLightingPassShader.setInt("causticMap", 4);
}

DeferredShadingPass::~DeferredShadingPass()
{
}

void DeferredShadingPass::execute(unsigned int amountOfObjectsToDraw, Model* objectsToDraw, Spotlight& sceneLight)
{

}