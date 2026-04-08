#include <glad/glad.h>
#include <vector>

class Framebuffer
{
public:
	Framebuffer(unsigned int attachmentsWidth,
				unsigned int attachmentsHeight);
	~Framebuffer();

	void bind();
	void unbind();
	void addTextureColorAttachment();
	const std::vector<unsigned int>& getTextureColorAttachments() const;
	unsigned int getID();
private:
	unsigned int ID;
	const unsigned int attachmentsWidth, attachmentsHeight;
	std::vector<unsigned int> textureColorAttachments;
	std::vector<unsigned int> colorAttachmentsToDrawTo;
};

