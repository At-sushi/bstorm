﻿#pragma once

#include <bstorm/font_params.hpp>
#include <bstorm/non_copyable.hpp>
#include <bstorm/color_rgb.hpp>
#include <bstorm/cache_store.hpp>

#include <windows.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <d3d9.h>

namespace bstorm
{
class GraphicDevice;
class Font : private NonCopyable
{
public:
    Font(const FontParams& params, HWND hWnd, const std::shared_ptr<GraphicDevice>& graphicDevice);
    ~Font();
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetTextureWidth() const { return textureWidth_; }
    int GetTextureHeight() const { return textureHeight_; }
    int GetPrintOffsetX() const { return printOffsetX_; }
    int GetPrintOffsetY() const { return printOffsetY_; }
    int GetRightCharOffsetX() const { return rightCharOffsetX_; }
    int GetNextLineOffsetY() const { return nextLineOffsetY_; }
    IDirect3DTexture9* GetTexture() const { return texture_; }
    const FontParams& GetParams() const { return params_; }
private:
    FontParams params_;
    IDirect3DTexture9 *texture_; // セル(文字画像)を格納したテクスチャ, 綺麗に出力できるように高さと幅は2のn乗に揃える
    int width_;  // セルの幅
    int height_; // セルの高さ
    /* セルの左上を置いた位置から描画開始位置までの距離 */
    int printOffsetX_;
    int printOffsetY_;
    int rightCharOffsetX_; // 右に来る文字のまでの距離
    int nextLineOffsetY_; // 次の行までの距離
    int textureWidth_; // テクスチャの幅(2のn乗)
    int textureHeight_; // テクスチャの高さ(2のn乗)
};

class FontStore
{
public:
    FontStore(HWND hWnd, const std::shared_ptr<GraphicDevice>& graphicDevice);
    const std::shared_ptr<Font>& Create(const FontParams& params);
    bool Contains(const FontParams& params) const;
    void RemoveUnusedFont();
    template <class Fn>
    void ForEach(Fn func) { cacheStore_.ForEach(func); }
private:
    HWND hWnd_;
    const std::shared_ptr<GraphicDevice> graphicDevice_;
    CacheStore<FontParams, Font> cacheStore_;
};

bool InstallFont(const std::wstring& path);
}