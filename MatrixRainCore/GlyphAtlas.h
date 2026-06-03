#pragma once




using Microsoft::WRL::ComPtr;




struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;




////////////////////////////////////////////////////////////////////////////////
//
//  GlyphAtlas
//
//  Owns the per-device GPU resources for the rain and overlay glyph atlases.
//  The device-independent glyph layout (UV coordinates, advance widths) lives
//  in the CharacterSet singleton; this holds only the textures and views that
//  are bound to a single D3D11 device.  Each per-monitor RenderSystem owns one
//  GlyphAtlas built on its own device, so an SRV is never bound to a device
//  that did not create it.  CharacterSet is a friend so it can populate the
//  resources while keeping them immutable to everyone else.
//
////////////////////////////////////////////////////////////////////////////////

class GlyphAtlas
{
public:
    ID3D11ShaderResourceView * RainResourceView()    const { return m_rainSRV.Get();    }
    ID3D11ShaderResourceView * OverlayResourceView() const { return m_overlaySRV.Get(); }

    void Reset()
    {
        m_rainSRV.Reset();
        m_rainTexture.Reset();
        m_overlaySRV.Reset();
        m_overlayTexture.Reset();
    }

private:
    friend class CharacterSet;

    ComPtr<ID3D11Texture2D>          m_rainTexture;
    ComPtr<ID3D11ShaderResourceView> m_rainSRV;
    ComPtr<ID3D11Texture2D>          m_overlayTexture;
    ComPtr<ID3D11ShaderResourceView> m_overlaySRV;
};
