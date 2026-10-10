#pragma once

#include <string>

#include <peco\image.hpp>
#include <peco\result.hpp>

namespace peco {

struct ImageLoadOptions {
    int maximumWidth = 16384;
    int maximumHeight = 16384;
    bool flipVertically = false;
    bool premultiplyAlpha = false;
};

class ImageLoader {
public:
    static Result<Image, std::string> load(const std::wstring& path,
                                           const ImageLoadOptions& options = {});
    static Result<void, std::string> saveBmp(const std::wstring& path,
                                             const Image& image);
    static bool supportsExtension(const std::wstring& path);
};

} // namespace peco
