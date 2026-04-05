#include <glad/glad.h>
#include <vector>

class Framebuffer
{
public:
	Framebuffer(unsigned int attachmentsWidth,
				unsigned int attachmentsHeight);
	~Framebuffer();

	void addTextureColorAttachment();
	const std::vector<unsigned int>& getTextureColorAttachments() const;
private:
	unsigned int ID;
	const unsigned int attachmentsWidth, attachmentsHeight;
	std::vector<unsigned int> textureColorAttachments;
};

