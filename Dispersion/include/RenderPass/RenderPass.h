#include "Framebuffer.h"
#include "Model.h"
#include "Spotlight.h"

class RenderPass
{
public:
	RenderPass(unsigned int windowWidth, unsigned int windowHeight);
	~RenderPass();

	virtual void execute(unsigned int amountOfObjectsToDraw, Model* objectsToDraw, Spotlight& sceneLight) = 0;

protected:
	unsigned int windowWidth, windowHeight;
};

