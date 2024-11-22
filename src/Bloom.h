#pragma once

#include <cstdint>

struct BloomRenderer
{

	BloomRenderer() : mInit(false) { Init(); }
	~BloomRenderer() = default;
	bool Init();
	void Destroy();
	void RenderBloomTexture(float filterRadius);
	unsigned int BloomTexture();
	unsigned int BloomMip_i(int index);
  void Render();
  uint32_t getFBO() const {
    return hdrFBO;
  }

private:
  uint32_t hdrFBO, colorBuffers[2], rboDepth, pingpongFBO[2], pingpongColorbuffers[2];
  
	void RenderDownsamples(unsigned int srcTexture);
	void RenderUpsamples(float filterRadius);

  bool mInit;
	bool mKarisAverageOnDownsample = true;
};
