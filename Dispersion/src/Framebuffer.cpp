#include "Framebuffer.h"

Framebuffer::Framebuffer(unsigned int attachmentsWidth, unsigned int attachmentsHeight) : attachmentsWidth(attachmentsWidth), attachmentsHeight(attachmentsHeight)
{
	glGenFramebuffers(1, &ID);
}

Framebuffer::~Framebuffer()
{
	glDeleteFramebuffers(1, &ID);
	glDeleteTextures(textureColorAttachments.size(), textureColorAttachments.data());
}

void Framebuffer::addTextureColorAttachment()
{
	glBindFramebuffer(GL_FRAMEBUFFER, ID);

	textureColorAttachments.push_back(0);
	glGenTextures(1, &textureColorAttachments.back());
	unsigned int newTextureColorAttachment = textureColorAttachments.back();
	
	glBindTexture(GL_TEXTURE_2D, newTextureColorAttachment);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, attachmentsWidth, attachmentsHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + textureColorAttachments.size() - 1, GL_TEXTURE_2D, newTextureColorAttachment, 0);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

const std::vector<unsigned int>& Framebuffer::getTextureColorAttachments() const
{
	return textureColorAttachments;
}
