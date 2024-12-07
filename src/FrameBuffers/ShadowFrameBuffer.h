#pragma once

struct ShadowFrameBuffer
{
	void Init(); 
	void CleanUp();
	void Clear();

    unsigned int m_ID = 0;
    unsigned int m_depthTexture = 0;
};
