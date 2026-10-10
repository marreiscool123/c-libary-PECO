#pragma once

#include <cstdint>
#include <vector>

#include <peco\color.hpp>
#include <peco\platform.hpp>
#include <peco\camera.hpp>
#include <peco\mesh.hpp>

namespace peco {

class Renderer3D {
public:
    void setClearColor(Color color) { clearColor_ = color; }
    void render(HDC deviceContext, int width, int height,
                const Mesh& mesh, const Camera& camera);

private:
    void rasterizeTriangle(const float* v0, const float* v1, const float* v2,
                           Color color, int width, int height);

    Color clearColor_{24, 28, 36};
    std::vector<std::uint32_t> colorBuffer_;
    std::vector<float> depthBuffer_;
};

} // namespace peco
