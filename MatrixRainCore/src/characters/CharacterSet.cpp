#include "pch.h"
#include "matrixrain/CharacterSet.h"
#include <random>

namespace MatrixRain
{
    CharacterSet::CharacterSet()
        : m_textureResource(nullptr)
        , m_initialized(false)
    {
    }

    CharacterSet::~CharacterSet()
    {
        Shutdown();
    }

    CharacterSet& CharacterSet::GetInstance()
    {
        static CharacterSet instance;
        return instance;
    }

    bool CharacterSet::Initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        // TODO: Phase 3 will implement texture atlas creation
        // For now, just mark as initialized
        m_initialized = true;
        return true;
    }

    size_t CharacterSet::GetRandomGlyphIndex() const
    {
        if (m_glyphs.empty())
        {
            return 0;
        }

        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, m_glyphs.size() - 1);
        return dist(gen);
    }

    const GlyphInfo& CharacterSet::GetGlyph(size_t index) const
    {
        static GlyphInfo defaultGlyph;
        if (index >= m_glyphs.size())
        {
            return defaultGlyph;
        }
        return m_glyphs[index];
    }

    size_t CharacterSet::GetGlyphCount() const
    {
        return m_glyphs.size();
    }

    void* CharacterSet::GetTextureResource() const
    {
        return m_textureResource;
    }

    void CharacterSet::Shutdown()
    {
        m_glyphs.clear();
        m_textureResource = nullptr;
        m_initialized = false;
    }

    bool CharacterSet::CreateTextureAtlas()
    {
        // TODO: Phase 3 - DirectX texture creation
        return false;
    }

    bool CharacterSet::RenderGlyphsToAtlas()
    {
        // TODO: Phase 3 - Direct2D/DirectWrite glyph rendering
        return false;
    }

    void CharacterSet::CalculateUVCoordinates()
    {
        // TODO: Phase 3 - Calculate UV coordinates for each glyph
    }
}
