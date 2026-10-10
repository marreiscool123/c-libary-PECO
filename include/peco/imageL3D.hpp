#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include <peco\image_loader.hpp>

namespace peco {

enum class TextureAddressMode {
    Clamp,
    Repeat,
    Mirror
};

enum class TextureFilter {
    Nearest,
    Bilinear,
    Trilinear
};

struct TextureSampler {
    TextureAddressMode addressU = TextureAddressMode::Repeat;
    TextureAddressMode addressV = TextureAddressMode::Repeat;
    TextureFilter filter = TextureFilter::Trilinear;
    float lodBias = 0.0f;
    float minimumLod = 0.0f;
    float maximumLod = 1000.0f;
};

class ImageL3D {
public:
    ImageL3D() = default;
    explicit ImageL3D(Image image) { setImage(std::move(image)); }

    static Result<ImageL3D, std::string> load(
        const std::wstring& path, const ImageLoadOptions& options = {})
    {
        auto result = ImageLoader::load(path, options);
        if (!result) {
            return Result<ImageL3D, std::string>::failure(result.error());
        }
        ImageL3D texture{std::move(result).value()};
        texture.generateMipmaps();
        return Result<ImageL3D, std::string>::success(std::move(texture));
    }

    bool loadFromFile(const std::wstring& path,
                      const ImageLoadOptions& options = {})
    {
        auto result = ImageLoader::load(path, options);
        if (!result) {
            lastError_ = result.error();
            return false;
        }
        setImage(std::move(result).value());
        generateMipmaps();
        lastError_.clear();
        return true;
    }

    void setImage(Image image)
    {
        levels_.clear();
        if (!image.empty()) {
            levels_.push_back(std::move(image));
        }
    }
    const Image& image() const
    {
        static const Image emptyImage;
        return levels_.empty() ? emptyImage : levels_.front();
    }
    Image& image()
    {
        if (levels_.empty()) {
            levels_.emplace_back();
        }
        levels_.resize(1);
        return levels_.front();
    }
    const Image& mipLevel(std::size_t level) const
    {
        static const Image emptyImage;
        return level < levels_.size() ? levels_[level] : emptyImage;
    }
    std::size_t mipLevelCount() const { return levels_.size(); }
    int width() const { return image().width(); }
    int height() const { return image().height(); }
    bool empty() const { return image().empty(); }
    const TextureSampler& sampler() const { return sampler_; }
    TextureSampler& sampler() { return sampler_; }
    const std::string& lastError() const { return lastError_; }
    void setSampler(const TextureSampler& sampler) { sampler_ = sampler; }

    void generateMipmaps()
    {
        if (levels_.empty() || levels_.front().empty()) {
            levels_.clear();
            return;
        }
        levels_.resize(1);
        while (levels_.back().width() > 1 || levels_.back().height() > 1) {
            const Image& previous = levels_.back();
            const int nextWidth = std::max(1, previous.width() / 2
                + previous.width() % 2);
            const int nextHeight = std::max(1, previous.height() / 2
                + previous.height() % 2);
            Image next{nextWidth, nextHeight, Color::transparent()};
            for (int y = 0; y < nextHeight; ++y) {
                for (int x = 0; x < nextWidth; ++x) {
                    unsigned int red = 0;
                    unsigned int green = 0;
                    unsigned int blue = 0;
                    unsigned int alpha = 0;
                    unsigned int count = 0;
                    for (int dy = 0; dy < 2; ++dy) {
                        for (int dx = 0; dx < 2; ++dx) {
                            const int sourceX = x * 2 + dx;
                            const int sourceY = y * 2 + dy;
                            if (!previous.contains(sourceX, sourceY)) {
                                continue;
                            }
                            const Color sample = previous.at(sourceX, sourceY);
                            red += sample.r;
                            green += sample.g;
                            blue += sample.b;
                            alpha += sample.a;
                            ++count;
                        }
                    }
                    if (count != 0) {
                        next.at(x, y) = {
                            static_cast<std::uint8_t>((red + count / 2u) / count),
                            static_cast<std::uint8_t>((green + count / 2u) / count),
                            static_cast<std::uint8_t>((blue + count / 2u) / count),
                            static_cast<std::uint8_t>((alpha + count / 2u) / count)
                        };
                    }
                }
            }
            levels_.push_back(std::move(next));
        }
    }

    Color sample(const Vec2& uv, float lod = 0.0f) const
    {
        if (levels_.empty() || !uv.isFinite() || !std::isfinite(lod)) {
            return Color::transparent();
        }
        const float maximum = std::min(
            sampler_.maximumLod,
            static_cast<float>(levels_.size() - 1));
        const float clampedLod = std::clamp(lod + sampler_.lodBias,
            sampler_.minimumLod, std::max(sampler_.minimumLod, maximum));
        if (sampler_.filter != TextureFilter::Trilinear) {
            const std::size_t level = sampler_.filter == TextureFilter::Nearest
                ? static_cast<std::size_t>(std::lround(clampedLod))
                : static_cast<std::size_t>(std::floor(clampedLod + 0.5f));
            return sampleLevel(std::min(level, levels_.size() - 1), uv,
                               sampler_.filter == TextureFilter::Bilinear);
        }
        const std::size_t lower = static_cast<std::size_t>(std::floor(clampedLod));
        const std::size_t upper = std::min(lower + 1, levels_.size() - 1);
        const float fraction = clampedLod - static_cast<float>(lower);
        return Color::lerp(sampleLevel(lower, uv, true),
                           sampleLevel(upper, uv, true), fraction);
    }

    Color sampleLevel(std::size_t level, const Vec2& uv, bool bilinear) const
    {
        if (level >= levels_.size() || levels_[level].empty()) {
            return Color::transparent();
        }
        const Image& source = levels_[level];
        const float u = address(uv.x, sampler_.addressU);
        const float v = address(uv.y, sampler_.addressV);
        if (!bilinear) {
            const int x = std::clamp(static_cast<int>(u * source.width()),
                                     0, source.width() - 1);
            const int y = std::clamp(static_cast<int>(v * source.height()),
                                     0, source.height() - 1);
            return source.at(x, y);
        }
        const float x = u * static_cast<float>(source.width()) - 0.5f;
        const float y = v * static_cast<float>(source.height()) - 0.5f;
        const int x0 = static_cast<int>(std::floor(x));
        const int y0 = static_cast<int>(std::floor(y));
        const float tx = x - static_cast<float>(x0);
        const float ty = y - static_cast<float>(y0);
        const Color top = Color::lerp(fetch(source, x0, y0),
                                      fetch(source, x0 + 1, y0), tx);
        const Color bottom = Color::lerp(fetch(source, x0, y0 + 1),
                                          fetch(source, x0 + 1, y0 + 1), tx);
        return Color::lerp(top, bottom, ty);
    }

private:
    static float address(float value, TextureAddressMode mode)
    {
        switch (mode) {
        case TextureAddressMode::Clamp:
            return std::clamp(value, 0.0f, 1.0f);
        case TextureAddressMode::Repeat:
            return value - std::floor(value);
        case TextureAddressMode::Mirror: {
            float wrapped = std::fmod(value, 2.0f);
            if (wrapped < 0.0f) {
                wrapped += 2.0f;
            }
            return wrapped <= 1.0f ? wrapped : 2.0f - wrapped;
        }
        }
        return value;
    }
    int addressIndex(int value, int extent, TextureAddressMode mode) const
    {
        if (extent <= 1) {
            return 0;
        }
        if (mode == TextureAddressMode::Clamp) {
            return std::clamp(value, 0, extent - 1);
        }
        if (mode == TextureAddressMode::Repeat) {
            const int remainder = value % extent;
            return remainder < 0 ? remainder + extent : remainder;
        }
        const int period = extent * 2;
        int remainder = value % period;
        if (remainder < 0) {
            remainder += period;
        }
        return remainder < extent ? remainder : period - remainder - 1;
    }
    Color fetch(const Image& image, int x, int y) const
    {
        return image.at(addressIndex(x, image.width(), sampler_.addressU),
                        addressIndex(y, image.height(), sampler_.addressV));
    }

    std::vector<Image> levels_;
    TextureSampler sampler_;
    std::string lastError_;
};

} // namespace peco
