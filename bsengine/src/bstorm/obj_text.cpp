#include <bstorm/obj_text.hpp>

#include <bstorm/dnh_const.hpp>
#include <bstorm/string_util.hpp>
#include <bstorm/dx_util.hpp>
#include <bstorm/vertex.hpp>
#include <bstorm/font.hpp>
#include <bstorm/texture.hpp>
#include <bstorm/renderer.hpp>
#include <bstorm/package.hpp>

#include <algorithm>

#undef CreateFont

namespace bstorm
{
ObjText::ObjText(const std::shared_ptr<Package>& state) :
    ObjRender(state),
    fontName_(L"ＭＳ ゴシック"),
    size_(20),
    isBold_(false),
    topColor_(0xff, 0xff, 0xff),
    bottomColor_(0xff, 0xff, 0xff),
    borderType_(BORDER_NONE),
    borderWidth_(0),
    borderColor_(0xff, 0xff, 0xff),
    maxWidth_(INT_MAX),
    maxHeight_(INT_MAX),
    linePitch_(4),
    sidePitch_(0),
    transCenterX_(0),
    transCenterY_(0),
    autoTransCenterEnable_(true),
    horizontalAlignment_(ALIGNMENT_LEFT),
    syntacticAnalysisEnable_(true),
    isFontParamModified_(true)
{
    SetType(OBJ_TEXT);
}

ObjText::~ObjText()
{
}

void ObjText::Update() {}

void ObjText::GenerateFonts()
{
    if (isFontParamModified_)
    {
        if (auto package = GetPackage().lock())
        {
            bodyFonts_.clear();
            rubyFonts_.clear();
            for (wchar_t c : bodyText_)
            {
                if (c == L'\n')
                {
                    bodyFonts_.push_back(nullptr);
                } else
                {
                    bodyFonts_.push_back(package->CreateFont(FontParams(fontName_, size_, isBold_ ? FW_BOLD : FW_DONTCARE, topColor_, bottomColor_, borderType_, borderWidth_, borderColor_, c)));
                }
            }
            for (const Ruby<std::wstring>& ruby : rubies_)
            {
                std::vector<std::shared_ptr<Font>> fonts;
                for (wchar_t c : ruby.text)
                {
                    fonts.push_back(package->CreateFont(FontParams(fontName_, size_ / 2, FW_BOLD, topColor_, bottomColor_, borderType_, borderWidth_ / 2, borderColor_, c)));
                }
                rubyFonts_.emplace_back(ruby.begin, ruby.end, fonts);
            }
        }
        isFontParamModified_ = false;
    }
}

const std::vector<std::shared_ptr<Font>>& ObjText::GetBodyFonts() const
{
    return bodyFonts_;
}

int ObjText::GetNextLineOffsetY() const
{
    auto it = std::find_if(bodyFonts_.begin(), bodyFonts_.end(), [](const std::shared_ptr<Font>& font) { return font; });
    if (it == bodyFonts_.end()) return 0;
    auto& font = *it;
    return font->GetNextLineOffsetY() + linePitch_;
}

void ObjText::RenderFont(const std::shared_ptr<Font>& font, const D3DXMATRIX& world, const std::shared_ptr<Renderer>& renderer)
{
    std::array<Vertex, 4> vertices;
    D3DCOLOR color = GetD3DCOLOR();
    for (auto& vertex : vertices) { vertex.color = color; }
    vertices[1].u = vertices[3].u = 1.0f * font->GetWidth() / font->GetTextureWidth();
    vertices[2].v = vertices[3].v = 1.0f * font->GetHeight() / font->GetTextureHeight();

    vertices[0].x = vertices[2].x = 0;
    vertices[0].y = vertices[1].y = 0;
    vertices[1].x = vertices[3].x = font->GetWidth();
    vertices[2].y = vertices[3].y = font->GetHeight();

    renderer->RenderPrim2D(D3DPT_TRIANGLESTRIP, 4, vertices.data(), font->GetTexture(), GetBlendType(), world, GetAppliedShader(), IsPermitCamera(), true);
}

void ObjText::SetText(const std::wstring& t)
{
    if (text_ == t) return;
    text_ = t;
    if (syntacticAnalysisEnable_)
    {
        ParseRubiedString(t, bodyText_, rubies_);
    } else
    {
        bodyText_ = t;
        rubies_.clear();
    }
    isFontParamModified_ = true;
}

const std::wstring & ObjText::GetFontName() const
{
    return fontName_;
}

void ObjText::SetFontName(const std::wstring& name)
{
    if (fontName_ == name) return;
    fontName_ = name;
    isFontParamModified_ = true;
}

int ObjText::GetFontSize() const
{
    return size_;
}

void ObjText::SetFontSize(int s)
{
    if (size_ == s) return;
    size_ = s;
    isFontParamModified_ = true;
}

bool ObjText::IsFontBold() const
{
    return isBold_;
}

void ObjText::SetFontBold(bool b)
{
    if (isBold_ == b) return;
    isBold_ = b;
    isFontParamModified_ = true;
}

const ColorRGB & ObjText::GetFontColorTop() const
{
    return topColor_;
}

void ObjText::SetFontColorTop(int r, int g, int b)
{
    ColorRGB color(r, g, b);
    if (topColor_ == color) return;
    topColor_ = color;
    isFontParamModified_ = true;
}

const ColorRGB & ObjText::GetFontColorBottom() const
{
    return bottomColor_;
}

void ObjText::SetFontColorBottom(int r, int g, int b)
{
    ColorRGB color(r, g, b);
    if (bottomColor_ == color) return;
    bottomColor_ = color;
    isFontParamModified_ = true;
}

int ObjText::GetFontBorderType() const
{
    return borderType_;
}

void ObjText::SetFontBorderType(int t)
{
    if (borderType_ == t) return;
    borderType_ = t;
    isFontParamModified_ = true;
}

int ObjText::GetFontBorderWidth() const
{
    return borderWidth_;
}

void ObjText::SetFontBorderWidth(int w)
{
    if (borderWidth_ == w) return;
    borderWidth_ = w;
    isFontParamModified_ = true;
}

const ColorRGB & ObjText::GetFontBorderColor() const
{
    return borderColor_;
}

void ObjText::SetFontBorderColor(int r, int g, int b)
{
    ColorRGB color(r, g, b);
    if (borderColor_ == color) return;
    borderColor_ = color;
    isFontParamModified_ = true;
}

int ObjText::GetLinePitch() const
{
    return linePitch_;
}

int ObjText::GetSidePitch() const
{
    return sidePitch_;
}

bool ObjText::IsAutoTransCenterEnabled() const
{
    return autoTransCenterEnable_;
}

bool ObjText::IsSyntacticAnalysisEnabled() const
{
    return syntacticAnalysisEnable_;
}

void ObjText::SetSyntacticAnalysis(bool enable)
{
    if (syntacticAnalysisEnable_ != enable)
    {
        syntacticAnalysisEnable_ = enable;
        SetText(text_);
    }
}

int ObjText::GetHorizontalAlignment() const
{
    return horizontalAlignment_;
}

void ObjText::SetHorizontalAlignment(int alignmentType)
{
    horizontalAlignment_ = alignmentType;
}

float ObjText::GetTransCenterX() const
{
    return transCenterX_;
}

float ObjText::GetTransCenterY() const
{
    return transCenterY_;
}

int ObjText::GetMaxWidth() const
{
    return maxWidth_;
}

int ObjText::GetMaxHeight() const
{
    return maxHeight_;
}

void ObjText::Render(const std::shared_ptr<Renderer>& renderer)
{
    if (auto package = GetPackage().lock())
    {
        GenerateFonts();
        int idx = 0;
        float lineY = GetY(); // 現在の行のy座標
        const int centerX = GetX() + (autoTransCenterEnable_ ? std::max(0, (GetTotalWidth() - sidePitch_) / 2) : transCenterX_);
        const int centerY = GetY() + (autoTransCenterEnable_ ? GetTotalHeight() / 2 + (borderType_ != BORDER_NONE ? borderWidth_ / 2 : 0) : transCenterY_);
        const int nextLineOffsetY = GetNextLineOffsetY();
        auto ruby = rubyFonts_.begin();
        for (auto cnt : GetTextLengthCUL())
        { // 行ごとの文字数を取得
            float colX = GetX(); // 現在の列のx座標
            if (cnt != 0)
            {
                if (!bodyFonts_[idx]) idx++;
                if (horizontalAlignment_ != ALIGNMENT_LEFT)
                {
                    // alignment
                    int lineBodyWidth = 0;
                    for (int k = 0; k < cnt; k++)
                    {
                        lineBodyWidth += bodyFonts_[idx + k]->GetRightCharOffsetX() + sidePitch_;
                    }
                    if (borderType_ != BORDER_NONE)
                    {
                        lineBodyWidth += borderWidth_;
                    }
                    if (horizontalAlignment_ == ALIGNMENT_RIGHT)
                    {
                        colX += maxWidth_ - lineBodyWidth;
                    } else if (horizontalAlignment_ == ALIGNMENT_CENTER)
                    {
                        colX += (maxWidth_ - lineBodyWidth) / 2;
                    }
                }
                for (int k = 0; k < cnt; k++)
                {
                    auto bodyFont = bodyFonts_[idx];
                    // 初めに中心座標を原点に持ってきてから拡大・回転したのち元の位置に戻す
                    D3DXMATRIXA16 trans = CreateScaleRotTransMatrix(colX + bodyFont->GetPrintOffsetX() - centerX, lineY + bodyFont->GetPrintOffsetY() - centerY, GetZ(), 0, 0, 0, 1, 1, 1);
                    D3DXMATRIXA16 scaleRot = CreateScaleRotTransMatrix(centerX, centerY, 0, GetAngleX(), GetAngleY(), GetAngleZ(), GetScaleX(), GetScaleY(), GetScaleZ());
                    D3DXMATRIXA16 world = trans * scaleRot;
                    RenderFont(bodyFont, world, renderer);
                    if (ruby != rubyFonts_.end())
                    {
                        if (ruby->begin == idx)
                        {
                            if (!ruby->text.empty())
                            {
                                int bodyWidth = 0;
                                for (int i = ruby->begin; i < ruby->end && i < bodyFonts_.size(); i++)
                                {
                                    if (bodyFonts_[i])
                                    {
                                        bodyWidth += bodyFonts_[i]->GetRightCharOffsetX() + sidePitch_;
                                    }
                                }
                                int rubyOffsetSum = 0;
                                for (auto& font : ruby->text)
                                {
                                    rubyOffsetSum += font->GetRightCharOffsetX();
                                }
                                const int rubySidePitch = std::max(0, (bodyWidth - rubyOffsetSum)) / (ruby->text.size());
                                const int rubyY = lineY - ruby->text[0]->GetNextLineOffsetY();
                                float rubyX = colX;
                                for (auto rubyFont : ruby->text)
                                {
                                    D3DXMATRIXA16 trans = CreateScaleRotTransMatrix(rubyX + rubyFont->GetPrintOffsetX() - centerX, rubyY + rubyFont->GetPrintOffsetY() - centerY, GetZ(), 0, 0, 0, 1, 1, 1);
                                    D3DXMATRIXA16 world = trans * scaleRot;
                                    RenderFont(rubyFont, world, renderer);
                                    rubyX += rubyFont->GetRightCharOffsetX() + rubySidePitch;
                                }
                            }
                            ruby++;
                        }
                    }
                    colX += bodyFont->GetRightCharOffsetX() + sidePitch_; // 文字の間隔空け
                    idx++;
                }
            } else idx++;
            lineY += nextLineOffsetY; // 行の間隔空け
        }
    }
}

const std::wstring & ObjText::GetText() const
{
    return text_;
}

int ObjText::GetTotalWidth() const
{
    int idx = 0;
    int maxTotalWidth = 0;
    for (auto cnt : GetTextLengthCUL())
    {
        int totalWidth = 0;
        if (cnt != 0)
        {
            if (!bodyFonts_[idx]) idx++;
            for (int k = 0; k < cnt; k++)
            {
                totalWidth += bodyFonts_[idx]->GetRightCharOffsetX() + sidePitch_;
                idx++;
            }
            if (borderType_ != BORDER_NONE)
            {
                totalWidth += borderWidth_;
            }
            maxTotalWidth = std::max(maxTotalWidth, totalWidth);
        } else idx++;
    }
    return maxTotalWidth;
}

int ObjText::GetTotalHeight() const
{
    if (bodyFonts_.empty())
    {
        return 0;
    } else
    {
        int lineCnt = GetTextLengthCUL().size(); // 行数
        // NULLでないフォントを探す
        int h = lineCnt * GetNextLineOffsetY() - linePitch_;
        if (borderType_ != BORDER_NONE) h += borderWidth_;
        return h;
    }
}

int ObjText::GetTextLength() const
{
    int cnt = 0;
    for (wchar_t c : text_)
    {
        if ((int)c < 256)
        {
            cnt++;
        } else
        {
            cnt += 2;
        }
    }
    return cnt;
}

int ObjText::GetTextLengthCU() const
{
    int cnt = 0;
    for (wchar_t c : bodyText_)
    {
        if (c != L'\n')
        {
            cnt++;
        }
    }
    return cnt;
}

std::vector<int> ObjText::GetTextLengthCUL() const
{
    std::vector<int> cnts;
    int idx = 0;
    int cnt = 0;
    int sumWidth = 0;
    while (idx < bodyFonts_.size())
    {
        if (!bodyFonts_[idx])
        {
            // 改行文字
            cnts.push_back(cnt);
            cnt = 0;
            sumWidth = 0;
        } else
        {
            int charWidth = bodyFonts_[idx]->GetRightCharOffsetX() + sidePitch_;
            sumWidth += charWidth;
            if (maxWidth_ <= sumWidth)
            {
                // 幅に収まらずに改行
                cnts.push_back(cnt);
                // 行に1文字も入らない場合は1文字分飛ばす
                if (cnt == 0) idx++;
                cnt = 0;
                sumWidth = 0;
                continue;
            }
            cnt++;
        }
        idx++;
    }
    if (cnt != 0) cnts.push_back(cnt);
    return cnts;
}

bool ObjText::IsFontParamModified() const
{
    return isFontParamModified_;
}

// FUTURE: パーサの引数に返り値を持たせないと使いにくいので直す(要求度：低)

static bool parseChar(const std::wstring& src, int& i, wchar_t c)
{
    if (i < src.size() && src[i] == c)
    {
        i++;
        return true;
    }
    return false;
}

static bool parseString(const std::wstring& src, int& i, const std::wstring& s)
{
    if (i < src.size() && s == src.substr(i, s.size()))
    {
        i += s.size();
        return true;
    }
    return false;
}

static bool parseNewLine(const std::wstring& src, int& i)
{
    return parseString(src, i, L"[r]");
}

static void skipSpace(const std::wstring& src, int& i)
{
    while (i < src.size() && IsSpace(src[i])) i++;
}

static bool parseRuby(const std::wstring& src, int& i, std::wstring& rb, std::wstring& rt)
{
    int prevIdx = i;
    if (!parseString(src, i, L"[ruby")) goto parse_failed;
    while (i < src.size() && src[i] != L']')
    {
        skipSpace(src, i);
        std::wstring propName;
        if (parseString(src, i, L"rb"))
        {
            propName = L"rb";
        } else if (parseString(src, i, L"rt"))
        {
            propName = L"rt";
        } else goto parse_failed;
        skipSpace(src, i);
        if (!parseChar(src, i, L'=')) goto parse_failed;
        skipSpace(src, i);
        if (!parseChar(src, i, L'"')) goto parse_failed;
        std::wstring& dst = (propName == L"rb") ? rb : rt;
        while (i < src.size() && src[i] != L'"')
        {
            if (parseString(src, i, L"&nbsp;"))
            {
                dst += L" ";
            } else if (parseString(src, i, L"&quot;"))
            {
                dst += L"\"";
            } else if (parseString(src, i, L"&osb;"))
            {
                dst += L"[";
            } else if (parseString(src, i, L"&csb;"))
            {
                dst += L"]";
            } else
            {
                dst += src[i++];
            }
        }
        if (!parseChar(src, i, L'"')) goto parse_failed;
        skipSpace(src, i);
        parseChar(src, i, L','); // カンマをセパレータにできる。(NOTE: 本家は他にも書けるが未対応
        skipSpace(src, i);
    }
    if (!parseChar(src, i, L']')) goto parse_failed;
    return true;
parse_failed:
    i = prevIdx;
    rb.clear();
    rt.clear();
    return false;
}

const std::vector<ObjText::Ruby<std::vector<std::shared_ptr<Font>>>>& ObjText::GetRubyFonts() const
{
    return rubyFonts_;
}

void ObjText::ParseRubiedString(const std::wstring& src, std::wstring& bodyText, std::vector<Ruby<std::wstring>>& rubies)
{
    bodyText.clear();
    rubies.clear();
    for (int i = 0; i < src.size();)
    {
        if (parseNewLine(src, i))
        {
            bodyText += L'\n';
        } else if (parseString(src, i, L"&nbsp;"))
        {
            bodyText += L" ";
        } else if (parseString(src, i, L"&quot;"))
        {
            bodyText += L"\"";
        } else if (parseString(src, i, L"&osb;"))
        {
            bodyText += L"[";
        } else if (parseString(src, i, L"&csb;"))
        {
            bodyText += L"]";
        } else
        {
            std::wstring rb;
            std::wstring rt;
            if (parseRuby(src, i, rb, rt))
            {
                rubies.emplace_back(bodyText.size(), bodyText.size() + rb.size(), rt);
                bodyText += rb;
            } else
            {
                // 改行は除去
                if (src[i] != L'\r' && src[i] != '\n') bodyText += src[i];
                i++;
            }
        }
    }
}
}