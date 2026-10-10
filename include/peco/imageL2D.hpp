#pragma once

#include <string>
#include <utility>

#include <peco\canvas2d.hpp>
#include <peco\image_loader.hpp>

namespace peco {

class ImageL2D {
public:
    ImageL2D() = default;
    explicit ImageL2D(Image image) : image_(std::move(image)) {}

    static Result<ImageL2D, std::string> load(
        const std::wstring& path, const ImageLoadOptions& options = {})
    {
        auto result = ImageLoader::load(path, options);
        if (!result) {
            return Result<ImageL2D, std::string>::failure(result.error());
        }
        return Result<ImageL2D, std::string>::success(
            ImageL2D{std::move(result).value()});
    }

    bool loadFromFile(const std::wstring& path,
                      const ImageLoadOptions& options = {})
    {
        auto result = ImageLoader::load(path, options);
        if (!result) {
            lastError_ = result.error();
            return false;
        }
        image_ = std::move(result).value();
        lastError_.clear();
        return true;
    }

    bool saveBmp(const std::wstring& path) const
    {
        auto result = ImageLoader::saveBmp(path, image_);
        if (!result) {
            lastError_ = result.error();
            return false;
        }
        lastError_.clear();
        return true;
    }

    const Image& image() const { return image_; }
    Image& image() { return image_; }
    const std::string& lastError() const { return lastError_; }
    int width() const { return image_.width(); }
    int height() const { return image_.height(); }
    bool empty() const { return image_.empty(); }

    bool draw(Canvas2D& canvas, const Rect& destination,
              float opacity = 1.0f) const;

private:
    Image image_;
    mutable std::string lastError_;
};

} // namespace peco
