#include "RenderPass/DeferredShadingPass.h"

DeferredShadingPass::DeferredShadingPass(unsigned int windowWidth, unsigned int windowHeight) : 
	RenderPass(windowWidth, windowHeight), 
	gBuffer(windowWidth, windowHeight),
	resultBuffer(windowWidth, windowHeight),
	cameraGeometryPassShader("shaders/RenderPass/DeferredShadingPass/cameraGeometryPass.vert", "shaders/RenderPass/DeferredShadingPass/cameraGeometryPass.frag"),
	cameraLightingPassShader("shaders/RenderPass/DeferredShadingPass/cameraLightingPass.vert", "shaders/RenderPass/DeferredShadingPass/cameraLightingPass.frag")
{
	gBuffer.addTextureColorAttachment(); // position
	gBuffer.addTextureColorAttachment(); // normals
	gBuffer.addTextureColorAttachment(); // albedo

	resultBuffer.addTextureColorAttachment();

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

void DeferredShadingPass::execute(SceneGraphNode* sceneRoot, const glm::mat4& cameraView, const glm::mat4& cameraProjection, const Spotlight& sceneLight)
{
	gBuffer.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	cameraGeometryPassShader.use();
	cameraGeometryPassShader.setMat4("view", cameraView);
	cameraGeometryPassShader.setMat4("projection", cameraProjection);
	sceneRoot->render(glm::mat4(1.0f), true, cameraGeometryPassShader);
	gBuffer.unbind();

	resultBuffer.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer.getTextureColorAttachments()[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gBuffer.getTextureColorAttachments()[1]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gBuffer.getTextureColorAttachments()[2]);
	cameraLightingPassShader.use();
	cameraLightingPassShader.setVec3("light.position", sceneLight.position);
	cameraLightingPassShader.setVec3("light.direction", sceneLight.direction);
	cameraLightingPassShader.setVec3("light.color", sceneLight.color);
	cameraLightingPassShader.setFloat("light.innerCutoff", sceneLight.innerCutoff);
	cameraLightingPassShader.setFloat("light.outerCutoff", sceneLight.outerCutoff);
	cameraLightingPassShader.setFloat("light.attenuationConstant", sceneLight.attenuationConstant);
	cameraLightingPassShader.setFloat("light.attenuationLinear", sceneLight.attenuationLinear);
	cameraLightingPassShader.setFloat("light.attenuationQuadratic", sceneLight.attenuationQuadratic);
	cameraLightingPassShader.setMat4("viewPos", cameraView);
	renderWindowQuad();
	resultBuffer.unbind();
}

Framebuffer& DeferredShadingPass::getResult()
{
	return resultBuffer;
}