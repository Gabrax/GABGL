#pragma once

#include "BackendLogger.h"
#include <initializer_list>
#include <vector>
#include <cstdint>

enum class FramebufferTextureFormat
{
	None = 0,

	// Color
	RGBA8,
	RED_INTEGER,

	// Depth/stencil
	DEPTH24STENCIL8,

	// Defaults
	Depth = DEPTH24STENCIL8
};

struct FramebufferTextureSpecification
{
	FramebufferTextureSpecification() = default;
	FramebufferTextureSpecification(FramebufferTextureFormat format)
		: TextureFormat(format) {}

	FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
	// TODO: filtering/wrap
};

struct FramebufferAttachmentSpecification
{
	FramebufferAttachmentSpecification() = default;
	FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
		: Attachments(attachments) {}

	std::vector<FramebufferTextureSpecification> Attachments;
};

struct FramebufferSpecification
{
	uint32_t Width = 0, Height = 0;
	FramebufferAttachmentSpecification Attachments;
	uint32_t Samples = 1;

	bool SwapChainTarget = false;
};

struct Framebuffer
{
	Framebuffer(const FramebufferSpecification& spec);
	~Framebuffer();

	void Invalidate();

	void Bind();
	void Unbind();

	void Resize(uint32_t width, uint32_t height);
	int ReadPixel(uint32_t attachmentIndex, int x, int y);

	void ClearAttachment(uint32_t attachmentIndex, int value);

	inline uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const { GABGL_ASSERT(index < m_ColorAttachments.size(),""); return m_ColorAttachments[index]; }

	inline const FramebufferSpecification& GetSpecification() const { return m_Specification; };

	static std::shared_ptr<Framebuffer> Create(const FramebufferSpecification& spec);

private:
	uint32_t m_RendererID = 0;
	FramebufferSpecification m_Specification;

	std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
	FramebufferTextureSpecification m_DepthAttachmentSpecification = FramebufferTextureFormat::None;

	std::vector<uint32_t> m_ColorAttachments;
	uint32_t m_DepthAttachment = 0;
};
