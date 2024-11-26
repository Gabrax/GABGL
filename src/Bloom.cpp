#include "Bloom.h"


#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include "Input/Input.h"
#include "Renderer.h"
#include "Window.h"


static unsigned int quadVAO = 0;
static unsigned int quadVBO;
static void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}


// bloom stuff
struct bloomMip
{
	glm::vec2 size;
	glm::ivec2 intSize;
	unsigned int texture;
};


class bloomFBO
{
public:
	bloomFBO();
	~bloomFBO();
	bool Init(unsigned int windowWidth, unsigned int windowHeight, unsigned int mipChainLength);
	void Destroy();
	void BindForWriting();
	const std::vector<bloomMip>& MipChain() const;
  void Resize(unsigned int windowWidth, unsigned int windowHeight, unsigned int mipChainLength);

private:
  bool mInit;
  unsigned int mFBO;
  std::vector<bloomMip> mMipChain;
};


bloomFBO::bloomFBO() : mInit(false) {}
bloomFBO::~bloomFBO() {}

bool bloomFBO::Init(unsigned int windowWidth, unsigned int windowHeight, unsigned int mipChainLength)
{
	if (mInit) return true;

	glGenFramebuffers(1, &mFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

	glm::vec2 mipSize((float)windowWidth, (float)windowHeight);
	glm::ivec2 mipIntSize((int)windowWidth, (int)windowHeight);
	// Safety check
	if (windowWidth > (unsigned int)INT_MAX || windowHeight > (unsigned int)INT_MAX) {
		std::cerr << "Window size conversion overflow - cannot build bloom FBO!" << std::endl;
		return false;
	}

	for (GLuint i = 0; i < mipChainLength; i++)
	{
		bloomMip mip;

		mipSize *= 0.5f;
		mipIntSize /= 2;
		mip.size = mipSize;
		mip.intSize = mipIntSize;

		glGenTextures(1, &mip.texture);
		glBindTexture(GL_TEXTURE_2D, mip.texture);
		// we are downscaling an HDR color buffer, so we need a float texture format
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F,
		             (int)mipSize.x, (int)mipSize.y,
		             0, GL_RGB, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		std::cout << "Created bloom mip " << mipIntSize.x << 'x' << mipIntSize.y << std::endl;
		mMipChain.emplace_back(mip);
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_2D, mMipChain[0].texture, 0);

	// setup attachments
	unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	// check completion status
	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("gbuffer FBO error, status: 0x%x\n", status);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	mInit = true;
	return true;
}

void bloomFBO::Destroy()
{
	for (int i = 0; i < (int)mMipChain.size(); i++) {
		glDeleteTextures(1, &mMipChain[i].texture);
		mMipChain[i].texture = 0;
	}
	glDeleteFramebuffers(1, &mFBO);
	mFBO = 0;
	mInit = false;
}

void bloomFBO::BindForWriting()
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
}

const std::vector<bloomMip>& bloomFBO::MipChain() const
{
	return mMipChain;
}

void bloomFBO::Resize(unsigned int windowWidth, unsigned int windowHeight, unsigned int mipChainLength)
{
    // Clear the existing mip chain
    for (int i = 0; i < (int)mMipChain.size(); i++) {
        glDeleteTextures(1, &mMipChain[i].texture);
        mMipChain[i].texture = 0;
    }
    mMipChain.clear(); // Clear the vector to start fresh

    glm::vec2 mipSize((float)windowWidth, (float)windowHeight);
    glm::ivec2 mipIntSize((int)windowWidth, (int)windowHeight);

    for (GLuint i = 0; i < mipChainLength; i++)
    {
        bloomMip mip;

        mipSize *= 0.5f;
        mipIntSize /= 2;
        mip.size = mipSize;
        mip.intSize = mipIntSize;

        glGenTextures(1, &mip.texture);
        glBindTexture(GL_TEXTURE_2D, mip.texture);
        // we are downscaling an HDR color buffer, so we need a float texture format
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F,
                     (int)mipSize.x, (int)mipSize.y,
                     0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        std::cout << "Resized bloom mip " << mipIntSize.x << 'x' << mipIntSize.y << std::endl;
        mMipChain.emplace_back(mip);
    }

    // Reattach the first mip texture as the framebuffer attachment
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, mMipChain[0].texture, 0);

    unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments);

    // Check framebuffer completeness
    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("gbuffer FBO error after resize, status: 0x%x\n", status);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


static bloomFBO mFBO;
static glm::ivec2 mSrcViewportSize;
static glm::vec2 mSrcViewportSizeFloat;

static Shader& mDownsampleShader = Renderer::g_shaders.bloom_downsample; 
static Shader& mUpsampleShader = Renderer::g_shaders.bloom_upsample;
static Shader& shaderBloomFinal = Renderer::g_shaders.bloom_final;

bool BloomRenderer::Init()
{
	if (mInit) return true;

  glGenFramebuffers(1, &hdrFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
  // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
  glGenTextures(2, colorBuffers);
  for (unsigned int i = 0; i < 2; i++)
  {
      glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Window::GetWindowWidth(), Window::GetWindowHeight(), 0, GL_RGBA, GL_FLOAT, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      // attach texture to framebuffer
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
  }
  // create and attach depth buffer (renderbuffer)
  glGenRenderbuffers(1, &rboDepth);
  glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Window::GetWindowWidth(), Window::GetWindowHeight());
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
  
  // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
  unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
  glDrawBuffers(2, attachments);
  // finally check if framebuffer is complete
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      std::cout << "Framebuffer not complete!" << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // ping-pong-framebuffer for blurring
  glGenFramebuffers(2, pingpongFBO);
  glGenTextures(2, pingpongColorbuffers);
  for (unsigned int i = 0; i < 2; i++)
  {
      glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
      glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Window::GetWindowWidth(), Window::GetWindowHeight(), 0, GL_RGBA, GL_FLOAT, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
      // also check if framebuffers are complete (no need for depth buffer)
      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
          std::cout << "Framebuffer not complete!" << std::endl;
  }

	mSrcViewportSize = glm::ivec2(Window::GetWindowWidth(), Window::GetWindowHeight());
	mSrcViewportSizeFloat = glm::vec2((float)Window::GetWindowWidth(), (float)Window::GetWindowHeight());

	// Framebuffer
	const unsigned int num_bloom_mips = 6; // TODO: Play around with this value
	bool status = mFBO.Init(Window::GetWindowWidth(), Window::GetWindowHeight(), num_bloom_mips);
	if (!status) {
		std::cerr << "Failed to initialize bloom FBO - cannot create bloom renderer!\n";
		return false;
	}


	// Downsample
  mDownsampleShader.Use();
  mDownsampleShader.setInt("srcTexture", 0);
  glUseProgram(0);

  // Upsample
  mUpsampleShader.Use();
  mUpsampleShader.setInt("srcTexture", 0);
  glUseProgram(0);

  return true;
}

void BloomRenderer::Destroy()
{
	mFBO.Destroy();
	Renderer::g_shaders.bloom_downsample.Delete();
	Renderer::g_shaders.bloom_upsample.Delete();
}

void BloomRenderer::RenderDownsamples(unsigned int srcTexture)
{
	const std::vector<bloomMip>& mipChain = mFBO.MipChain();

	mDownsampleShader.Use();
	mDownsampleShader.setVec2("srcResolution", mSrcViewportSizeFloat);
	if (mKarisAverageOnDownsample) {
		mDownsampleShader.setInt("mipLevel", 0);
	}

	// Bind srcTexture (HDR color buffer) as initial texture input
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, srcTexture);

	// Progressively downsample through the mip chain
	for (int i = 0; i < (int)mipChain.size(); i++)
	{
		const bloomMip& mip = mipChain[i];
		glViewport(0, 0, mip.size.x, mip.size.y);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		                       GL_TEXTURE_2D, mip.texture, 0);

		// Render screen-filled quad of resolution of current mip
		renderQuad();

		// Set current mip resolution as srcResolution for next iteration
		mDownsampleShader.setVec2("srcResolution", mip.size);
		// Set current mip as texture input for next iteration
		glBindTexture(GL_TEXTURE_2D, mip.texture);
		// Disable Karis average for consequent downsamples
		if (i == 0) { mDownsampleShader.setInt("mipLevel", 1); }
	}

	glUseProgram(0);
}

void BloomRenderer::RenderUpsamples(float filterRadius)
{
	const std::vector<bloomMip>& mipChain = mFBO.MipChain();

	mUpsampleShader.Use();
	mUpsampleShader.setFloat("filterRadius", filterRadius);

	// Enable additive blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	for (int i = (int)mipChain.size() - 1; i > 0; i--)
	{
		const bloomMip& mip = mipChain[i];
		const bloomMip& nextMip = mipChain[i-1];

		// Bind viewport and texture from where to read
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mip.texture);

		// Set framebuffer render target (we write to this texture)
		glViewport(0, 0, nextMip.size.x, nextMip.size.y);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		                       GL_TEXTURE_2D, nextMip.texture, 0);

		// Render screen-filled quad of resolution of current mip
		renderQuad();
	}

	// Disable additive blending
	/*glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);*/
	glDisable(GL_BLEND);

	glUseProgram(0);
}

void BloomRenderer::RenderBloomTexture(float filterRadius)
{
	mFBO.BindForWriting();

	this->RenderDownsamples(colorBuffers[1]);
	this->RenderUpsamples(filterRadius);

  if(Input::KeyPressed(KEY_F)){
    Resize(Window::GetWindowWidth(),Window::GetWindowHeight());
  }
}

void BloomRenderer::Resize(int newWidth, int newHeight) {
    // Update viewport
    glViewport(0, 0, newWidth, newHeight);

    // Resize the HDR framebuffers and color buffers
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    for (unsigned int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, newWidth, newHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    }

    // Resize the depth renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, newWidth, newHeight);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete after resizing!" << '\n';
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BloomRenderer::Render()
{
  shaderBloomFinal.Use();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
  shaderBloomFinal.setInt("scene", 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, getBloomTexture());
  shaderBloomFinal.setInt("bloomBlur", 1);

  shaderBloomFinal.setFloat("renderWidth", Window::GetWindowWidth());
  shaderBloomFinal.setFloat("renderHeight", Window::GetWindowHeight());

  renderQuad();
}

void BloomRenderer::Bind() const 
{
  glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
}

void BloomRenderer::UnBind() const 
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Restore viewport
	glViewport(0, 0, Window::GetWindowWidth(), Window::GetWindowHeight());
}

GLuint BloomRenderer::getBloomTexture()
{
	return mFBO.MipChain()[0].texture;
}

GLuint BloomRenderer::BloomMip_i(int index)
{
	const std::vector<bloomMip>& mipChain = mFBO.MipChain();
	int size = (int)mipChain.size();
	return mipChain[(index > size-1) ? size-1 : (index < 0) ? 0 : index].texture;
}

