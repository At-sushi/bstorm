﻿#pragma once

#include <bstorm/rect.hpp>

#include <memory>
#include <vector>

namespace bstorm
{
class Package;
class Texture;
class Font;
class RenderTarget;
class ResourceMonitor
{
public:
    ResourceMonitor(int left, int top, int width, int height);
    ~ResourceMonitor();
    void draw(const std::shared_ptr<Package>& package);
    bool isOpened() const { return openFlag; }
    void setOpen(bool b) { openFlag = b; }
private:
    int iniLeft;
    int iniTop;
    int iniWidth;
    int iniHeight;
    bool openFlag;
};

void DrawTextureInfo(const std::shared_ptr<Texture>& texture, const std::vector<Rect<int>>& rects, bool* reserved);
void DrawFontInfo(const std::shared_ptr<Font>& font);
void DrawRenderTargetInfo(const std::shared_ptr<RenderTarget>& renderTarget, const std::vector<Rect<int>>& rects);
}