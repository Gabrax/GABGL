#pragma once

#include <vector>

//#undef INFINITE
//#include "msdf-atlas-gen.h"
//#include "../Backend/BackendScopeRef.h"
//#include "Texture.h"
//
//struct MSDFData
//{
//	std::vector<msdf_atlas::GlyphGeometry> Glyphs;
//	msdf_atlas::FontGeometry FontGeometry;
//};
//
//class Font
//{
//public:
//	Font(const std::filesystem::path& font);
//	~Font();
//
//	const MSDFData* GetMSDFData() const { return m_Data; }
//	Ref<Texture> GetAtlasTexture() const { return m_AtlasTexture; }
//
//	static Ref<Font> GetDefault();
//private:
//	MSDFData* m_Data;
//	Ref<Texture> m_AtlasTexture;
//};