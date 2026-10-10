#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <utility>
#include <vector>

#include <peco\camera.hpp>
#include <peco\canvas2d.hpp>
#include <peco\color.hpp>
#include <peco\image.hpp>
#include <peco\imageL3D.hpp>
#include <peco\frustum.hpp>
#include <peco\lighting.hpp>
#include <peco\material.hpp>
#include <peco\mesh.hpp>
#include <peco\scene.hpp>
#include <peco\vector2.hpp>
#include <peco\vector3.hpp>

namespace peco {

enum class CullMode3D {
    None,
    Back,
    Front
};

enum class ToneMap3D {
    None,
    Reinhard,
    Aces
};

struct DirectionalLight3D {
    LightType type = LightType::Directional;
    Vec3 direction{-0.4f, 0.7f, 0.6f};
    Vec3 position{};
    Color color{255, 255, 255};
    float intensity = 0.8f;
    float range = 10.0f;
    float innerConeRadians = 0.35f;
    float outerConeRadians = 0.6f;
};

struct RenderOptions3D {
    Color clearColor{24, 28, 36};
    Color ambientColor{48, 48, 56};
    CullMode3D cullMode = CullMode3D::None;
    Vec3 lightDirection{-0.4f, 0.7f, 0.6f};
    float ambient = 0.2f;
    float diffuse = 0.8f;
    bool useMaterialColors = true;
    bool wireframe = false;
    Color wireColor{12, 12, 16};
    float wireWidth = 1.0f;
    bool blendTransparent = true;
    bool useTextureCoordinates = true;
};

struct RenderStatistics3D {
    std::size_t inputTriangles = 0;
    std::size_t clippedTriangles = 0;
    std::size_t rasterizedTriangles = 0;
    std::size_t testedPixels = 0;
    std::size_t shadedPixels = 0;
    std::size_t depthRejectedPixels = 0;
};

struct MeshInstance3D {
    const Mesh* mesh = nullptr;
    Transform3D transform;
    Color tint = Color::white();
    const ImageL3D* texture = nullptr;
    bool overrideMaterial = false;
    Material material;
};

struct MeshHit3D {
    std::size_t triangleIndex = 0;
    Vec3 position{};
    Vec3 normal{};
    Vec2 barycentric{};
    float distance = 0.0f;
    unsigned int materialIndex = 0;
};

class Renderer3DTarget {
public:
    Renderer3DTarget() = default;
    Renderer3DTarget(int width, int height) { resize(width, height); }

    bool resize(int width, int height)
    {
        if (width <= 0 || height <= 0) {
            return false;
        }
        const std::size_t w = static_cast<std::size_t>(width);
        const std::size_t h = static_cast<std::size_t>(height);
        if (w > std::numeric_limits<std::size_t>::max() / h) {
            return false;
        }
        Image next(width, height, options_.clearColor);
        std::vector<float> nextDepth(w * h,
            std::numeric_limits<float>::infinity());
        output_ = std::move(next);
        depth_ = std::move(nextDepth);
        return true;
    }
    bool ready() const { return !output_.empty() && depth_.size() == output_.pixelCount(); }
    int width() const { return output_.width(); }
    int height() const { return output_.height(); }
    const Image& image() const { return output_; }
    Image& image() { return output_; }
    const std::vector<float>& depthBuffer() const { return depth_; }
    const RenderOptions3D& options() const { return options_; }
    RenderOptions3D& options() { return options_; }
    void setOptions(const RenderOptions3D& value) { options_ = value; }
    const RenderStatistics3D& statistics() const { return statistics_; }
    void setDirectionalLights(std::vector<DirectionalLight3D> lights)
    {
        lights_ = std::move(lights);
    }
    const std::vector<DirectionalLight3D>& directionalLights() const { return lights_; }
    void setPixelShader(std::function<Color(Color, float, const Vec2&)> shader)
    {
        pixelShader_ = std::move(shader);
    }
    void clearPixelShader() { pixelShader_ = {}; }

    bool render(const Mesh& mesh, const Camera& camera,
                const ImageL3D* texture = nullptr)
    {
        if (!ready() || !mesh.isValid()) {
            return false;
        }
        beginFrame();
        return renderIntoCurrentFrame(mesh, camera, texture);
    }
    bool renderMesh(const Mesh& mesh, const Camera& camera,
                    const ImageL3D* texture = nullptr)
    {
        return render(mesh, camera, texture);
    }
    bool render(const Mesh& mesh, const Camera& camera,
                const ImageL3D& texture)
    {
        return render(mesh, camera, &texture);
    }
    bool renderCube(const Camera& camera, float size = 1.0f,
                    Color color = {220, 220, 230})
    {
        return render(Mesh::cube(size, color), camera);
    }
    bool renderPlane(const Camera& camera, float widthValue = 1.0f,
                     float depthValue = 1.0f,
                     Color color = {220, 220, 230})
    {
        return render(Mesh::plane(widthValue, depthValue, color), camera);
    }
    bool renderSphere(const Camera& camera, float radius = 1.0f,
                      unsigned int longitudeSegments = 24,
                      unsigned int latitudeSegments = 12,
                      Color color = {220, 220, 230})
    {
        return render(Mesh::uvSphere(radius, longitudeSegments,
                                     latitudeSegments, color), camera);
    }
    bool renderCylinder(const Camera& camera, float radius = 0.5f,
                       float heightValue = 1.0f, unsigned int segments = 24,
                       Color color = {220, 220, 230})
    {
        return render(Mesh::cylinder(radius, heightValue, segments, color), camera);
    }
    bool renderCone(const Camera& camera, float radius = 0.5f,
                    float heightValue = 1.0f, unsigned int segments = 24,
                    Color color = {220, 220, 230})
    {
        return render(Mesh::cone(radius, heightValue, segments, color), camera);
    }
    bool renderBatch(const std::vector<MeshInstance3D>& instances,
                     const Camera& camera)
    {
        if (!ready()) {
            return false;
        }
        beginFrame();
        for (const MeshInstance3D& instance : instances) {
            if (instance.mesh == nullptr || !instance.mesh->isValid()) {
                continue;
            }
            Mesh transformed = *instance.mesh;
            transformed.applyTransform(instance.transform);
            transformed.setColor(Color::multiply(transformed.color(), instance.tint));
            if (instance.overrideMaterial) {
                transformed.setMaterials({instance.material});
            }
            if (!renderIntoCurrentFrame(transformed, camera, instance.texture)) {
                return false;
            }
        }
        return true;
    }
    bool renderVisible(const std::vector<MeshInstance3D>& instances,
                       const Camera& camera)
    {
        if (!ready()) {
            return false;
        }
        const Mat4 matrix = camera.viewProjection(
            static_cast<float>(width()) / static_cast<float>(height()));
        const Frustum frustum = Frustum::fromViewProjection(matrix);
        std::vector<MeshInstance3D> visible;
        visible.reserve(instances.size());
        for (const MeshInstance3D& instance : instances) {
            if (instance.mesh == nullptr || !instance.mesh->isValid()) {
                continue;
            }
            const Bounds3 bounds = transformBounds(instance.mesh->bounds(),
                                                    instance.transform);
            if (frustum.intersects(bounds)) {
                visible.push_back(instance);
            }
        }
        return renderBatch(visible, camera);
    }
    bool renderScene(Scene& scene)
    {
        if (!ready()) {
            return false;
        }
        std::vector<MeshInstance3D> instances;
        instances.reserve(scene.nodeCount());
        const Mat4 viewProjection = scene.camera().viewProjection(
            static_cast<float>(width()) / static_cast<float>(height()));
        const Frustum frustum = Frustum::fromViewProjection(viewProjection);
        scene.forEachVisible([&instances, &frustum](SceneNode& node) {
            if (node.hasMesh()) {
                const Transform3D world = node.worldTransform();
                if (!frustum.intersects(transformBounds(node.mesh()->bounds(), world))) {
                    return;
                }
                instances.push_back({node.mesh().get(), world,
                                     Color::white(), nullptr, true, node.material()});
            }
        });
        std::vector<DirectionalLight3D> sceneLights;
        sceneLights.reserve(scene.lights().size());
        for (const Light& light : scene.lights()) {
            if (!light.enabled) {
                continue;
            }
            DirectionalLight3D converted;
            converted.type = light.type;
            converted.direction = light.direction;
            converted.position = light.position;
            converted.color = light.color;
            converted.intensity = light.intensity;
            converted.range = light.range;
            converted.innerConeRadians = light.innerConeRadians;
            converted.outerConeRadians = light.outerConeRadians;
            sceneLights.push_back(converted);
        }
        lights_ = std::move(sceneLights);
        options_.ambientColor = scene.ambientLight().color;
        options_.ambient = scene.ambientLight().intensity;
        return renderBatch(instances, scene.camera());
    }
    Ray3 rayFromPixel(const Vec2& pixel, const Camera& camera) const
    {
        return camera.rayFromScreen(pixel,
            {static_cast<float>(width()), static_cast<float>(height())});
    }
    std::vector<MeshHit3D> pickAll(const Mesh& mesh, const Camera& camera,
                                   const Vec2& pixel) const
    {
        std::vector<MeshHit3D> hits;
        if (!ready() || !mesh.isValid() || !pixel.isFinite()
            || pixel.x < 0.0f || pixel.x >= width()
            || pixel.y < 0.0f || pixel.y >= height()) {
            return hits;
        }
        const Ray3 ray = rayFromPixel(pixel, camera);
        hits.reserve(mesh.triangleCount());
        for (std::size_t index = 0; index < mesh.triangles().size(); ++index) {
            const Triangle& triangle = mesh.triangles()[index];
            float distance = 0.0f;
            float u = 0.0f;
            float v = 0.0f;
            if (triangle.a >= mesh.vertices().size()
                || triangle.b >= mesh.vertices().size()
                || triangle.c >= mesh.vertices().size()
                || !intersectTriangle(ray, mesh.vertices()[triangle.a],
                                      mesh.vertices()[triangle.b],
                                      mesh.vertices()[triangle.c],
                                      distance, u, v)) {
                continue;
            }
            const Vec3 face = (mesh.vertices()[triangle.b] - mesh.vertices()[triangle.a])
                .cross(mesh.vertices()[triangle.c] - mesh.vertices()[triangle.a]).normalized();
            MeshHit3D hit;
            hit.triangleIndex = index;
            hit.distance = distance;
            hit.position = ray.at(distance);
            hit.barycentric = {u, v};
            hit.normal = mesh.hasNormals()
                ? (mesh.normals()[triangle.a] * (1.0f - u - v)
                    + mesh.normals()[triangle.b] * u
                    + mesh.normals()[triangle.c] * v).normalized()
                : face;
            hit.materialIndex = triangle.materialIndex;
            hits.push_back(hit);
        }
        std::stable_sort(hits.begin(), hits.end(),
            [](const MeshHit3D& lhs, const MeshHit3D& rhs) {
                return lhs.distance < rhs.distance;
            });
        return hits;
    }
    void drawSphereWire(const Vec3& center, float radius, const Camera& camera,
                        Color color, unsigned int segments = 24,
                        float thickness = 1.0f)
    {
        if (!center.isFinite() || !std::isfinite(radius) || radius <= 0.0f
            || segments < 3) {
            return;
        }
        segments = std::min(segments, 512u);
        constexpr float tau = 6.28318530717958647692f;
        std::vector<Vec3> ring;
        ring.reserve(static_cast<std::size_t>(segments) + 1u);
        for (int plane = 0; plane < 3; ++plane) {
            ring.clear();
            for (unsigned int index = 0; index <= segments; ++index) {
                const float angle = tau * static_cast<float>(index)
                    / static_cast<float>(segments);
                const float x = std::cos(angle) * radius;
                const float y = std::sin(angle) * radius;
                if (plane == 0) {
                    ring.push_back(center + Vec3{x, y, 0.0f});
                } else if (plane == 1) {
                    ring.push_back(center + Vec3{x, 0.0f, y});
                } else {
                    ring.push_back(center + Vec3{0.0f, x, y});
                }
            }
            drawPolyline3D(ring, camera, color, thickness);
        }
    }
    void drawCameraGizmo(const Camera& camera, float axisLength = 0.25f,
                         float thickness = 1.0f)
    {
        if (axisLength <= 0.0f || !std::isfinite(axisLength)) {
            return;
        }
        const Vec3 origin = camera.position();
        drawLine(origin, origin + camera.right() * axisLength,
                 camera, {235, 70, 70}, thickness);
        drawLine(origin, origin + camera.correctedUp() * axisLength,
                 camera, {90, 220, 110}, thickness);
        drawLine(origin, origin + camera.forward() * axisLength,
                 camera, {80, 140, 245}, thickness);
    }
    void drawMeshNormals(const Mesh& mesh, const Camera& camera,
                         float length, Color color,
                         bool useVertexNormals = true,
                         float thickness = 1.0f)
    {
        if (!mesh.isValid() || !std::isfinite(length) || length <= 0.0f) {
            return;
        }
        if (useVertexNormals && mesh.hasNormals()) {
            for (std::size_t index = 0; index < mesh.vertices().size(); ++index) {
                drawLine(mesh.vertices()[index],
                    mesh.vertices()[index] + mesh.normals()[index].normalized() * length,
                    camera, color, thickness);
            }
            return;
        }
        for (const Triangle& triangle : mesh.triangles()) {
            if (triangle.a >= mesh.vertices().size()
                || triangle.b >= mesh.vertices().size()
                || triangle.c >= mesh.vertices().size()) {
                continue;
            }
            const Vec3 center = (mesh.vertices()[triangle.a]
                + mesh.vertices()[triangle.b] + mesh.vertices()[triangle.c]) / 3.0f;
            const Vec3 normal = (mesh.vertices()[triangle.b] - mesh.vertices()[triangle.a])
                .cross(mesh.vertices()[triangle.c] - mesh.vertices()[triangle.a]).normalized();
            drawLine(center, center + normal * length, camera, color, thickness);
        }
    }

    bool projectPoint(const Vec3& point, const Camera& camera,
                     Vec2& pixel, float* depth = nullptr) const
    {
        if (!ready() || !point.isFinite()) {
            return false;
        }
        const Mat4 viewProjection = camera.viewProjection(
            static_cast<float>(width()) / static_cast<float>(height()));
        const auto clip = viewProjection.transform(point.x, point.y, point.z, 1.0f);
        if (!std::isfinite(clip[3]) || clip[3] <= 0.0f) {
            return false;
        }
        const float inverseW = 1.0f / clip[3];
        const float ndcX = clip[0] * inverseW;
        const float ndcY = clip[1] * inverseW;
        const float ndcZ = clip[2] * inverseW;
        if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f
            || ndcZ < -1.0f || ndcZ > 1.0f) {
            return false;
        }
        pixel = {(ndcX * 0.5f + 0.5f) * (width() - 1),
                 (0.5f - ndcY * 0.5f) * (height() - 1)};
        if (depth != nullptr) {
            *depth = ndcZ * 0.5f + 0.5f;
        }
        return true;
    }
    bool unprojectPoint(const Vec2& pixel, float depth,
                       const Camera& camera, Vec3& world) const
    {
        if (!ready() || !pixel.isFinite() || !std::isfinite(depth)
            || pixel.x < 0.0f || pixel.x >= width()
            || pixel.y < 0.0f || pixel.y >= height()
            || depth < 0.0f || depth > 1.0f) {
            return false;
        }
        Mat4 inverse;
        if (!camera.viewProjection(
                static_cast<float>(width()) / static_cast<float>(height()))
                .inverse(inverse)) {
            return false;
        }
        const float x = width() > 1
            ? pixel.x / static_cast<float>(width() - 1) * 2.0f - 1.0f : 0.0f;
        const float y = height() > 1
            ? 1.0f - pixel.y / static_cast<float>(height() - 1) * 2.0f : 0.0f;
        const auto point = inverse.transform(x, y, depth * 2.0f - 1.0f, 1.0f);
        if (!std::isfinite(point[3]) || std::abs(point[3]) < 1.0e-12f) {
            return false;
        }
        world = {point[0] / point[3], point[1] / point[3], point[2] / point[3]};
        return world.isFinite();
    }
    bool pick(const Mesh& mesh, const Camera& camera, const Vec2& pixel,
              MeshHit3D& hit) const
    {
        if (!ready() || !mesh.isValid() || !pixel.isFinite()
            || pixel.x < 0.0f || pixel.x >= width()
            || pixel.y < 0.0f || pixel.y >= height()) {
            return false;
        }
        const Ray3 ray = camera.rayFromScreen(pixel,
            {static_cast<float>(width()), static_cast<float>(height())});
        float closest = std::numeric_limits<float>::infinity();
        bool found = false;
        for (std::size_t index = 0; index < mesh.triangles().size(); ++index) {
            const Triangle& triangle = mesh.triangles()[index];
            if (triangle.a >= mesh.vertices().size()
                || triangle.b >= mesh.vertices().size()
                || triangle.c >= mesh.vertices().size()) {
                continue;
            }
            float distance = 0.0f;
            float u = 0.0f;
            float v = 0.0f;
            if (!intersectTriangle(ray, mesh.vertices()[triangle.a],
                                   mesh.vertices()[triangle.b],
                                   mesh.vertices()[triangle.c],
                                   distance, u, v)
                || distance >= closest) {
                continue;
            }
            closest = distance;
            hit.triangleIndex = index;
            hit.distance = distance;
            hit.barycentric = {u, v};
            hit.position = ray.origin + ray.direction * distance;
            const Vec3 face = (mesh.vertices()[triangle.b] - mesh.vertices()[triangle.a])
                .cross(mesh.vertices()[triangle.c] - mesh.vertices()[triangle.a]).normalized();
            if (mesh.hasNormals()) {
                hit.normal = (mesh.normals()[triangle.a] * (1.0f - u - v)
                    + mesh.normals()[triangle.b] * u
                    + mesh.normals()[triangle.c] * v).normalized();
            } else {
                hit.normal = face;
            }
            hit.materialIndex = triangle.materialIndex;
            found = true;
        }
        return found;
    }
    void drawGrid(const Camera& camera, float size, unsigned int divisions,
                  Color color, float thickness = 1.0f)
    {
        if (size <= 0.0f || divisions == 0 || !std::isfinite(size)) {
            return;
        }
        divisions = std::min(divisions, 1024u);
        const float half = size * 0.5f;
        for (unsigned int index = 0; index <= divisions; ++index) {
            const float coordinate = -half + size * static_cast<float>(index)
                / static_cast<float>(divisions);
            drawLine({coordinate, 0.0f, -half}, {coordinate, 0.0f, half},
                     camera, color, thickness);
            drawLine({-half, 0.0f, coordinate}, {half, 0.0f, coordinate},
                     camera, color, thickness);
        }
    }
    void drawAxes(const Camera& camera, float length,
                  float thickness = 2.0f)
    {
        if (!std::isfinite(length) || length <= 0.0f) {
            return;
        }
        drawLine({}, {length, 0.0f, 0.0f}, camera, {235, 70, 70}, thickness);
        drawLine({}, {0.0f, length, 0.0f}, camera, {90, 220, 110}, thickness);
        drawLine({}, {0.0f, 0.0f, length}, camera, {80, 140, 245}, thickness);
    }
    void drawBounds(const Bounds3& bounds, const Camera& camera,
                    Color color, float thickness = 1.0f)
    {
        if (bounds.isEmpty()) {
            return;
        }
        const Vec3 minimum = bounds.minimum;
        const Vec3 maximum = bounds.maximum;
        const Vec3 corners[8]{
            {minimum.x, minimum.y, minimum.z}, {maximum.x, minimum.y, minimum.z},
            {maximum.x, maximum.y, minimum.z}, {minimum.x, maximum.y, minimum.z},
            {minimum.x, minimum.y, maximum.z}, {maximum.x, minimum.y, maximum.z},
            {maximum.x, maximum.y, maximum.z}, {minimum.x, maximum.y, maximum.z}
        };
        const unsigned int edges[12][2]{
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
            {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };
        for (const auto& edgeIndices : edges) {
            drawLine(corners[edgeIndices[0]], corners[edgeIndices[1]],
                     camera, color, thickness);
        }
    }
    bool present(Canvas2D& canvas, const Rect& destination,
                 float opacity = 1.0f) const
    {
        return ready() && canvas.drawImage(output_, destination, opacity);
    }
    bool getPixel(int x, int y, Color& color) const
    {
        return ready() && output_.getPixel(x, y, color);
    }
    bool getDepth(int x, int y, float& depth) const
    {
        if (!ready() || x < 0 || y < 0 || x >= width() || y >= height()) {
            return false;
        }
        depth = depth_[pixelIndex(x, y)];
        return std::isfinite(depth);
    }
    float depthAt(int x, int y) const
    {
        float depth = std::numeric_limits<float>::infinity();
        getDepth(x, y, depth);
        return depth;
    }
    Color colorAt(int x, int y) const
    {
        Color color = Color::transparent();
        output_.getPixel(x, y, color);
        return color;
    }
    bool setDepth(int x, int y, float depth)
    {
        if (!ready() || x < 0 || y < 0 || x >= width() || y >= height()
            || !std::isfinite(depth) || depth < 0.0f || depth > 1.0f) {
            return false;
        }
        depth_[pixelIndex(x, y)] = depth;
        return true;
    }
    bool testAndSetDepth(int x, int y, float depth, Color color)
    {
        if (!ready() || x < 0 || y < 0 || x >= width() || y >= height()
            || !std::isfinite(depth) || depth < 0.0f || depth > 1.0f) {
            return false;
        }
        const std::size_t index = pixelIndex(x, y);
        if (depth >= depth_[index]) {
            ++statistics_.depthRejectedPixels;
            return false;
        }
        depth_[index] = depth;
        output_.pixels()[index] = color;
        ++statistics_.shadedPixels;
        return true;
    }
    float linearizeDepth(float depth, const Camera& camera) const
    {
        if (!std::isfinite(depth) || depth < 0.0f || depth > 1.0f
            || !std::isfinite(camera.nearPlane())
            || !std::isfinite(camera.farPlane())
            || camera.nearPlane() <= 0.0f
            || camera.farPlane() <= camera.nearPlane()) {
            return std::numeric_limits<float>::infinity();
        }
        const float nearPlane = camera.nearPlane();
        const float farPlane = camera.farPlane();
        const float ndc = depth * 2.0f - 1.0f;
        const float denominator = farPlane + nearPlane
            - ndc * (farPlane - nearPlane);
        return std::abs(denominator) > 1.0e-8f
            ? (2.0f * nearPlane * farPlane) / denominator
            : std::numeric_limits<float>::infinity();
    }
    void drawPointCloud(const std::vector<Vec3>& points, const Camera& camera,
                        Color color, float size = 2.0f)
    {
        if (!ready() || !std::isfinite(size) || size <= 0.0f) {
            return;
        }
        const Mat4 matrix = camera.viewProjection(
            static_cast<float>(width()) / static_cast<float>(height()));
        for (const Vec3& point : points) {
            ClipVertex vertex;
            if (!point.isFinite()) {
                continue;
            }
            vertex.position = matrix.transform(point.x, point.y, point.z, 1.0f);
            bool inside = true;
            for (int plane = 0; plane < 6; ++plane) {
                inside = inside && planeDistance(vertex, plane) >= 0.0f;
            }
            if (inside) {
                const ScreenVertex projected = toScreen(vertex);
                drawDepthPoint(projected.x, projected.y,
                    projected.z * 0.5f + 0.5f, color, size);
            }
        }
    }
    void drawPolyline3D(const std::vector<Vec3>& points,
                       const Camera& camera, Color color,
                       float thickness = 1.0f, bool closed = false)
    {
        if (points.size() < 2 || !std::isfinite(thickness) || thickness <= 0.0f) {
            return;
        }
        for (std::size_t index = 1; index < points.size(); ++index) {
            drawLine(points[index - 1], points[index], camera, color, thickness);
        }
        if (closed) {
            drawLine(points.back(), points.front(), camera, color, thickness);
        }
    }
    bool drawWorldText(Canvas2D& canvas, const Vec3& position,
                       const Camera& camera, const std::wstring& text,
                       Color color, int textHeight = 18)
    {
        Vec2 pixel;
        float depth = 0.0f;
        if (!projectPoint(position, camera, pixel, &depth)) {
            return false;
        }
        float storedDepth = 0.0f;
        if (getDepth(static_cast<int>(pixel.x), static_cast<int>(pixel.y), storedDepth)
            && depth > storedDepth) {
            return false;
        }
        canvas.text(static_cast<int>(std::lround(pixel.x)),
                    static_cast<int>(std::lround(pixel.y)),
                    text, color, textHeight);
        return true;
    }
    void composite(const Image& overlay, float opacity = 1.0f)
    {
        if (!ready() || overlay.empty() || !std::isfinite(opacity)) {
            return;
        }
        const float factor = std::clamp(opacity, 0.0f, 1.0f);
        for (int y = 0; y < std::min(height(), overlay.height()); ++y) {
            for (int x = 0; x < std::min(width(), overlay.width()); ++x) {
                Color color = overlay.at(x, y);
                color.a = static_cast<std::uint8_t>(std::lround(color.a * factor));
                output_.at(x, y) = Color::alphaBlend(color, output_.at(x, y));
            }
        }
    }
    void applyGrayscale()
    {
        if (!ready()) {
            return;
        }
        for (Color& color : output_.pixels()) {
            color = color.grayscale();
        }
    }
    void applyInvert()
    {
        if (!ready()) {
            return;
        }
        for (Color& color : output_.pixels()) {
            color = color.inverted();
        }
    }
    void applySepia(float amount = 1.0f)
    {
        if (!ready() || !std::isfinite(amount)) {
            return;
        }
        amount = std::clamp(amount, 0.0f, 1.0f);
        for (Color& color : output_.pixels()) {
            const Color sepia{
                channel((0.393f * color.r + 0.769f * color.g + 0.189f * color.b) / 255.0f),
                channel((0.349f * color.r + 0.686f * color.g + 0.168f * color.b) / 255.0f),
                channel((0.272f * color.r + 0.534f * color.g + 0.131f * color.b) / 255.0f),
                color.a};
            color = Color::lerp(color, sepia, amount);
        }
    }
    void applyPosterize(unsigned int levels)
    {
        if (!ready() || levels < 2) {
            return;
        }
        const unsigned int steps = std::min(levels - 1u, 255u);
        const auto quantize = [steps](std::uint8_t value) {
            const unsigned int scaled = static_cast<unsigned int>(value) * steps;
            return static_cast<std::uint8_t>(((scaled + 127u) / 255u) * 255u / steps);
        };
        for (Color& color : output_.pixels()) {
            color.r = quantize(color.r);
            color.g = quantize(color.g);
            color.b = quantize(color.b);
        }
    }
    void applyFilmGrain(float strength, std::uint32_t seed)
    {
        if (!ready() || !std::isfinite(strength) || strength < 0.0f) {
            return;
        }
        strength = std::min(strength, 1.0f);
        std::uint32_t state = seed == 0 ? 0x6d2b79f5u : seed;
        for (Color& color : output_.pixels()) {
            state ^= state << 13u;
            state ^= state >> 17u;
            state ^= state << 5u;
            const int noise = static_cast<int>(state & 0xffffu) - 32768;
            const int delta = static_cast<int>(std::lround(
                static_cast<float>(noise) / 32768.0f * strength * 255.0f));
            color.r = clampChannel(static_cast<int>(color.r) + delta);
            color.g = clampChannel(static_cast<int>(color.g) + delta);
            color.b = clampChannel(static_cast<int>(color.b) + delta);
        }
    }
    void applySharpen(float amount = 0.5f)
    {
        if (!ready() || !std::isfinite(amount) || amount <= 0.0f) {
            return;
        }
        amount = std::min(amount, 4.0f);
        const Image source = output_;
        for (int y = 1; y + 1 < height(); ++y) {
            for (int x = 1; x + 1 < width(); ++x) {
                const Color center = source.at(x, y);
                const Color neighbors = averageColors({
                    source.at(x - 1, y), source.at(x + 1, y),
                    source.at(x, y - 1), source.at(x, y + 1)});
                const auto sharpen = [amount](std::uint8_t middle, std::uint8_t average) {
                    return clampChannel(static_cast<int>(std::lround(
                        middle + (static_cast<float>(middle) - average) * amount)));
                };
                output_.at(x, y) = {
                    sharpen(center.r, neighbors.r), sharpen(center.g, neighbors.g),
                    sharpen(center.b, neighbors.b), center.a};
            }
        }
    }
    void applyBoxBlur(unsigned int radius)
    {
        if (!ready() || radius == 0) {
            return;
        }
        output_ = blurredImage(output_, std::min(radius, 32u));
    }
    void applyMotionBlur(const Vec2& velocity, unsigned int samples = 8)
    {
        if (!ready() || !velocity.isFinite() || samples < 2) {
            return;
        }
        samples = std::min(samples, 64u);
        const Image source = output_;
        for (int y = 0; y < height(); ++y) {
            for (int x = 0; x < width(); ++x) {
                std::uint64_t red = 0, green = 0, blue = 0, alpha = 0;
                for (unsigned int sampleIndex = 0; sampleIndex < samples; ++sampleIndex) {
                    const float t = static_cast<float>(sampleIndex)
                        / static_cast<float>(samples - 1u) - 0.5f;
                    const int sx = std::clamp(static_cast<int>(std::lround(
                        x + velocity.x * t)), 0, width() - 1);
                    const int sy = std::clamp(static_cast<int>(std::lround(
                        y + velocity.y * t)), 0, height() - 1);
                    const Color color = source.at(sx, sy);
                    red += color.r;
                    green += color.g;
                    blue += color.b;
                    alpha += color.a;
                }
                output_.at(x, y) = {
                    static_cast<std::uint8_t>(red / samples),
                    static_cast<std::uint8_t>(green / samples),
                    static_cast<std::uint8_t>(blue / samples),
                    static_cast<std::uint8_t>(alpha / samples)};
            }
        }
    }
    void applyRadialBlur(const Vec2& center, float strength,
                         unsigned int samples = 8)
    {
        if (!ready() || !center.isFinite() || !std::isfinite(strength)
            || samples < 2) {
            return;
        }
        samples = std::min(samples, 64u);
        strength = std::clamp(strength, 0.0f, 1.0f);
        const Image source = output_;
        for (int y = 0; y < height(); ++y) {
            for (int x = 0; x < width(); ++x) {
                std::uint64_t red = 0, green = 0, blue = 0, alpha = 0;
                const Vec2 point{static_cast<float>(x), static_cast<float>(y)};
                const Vec2 toward = center - point;
                for (unsigned int index = 0; index < samples; ++index) {
                    const float t = strength * static_cast<float>(index)
                        / static_cast<float>(samples - 1u);
                    const int sx = std::clamp(static_cast<int>(std::lround(
                        point.x + toward.x * t)), 0, width() - 1);
                    const int sy = std::clamp(static_cast<int>(std::lround(
                        point.y + toward.y * t)), 0, height() - 1);
                    const Color color = source.at(sx, sy);
                    red += color.r;
                    green += color.g;
                    blue += color.b;
                    alpha += color.a;
                }
                output_.at(x, y) = {
                    static_cast<std::uint8_t>(red / samples),
                    static_cast<std::uint8_t>(green / samples),
                    static_cast<std::uint8_t>(blue / samples),
                    static_cast<std::uint8_t>(alpha / samples)};
            }
        }
    }
    void applyColorMatrix(const std::array<float, 20>& matrix)
    {
        if (!ready() || !std::all_of(matrix.begin(), matrix.end(),
                [](float value) { return std::isfinite(value); })) {
            return;
        }
        for (Color& color : output_.pixels()) {
            const float input[4]{color.r / 255.0f, color.g / 255.0f,
                                 color.b / 255.0f, color.a / 255.0f};
            float result[4]{};
            for (std::size_t row = 0; row < 4; ++row) {
                for (std::size_t column = 0; column < 4; ++column) {
                    result[row] += matrix[row * 5u + column] * input[column];
                }
                result[row] += matrix[row * 5u + 4u];
            }
            color = {channel(result[0]), channel(result[1]),
                     channel(result[2]), channel(result[3])};
        }
    }
    void applyDepthFog(const Camera& camera, Color fogColor,
                       float startDistance, float endDistance,
                       float density = 1.0f)
    {
        if (!ready() || !std::isfinite(startDistance) || !std::isfinite(endDistance)
            || !std::isfinite(density) || startDistance < 0.0f
            || endDistance <= startDistance || density < 0.0f) {
            return;
        }
        for (std::size_t index = 0; index < depth_.size(); ++index) {
            if (!std::isfinite(depth_[index])) {
                continue;
            }
            const float distance = linearizeDepth(depth_[index], camera);
            const float normalized = std::clamp(
                (distance - startDistance) / (endDistance - startDistance), 0.0f, 1.0f);
            const float amount = 1.0f - std::exp(-normalized * density);
            output_.pixels()[index] = Color::lerp(output_.pixels()[index],
                                                    fogColor, amount);
        }
    }
    void applyVibrance(float amount)
    {
        if (!ready() || !std::isfinite(amount)) {
            return;
        }
        amount = std::clamp(amount, -1.0f, 2.0f);
        for (Color& color : output_.pixels()) {
            const float red = color.r / 255.0f;
            const float green = color.g / 255.0f;
            const float blue = color.b / 255.0f;
            const float maximum = std::max({red, green, blue});
            const float minimum = std::min({red, green, blue});
            const float saturation = maximum - minimum;
            const float adjustment = amount * (1.0f - saturation);
            const float gray = (red + green + blue) / 3.0f;
            color.r = channel(red + (red - gray) * adjustment);
            color.g = channel(green + (green - gray) * adjustment);
            color.b = channel(blue + (blue - gray) * adjustment);
        }
    }
    void applyExposure(float stops)
    {
        if (!ready() || !std::isfinite(stops)) {
            return;
        }
        const float gain = std::exp2(std::clamp(stops, -16.0f, 16.0f));
        for (Color& color : output_.pixels()) {
            color = scaleRgb(color, gain);
        }
    }
    void applyGamma(float gamma)
    {
        if (!ready() || !std::isfinite(gamma) || gamma <= 0.0f) {
            return;
        }
        const float exponent = 1.0f / gamma;
        for (Color& color : output_.pixels()) {
            color.r = channel(std::pow(color.r / 255.0f, exponent));
            color.g = channel(std::pow(color.g / 255.0f, exponent));
            color.b = channel(std::pow(color.b / 255.0f, exponent));
        }
    }
    void applyToneMap(ToneMap3D mode = ToneMap3D::Reinhard)
    {
        if (!ready() || mode == ToneMap3D::None) {
            return;
        }
        for (Color& color : output_.pixels()) {
            const auto map = [mode](std::uint8_t value) {
                const float linear = value / 255.0f;
                float result = linear;
                if (mode == ToneMap3D::Reinhard) {
                    result = linear / (1.0f + linear);
                } else if (mode == ToneMap3D::Aces) {
                    result = (linear * (2.51f * linear + 0.03f))
                        / (linear * (2.43f * linear + 0.59f) + 0.14f);
                }
                return channel(result);
            };
            color.r = map(color.r);
            color.g = map(color.g);
            color.b = map(color.b);
        }
    }
    void applySaturation(float amount)
    {
        if (!ready() || !std::isfinite(amount)) {
            return;
        }
        amount = std::clamp(amount, 0.0f, 4.0f);
        for (Color& color : output_.pixels()) {
            const float red = color.r / 255.0f;
            const float green = color.g / 255.0f;
            const float blue = color.b / 255.0f;
            const float gray = red * 0.2126f + green * 0.7152f + blue * 0.0722f;
            color.r = channel(gray + (red - gray) * amount);
            color.g = channel(gray + (green - gray) * amount);
            color.b = channel(gray + (blue - gray) * amount);
        }
    }
    void applyColorTint(Color tint, float amount = 1.0f)
    {
        if (!ready() || !std::isfinite(amount)) {
            return;
        }
        const float factor = std::clamp(amount, 0.0f, 1.0f);
        for (Color& color : output_.pixels()) {
            const Color toned = Color::multiply(color, tint);
            color = Color::lerp(color, toned, factor);
        }
    }
    void applyFog(Color fogColor, float startDepth, float endDepth,
                  float density = 1.0f)
    {
        if (!ready() || !std::isfinite(startDepth) || !std::isfinite(endDepth)
            || !std::isfinite(density) || endDepth <= startDepth || density < 0.0f) {
            return;
        }
        for (std::size_t index = 0; index < depth_.size(); ++index) {
            if (!std::isfinite(depth_[index])) {
                continue;
            }
            const float depth = std::clamp(
                (depth_[index] - startDepth) / (endDepth - startDepth), 0.0f, 1.0f);
            const float fogAmount = 1.0f - std::exp(-depth * density);
            output_.pixels()[index] = Color::lerp(output_.pixels()[index],
                                                   fogColor, fogAmount);
        }
    }
    void applyVignette(float strength = 0.35f, float radius = 0.75f)
    {
        if (!ready() || !std::isfinite(strength) || !std::isfinite(radius)
            || radius <= 0.0f) {
            return;
        }
        strength = std::clamp(strength, 0.0f, 1.0f);
        const float aspect = static_cast<float>(width()) / height();
        for (int y = 0; y < height(); ++y) {
            for (int x = 0; x < width(); ++x) {
                const float nx = (2.0f * (x + 0.5f) / width() - 1.0f) * aspect;
                const float ny = 2.0f * (y + 0.5f) / height() - 1.0f;
                const float distance = std::sqrt(nx * nx + ny * ny);
                const float edge = std::clamp((distance - radius)
                    / std::max(1.0e-5f, 1.4143f - radius), 0.0f, 1.0f);
                const float factor = 1.0f - strength * edge * edge;
                const std::size_t index = pixelIndex(x, y);
                output_.pixels()[index] = scaleRgb(output_.pixels()[index], factor);
            }
        }
    }
    void applyAmbientOcclusion(float radius = 4.0f, float strength = 0.5f)
    {
        if (!ready() || !std::isfinite(radius) || !std::isfinite(strength)
            || radius <= 0.0f || strength < 0.0f) {
            return;
        }
        const std::vector<float> originalDepth = depth_;
        const Image originalColor = output_;
        const int sampleRadius = static_cast<int>(
            std::clamp(std::ceil(radius), 1.0f, 32.0f));
        for (int y = 0; y < height(); ++y) {
            for (int x = 0; x < width(); ++x) {
                const float center = originalDepth[pixelIndex(x, y)];
                if (!std::isfinite(center)) {
                    continue;
                }
                float occlusion = 0.0f;
                unsigned int samples = 0;
                for (int oy = -sampleRadius; oy <= sampleRadius; oy += 2) {
                    for (int ox = -sampleRadius; ox <= sampleRadius; ox += 2) {
                        if (ox == 0 && oy == 0) {
                            continue;
                        }
                        const int sx = x + ox;
                        const int sy = y + oy;
                        if (!originalColor.contains(sx, sy)) {
                            continue;
                        }
                        const float neighbor = originalDepth[pixelIndex(sx, sy)];
                        if (std::isfinite(neighbor)) {
                            occlusion += std::clamp((center - neighbor) * 8.0f,
                                                    0.0f, 1.0f);
                        }
                        ++samples;
                    }
                }
                if (samples != 0) {
                    const float factor = 1.0f - std::clamp(
                        occlusion / samples * strength, 0.0f, 0.95f);
                    output_.at(x, y) = scaleRgb(originalColor.at(x, y), factor);
                }
            }
        }
    }
    void applyDepthOfField(float focusDepth, float range,
                           unsigned int blurRadius = 3)
    {
        if (!ready() || !std::isfinite(focusDepth) || !std::isfinite(range)
            || range <= 0.0f) {
            return;
        }
        const Image blurred = blurredImage(output_, std::min(blurRadius, 16u));
        const Image original = output_;
        for (std::size_t index = 0; index < depth_.size(); ++index) {
            if (!std::isfinite(depth_[index])) {
                continue;
            }
            const float factor = std::clamp(
                std::abs(depth_[index] - focusDepth) / range, 0.0f, 1.0f);
            output_.pixels()[index] = Color::lerp(
                original.pixels()[index], blurred.pixels()[index], factor);
        }
    }
    void applyBloom(float threshold, float strength = 0.75f,
                    unsigned int blurRadius = 4)
    {
        if (!ready() || !std::isfinite(threshold) || !std::isfinite(strength)
            || strength < 0.0f) {
            return;
        }
        Image bright(width(), height(), Color::transparent());
        for (std::size_t index = 0; index < output_.pixelCount(); ++index) {
            const Color source = output_.pixels()[index];
            if (source.luminance() >= threshold) {
                bright.pixels()[index] = source;
            }
        }
        const Image glow = blurredImage(bright, std::min(blurRadius, 24u));
        for (std::size_t index = 0; index < output_.pixelCount(); ++index) {
            const Color glowPixel = scaleRgb(glow.pixels()[index], strength);
            output_.pixels()[index] = addColor(output_.pixels()[index], glowPixel);
        }
    }
    void applyChromaticAberration(float pixels)
    {
        if (!ready() || !std::isfinite(pixels) || pixels <= 0.0f) {
            return;
        }
        const Image source = output_;
        const int offset = std::clamp(static_cast<int>(std::lround(pixels)), 1, 32);
        for (int y = 0; y < height(); ++y) {
            for (int x = 0; x < width(); ++x) {
                const Color red = source.at(std::clamp(x - offset, 0, width() - 1), y);
                const Color center = source.at(x, y);
                const Color blue = source.at(std::clamp(x + offset, 0, width() - 1), y);
                output_.at(x, y) = {red.r, center.g, blue.b, center.a};
            }
        }
    }
    void applyFxaa(float contrastThreshold = 0.08f)
    {
        if (!ready() || !std::isfinite(contrastThreshold)
            || contrastThreshold < 0.0f) {
            return;
        }
        const Image source = output_;
        const auto luminance = [](Color color) {
            return color.luminance();
        };
        for (int y = 1; y + 1 < height(); ++y) {
            for (int x = 1; x + 1 < width(); ++x) {
                const Color center = source.at(x, y);
                const float north = luminance(source.at(x, y - 1));
                const float south = luminance(source.at(x, y + 1));
                const float west = luminance(source.at(x - 1, y));
                const float east = luminance(source.at(x + 1, y));
                const float minimum = std::min({center.luminance(), north, south, west, east});
                const float maximum = std::max({center.luminance(), north, south, west, east});
                if (maximum - minimum < contrastThreshold) {
                    continue;
                }
                Color horizontal = Color::lerp(source.at(x - 1, y),
                                                source.at(x + 1, y), 0.5f);
                Color vertical = Color::lerp(source.at(x, y - 1),
                                              source.at(x, y + 1), 0.5f);
                const Color candidate = std::abs(north - south) >= std::abs(west - east)
                    ? vertical : horizontal;
                output_.at(x, y) = Color::lerp(center, candidate, 0.5f);
            }
        }
    }
    bool drawDepthOutline(float threshold, Color outline)
    {
        if (!ready() || !std::isfinite(threshold) || threshold < 0.0f) {
            return false;
        }
        const std::vector<float> original = depth_;
        for (int y = 1; y + 1 < height(); ++y) {
            for (int x = 1; x + 1 < width(); ++x) {
                const float center = original[pixelIndex(x, y)];
                if (!std::isfinite(center)) {
                    continue;
                }
                bool edgeFound = false;
                for (const Vec2& direction : depthOffsets()) {
                    const int sx = x + static_cast<int>(direction.x);
                    const int sy = y + static_cast<int>(direction.y);
                    const float neighbor = original[pixelIndex(sx, sy)];
                    if (std::isfinite(neighbor)
                        && std::abs(center - neighbor) > threshold) {
                        edgeFound = true;
                        break;
                    }
                }
                if (edgeFound) {
                    output_.at(x, y) = Color::alphaBlend(outline, output_.at(x, y));
                }
            }
        }
        return true;
    }
    bool saveBmp(const std::wstring& path, std::string* error = nullptr) const
    {
        if (!ready()) {
            if (error != nullptr) {
                *error = "The 3D renderer has no completed image to save.";
            }
            return false;
        }
        const auto result = ImageLoader::saveBmp(path, output_);
        if (!result) {
            if (error != nullptr) {
                *error = result.error();
            }
            return false;
        }
        if (error != nullptr) {
            error->clear();
        }
        return true;
    }
    bool drawFrustum(const Camera& camera, Color color,
                     float thickness = 1.0f)
    {
        if (!ready()) {
            return false;
        }
        Mat4 inverse;
        if (!camera.viewProjection(
                static_cast<float>(width()) / static_cast<float>(height()))
                .inverse(inverse)) {
            return false;
        }
        const float coordinates[8][3]{
            {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f},
            {1.0f, 1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f},
            {-1.0f, -1.0f, 1.0f}, {1.0f, -1.0f, 1.0f},
            {1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}
        };
        Vec3 corners[8];
        for (std::size_t index = 0; index < 8; ++index) {
            const auto point = inverse.transform(coordinates[index][0],
                coordinates[index][1], coordinates[index][2], 1.0f);
            if (std::abs(point[3]) < 1.0e-12f) {
                return false;
            }
            corners[index] = {point[0] / point[3], point[1] / point[3],
                              point[2] / point[3]};
        }
        const unsigned int edges[12][2]{
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
            {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };
        for (const auto& edge : edges) {
            drawLine(corners[edge[0]], corners[edge[1]],
                     camera, color, thickness);
        }
        return true;
    }
    bool drawTriangleWire(const Vec3& a, const Vec3& b, const Vec3& c,
                          const Camera& camera, Color color,
                          float thickness = 1.0f)
    {
        if (!a.isFinite() || !b.isFinite() || !c.isFinite()) {
            return false;
        }
        drawLine(a, b, camera, color, thickness);
        drawLine(b, c, camera, color, thickness);
        drawLine(c, a, camera, color, thickness);
        return true;
    }

    void clear(Color color)
    {
        if (!ready()) {
            return;
        }
        output_.clear(color);
        std::fill(depth_.begin(), depth_.end(),
                  std::numeric_limits<float>::infinity());
    }
    bool drawDepthToImage(float nearDepth = 0.0f, float farDepth = 1.0f)
    {
        if (!ready() || !std::isfinite(nearDepth) || !std::isfinite(farDepth)
            || farDepth <= nearDepth) {
            return false;
        }
        for (std::size_t i = 0; i < depth_.size(); ++i) {
            const float depth = depth_[i];
            const float normalized = std::isfinite(depth)
                ? std::clamp((depth - nearDepth) / (farDepth - nearDepth), 0.0f, 1.0f)
                : 1.0f;
            const auto value = static_cast<std::uint8_t>(
                std::lround((1.0f - normalized) * 255.0f));
            output_.pixels()[i] = {value, value, value, 255};
        }
        return true;
    }
    bool drawNormalsToImage(const Mesh& mesh, const Camera& camera)
    {
        if (!ready() || !mesh.hasNormals()) {
            return false;
        }
        clear(options_.clearColor);
        const Mat4 viewProjection = camera.viewProjection(
            static_cast<float>(width()) / static_cast<float>(height()));
        for (std::size_t i = 0; i < mesh.vertices().size(); ++i) {
            const auto clip = viewProjection.transform(mesh.vertices()[i].x,
                mesh.vertices()[i].y, mesh.vertices()[i].z, 1.0f);
            ClipVertex vertex;
            vertex.position = clip;
            bool inside = true;
            for (int plane = 0; plane < 6; ++plane) {
                inside = inside && planeDistance(vertex, plane) >= 0.0f;
            }
            if (!inside) {
                continue;
            }
            const ScreenVertex point = toScreen({clip, {}, mesh.normals()[i]});
            const Vec3 normal = mesh.normals()[i].normalized();
            const Color color{
                channel(normal.x * 0.5f + 0.5f),
                channel(normal.y * 0.5f + 0.5f),
                channel(normal.z * 0.5f + 0.5f), 255};
            drawPoint(point.x, point.y, color, 3.0f);
        }
        return true;
    }

    Color shadeLambert(Color albedo, const Vec3& normal,
                       const Vec3& lightDirection, float ambient,
                       float diffuse) const
    {
        const Vec3 n = normal.normalized();
        const Vec3 l = (-lightDirection).normalized();
        const float lighting = std::clamp(ambient + diffuse * std::max(0.0f, n.dot(l)),
                                          0.0f, 4.0f);
        return scaleColor(albedo, lighting);
    }
    Color shadeBlinnPhong(Color albedo, const Vec3& normal,
                          const Vec3& lightDirection, const Vec3& viewDirection,
                          float ambient, float diffuse, float specular,
                          float shininess) const
    {
        const Vec3 n = normal.normalized();
        const Vec3 light = (-lightDirection).normalized();
        const Vec3 view = viewDirection.normalized();
        const Vec3 halfVector = (light + view).normalized();
        const float lambert = std::max(0.0f, n.dot(light));
        const float highlight = std::pow(std::max(0.0f, n.dot(halfVector)),
                                         std::max(1.0f, shininess));
        const Color diffuseColor = scaleColor(albedo, ambient + diffuse * lambert);
        const Color specularColor = scaleColor(Color::white(), specular * highlight);
        return addColor(diffuseColor, specularColor);
    }

    void drawPoint(float x, float y, Color color, float size = 1.0f)
    {
        if (!ready() || !std::isfinite(x) || !std::isfinite(y)
            || !std::isfinite(size) || size <= 0.0f) {
            return;
        }
        const float radius = size * 0.5f;
        if (x + radius <= 0.0f || y + radius <= 0.0f
            || x - radius >= width() || y - radius >= height()) {
            return;
        }
        const int left = std::max(0, static_cast<int>(std::floor(x - radius)));
        const int top = std::max(0, static_cast<int>(std::floor(y - radius)));
        const int right = std::min(width(), static_cast<int>(std::ceil(x + radius)));
        const int bottom = std::min(height(), static_cast<int>(std::ceil(y + radius)));
        for (int py = top; py < bottom; ++py) {
            for (int px = left; px < right; ++px) {
                output_.at(px, py) = Color::alphaBlend(color, output_.at(px, py));
            }
        }
    }
        bool drawScreenSpaceLine(const Vec2& from, float fromDepth,
                                 const Vec2& to, float toDepth,
                                 Color color, float thickness = 1.0f)
        {
            if (!ready() || !from.isFinite() || !to.isFinite()
                || !std::isfinite(fromDepth) || !std::isfinite(toDepth)
                || fromDepth < 0.0f || fromDepth > 1.0f
                || toDepth < 0.0f || toDepth > 1.0f
                || !std::isfinite(thickness) || thickness <= 0.0f) {
                return false;
            }
            ScreenVertex a;
            ScreenVertex b;
            a.x = from.x;
            a.y = from.y;
            a.z = fromDepth * 2.0f - 1.0f;
            b.x = to.x;
            b.y = to.y;
            b.z = toDepth * 2.0f - 1.0f;
            if (!clipScreenSegment(a, b, 0.0f, 0.0f,
                                   static_cast<float>(width() - 1),
                                   static_cast<float>(height() - 1))) {
                return false;
            }
            drawDepthLine(a, b, color, thickness);
            return true;
        }
        bool drawScreenSpaceTriangle(const Vec2& a, float depthA, Color colorA,
                                     const Vec2& b, float depthB, Color colorB,
                                     const Vec2& c, float depthC, Color colorC)
        {
            if (!ready() || !a.isFinite() || !b.isFinite() || !c.isFinite()
                || !std::isfinite(depthA) || !std::isfinite(depthB)
                || !std::isfinite(depthC) || depthA < 0.0f || depthA > 1.0f
                || depthB < 0.0f || depthB > 1.0f
                || depthC < 0.0f || depthC > 1.0f) {
                return false;
            }
            const float area = edge(a.x, a.y, b.x, b.y, c.x, c.y);
            if (std::abs(area) < 1.0e-8f) {
                return false;
            }
            const float minimumX = std::min({a.x, b.x, c.x});
            const float maximumX = std::max({a.x, b.x, c.x});
            const float minimumY = std::min({a.y, b.y, c.y});
            const float maximumY = std::max({a.y, b.y, c.y});
            if (maximumX < 0.0f || maximumY < 0.0f
                || minimumX >= width() || minimumY >= height()) {
                return true;
            }
            const int left = static_cast<int>(std::floor(std::clamp(
                minimumX, 0.0f, static_cast<float>(width() - 1))));
            const int right = static_cast<int>(std::ceil(std::clamp(
                maximumX, 0.0f, static_cast<float>(width() - 1))));
            const int top = static_cast<int>(std::floor(std::clamp(
                minimumY, 0.0f, static_cast<float>(height() - 1))));
            const int bottom = static_cast<int>(std::ceil(std::clamp(
                maximumY, 0.0f, static_cast<float>(height() - 1))));
            const float orientation = area < 0.0f ? -1.0f : 1.0f;
            for (int y = top; y <= bottom; ++y) {
                for (int x = left; x <= right; ++x) {
                    const float px = static_cast<float>(x) + 0.5f;
                    const float py = static_cast<float>(y) + 0.5f;
                    const float wa = edge(b.x, b.y, c.x, c.y, px, py)
                        * orientation / std::abs(area);
                    const float wb = edge(c.x, c.y, a.x, a.y, px, py)
                        * orientation / std::abs(area);
                    const float wc = 1.0f - wa - wb;
                    if (wa < 0.0f || wb < 0.0f || wc < 0.0f) {
                        continue;
                    }
                    const float depth = wa * depthA + wb * depthB + wc * depthC;
                    const auto channelValue = [wa, wb, wc](
                        std::uint8_t first, std::uint8_t second, std::uint8_t third) {
                        return static_cast<std::uint8_t>(std::clamp(
                            static_cast<int>(std::lround(first * wa + second * wb + third * wc)),
                            0, 255));
                    };
                    const Color color{
                        channelValue(colorA.r, colorB.r, colorC.r),
                        channelValue(colorA.g, colorB.g, colorC.g),
                        channelValue(colorA.b, colorB.b, colorC.b),
                        channelValue(colorA.a, colorB.a, colorC.a)};
                    testAndSetDepth(x, y, depth, color);
                }
            }
            return true;
        }
        void drawScreenSpaceRectangle(const Rect& rectangle, float depth,
                                      Color color, bool filled = true,
                                      float thickness = 1.0f)
        {
            if (!std::isfinite(rectangle.left()) || !std::isfinite(rectangle.top())
                || !std::isfinite(rectangle.right()) || !std::isfinite(rectangle.bottom())
                || !std::isfinite(depth) || depth < 0.0f || depth > 1.0f) {
                return;
            }
            const Rect r = rectangle.normalized();
            if (filled) {
                drawScreenSpaceTriangle({r.left(), r.top()}, depth, color,
                    {r.right(), r.top()}, depth, color,
                    {r.right(), r.bottom()}, depth, color);
                drawScreenSpaceTriangle({r.left(), r.top()}, depth, color,
                    {r.right(), r.bottom()}, depth, color,
                    {r.left(), r.bottom()}, depth, color);
                return;
            }
            drawScreenSpaceLine({r.left(), r.top()}, depth,
                                {r.right(), r.top()}, depth, color, thickness);
            drawScreenSpaceLine({r.right(), r.top()}, depth,
                                {r.right(), r.bottom()}, depth, color, thickness);
            drawScreenSpaceLine({r.right(), r.bottom()}, depth,
                                {r.left(), r.bottom()}, depth, color, thickness);
            drawScreenSpaceLine({r.left(), r.bottom()}, depth,
                                {r.left(), r.top()}, depth, color, thickness);
        }
    void drawLine(const Vec3& from, const Vec3& to, const Camera& camera,
                  Color color, float thickness = 1.0f)
    {
        if (!ready() || !from.isFinite() || !to.isFinite()
            || !std::isfinite(thickness) || thickness <= 0.0f) {
            return;
        }
        const Mat4 matrix = camera.viewProjection(
            static_cast<float>(width()) / static_cast<float>(height()));
        ClipVertex a;
        ClipVertex b;
        a.position = matrix.transform(from.x, from.y, from.z, 1.0f);
        b.position = matrix.transform(to.x, to.y, to.z, 1.0f);
        if (!clipLine(a, b)) {
            return;
        }
        drawDepthLine(toScreen(a), toScreen(b), color, thickness);
    }

private:
    struct ClipVertex {
        std::array<float, 4> position{};
        Vec2 uv{};
        Vec3 normal{};
    };
    struct ScreenVertex {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float inverseW = 1.0f;
        Vec2 uv{};
        Vec3 normal{};
    };

    bool renderIntoCurrentFrame(const Mesh& mesh, const Camera& camera,
                                const ImageL3D* texture)
    {
        if (!mesh.isValid()) {
            return false;
        }
        const Mat4 viewProjection = camera.viewProjection(
            static_cast<float>(width()) / static_cast<float>(height()));
        meshHasTextureCoordinates_ = mesh.hasTextureCoordinates()
            && options_.useTextureCoordinates;
        std::vector<ClipVertex> transformed;
        transformed.reserve(mesh.vertices().size());
        for (std::size_t index = 0; index < mesh.vertices().size(); ++index) {
            const Vec3& position = mesh.vertices()[index];
            ClipVertex vertex;
            vertex.position = viewProjection.transform(position.x, position.y,
                                                        position.z, 1.0f);
            if (meshHasTextureCoordinates_) {
                vertex.uv = mesh.textureCoordinates()[index];
            }
            vertex.normal = mesh.hasNormals()
                ? mesh.normals()[index].normalized() : Vec3::unitY();
            transformed.push_back(vertex);
        }
        statistics_.inputTriangles += mesh.triangles().size();
        for (const Triangle& triangle : mesh.triangles()) {
            if (triangle.a >= transformed.size() || triangle.b >= transformed.size()
                || triangle.c >= transformed.size()) {
                continue;
            }
            const Vec3 faceNormal = (mesh.vertices()[triangle.b] - mesh.vertices()[triangle.a])
                .cross(mesh.vertices()[triangle.c] - mesh.vertices()[triangle.a]).normalized();
            if (faceNormal.lengthSquared() <= 0.0f) {
                continue;
            }
            const Material* material = mesh.materialFor(triangle);
            const Color baseColor = material != nullptr && options_.useMaterialColors
                ? material->baseColor : mesh.color();
            const Vec3 facePosition = (mesh.vertices()[triangle.a]
                + mesh.vertices()[triangle.b] + mesh.vertices()[triangle.c]) / 3.0f;
            const bool doubleSided = material != nullptr && material->doubleSided;
            std::vector<ClipVertex> polygon{
                transformed[triangle.a], transformed[triangle.b], transformed[triangle.c]};
            polygon = clipPolygon(std::move(polygon));
            if (polygon.size() < 3) {
                continue;
            }
            if (polygon.size() > 3) {
                statistics_.clippedTriangles += polygon.size() - 2;
            }
            for (std::size_t index = 1; index + 1 < polygon.size(); ++index) {
                rasterize(polygon[0], polygon[index], polygon[index + 1],
                          faceNormal, facePosition, baseColor, material,
                          texture, doubleSided);
            }
            if (options_.wireframe) {
                const auto screen = projectPolygon(polygon);
                for (std::size_t index = 0; index < screen.size(); ++index) {
                    drawDepthLine(screen[index], screen[(index + 1) % screen.size()],
                                  options_.wireColor, options_.wireWidth);
                }
            }
        }
        return true;
    }
    static std::uint8_t channel(float value)
    {
        return static_cast<std::uint8_t>(std::clamp(
            static_cast<int>(std::lround(value * 255.0f)), 0, 255));
    }
    static std::uint8_t clampChannel(int value)
    {
        return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
    }
    static Color averageColors(std::initializer_list<Color> colors)
    {
        if (colors.size() == 0) {
            return Color::transparent();
        }
        unsigned int red = 0;
        unsigned int green = 0;
        unsigned int blue = 0;
        unsigned int alpha = 0;
        for (const Color color : colors) {
            red += color.r;
            green += color.g;
            blue += color.b;
            alpha += color.a;
        }
        const unsigned int count = static_cast<unsigned int>(colors.size());
        return {static_cast<std::uint8_t>(red / count),
                static_cast<std::uint8_t>(green / count),
                static_cast<std::uint8_t>(blue / count),
                static_cast<std::uint8_t>(alpha / count)};
    }
    static Color scaleColor(Color value, float scale)
    {
        const auto scaleChannel = [scale](std::uint8_t component) {
            return static_cast<std::uint8_t>(std::clamp(
                static_cast<int>(std::lround(component * scale)), 0, 255));
        };
        return {scaleChannel(value.r), scaleChannel(value.g),
                scaleChannel(value.b), value.a};
    }
    static Color addColor(Color lhs, Color rhs)
    {
        return {
            static_cast<std::uint8_t>(std::min(255, lhs.r + rhs.r)),
            static_cast<std::uint8_t>(std::min(255, lhs.g + rhs.g)),
            static_cast<std::uint8_t>(std::min(255, lhs.b + rhs.b)),
            std::max(lhs.a, rhs.a)};
    }
    static Color scaleRgb(Color color, float scale)
    {
        const auto scaleChannel = [scale](std::uint8_t value) {
            return channel(static_cast<float>(value) / 255.0f * scale);
        };
        color.r = scaleChannel(color.r);
        color.g = scaleChannel(color.g);
        color.b = scaleChannel(color.b);
        return color;
    }
    static Image blurredImage(const Image& source, unsigned int radius)
    {
        if (source.empty() || radius == 0) {
            return source;
        }
        radius = std::min(radius, 32u);
        Image horizontal = source;
        Image output = source;
        const int r = static_cast<int>(radius);
        const float weight = 1.0f / static_cast<float>(r * 2 + 1);
        for (int y = 0; y < source.height(); ++y) {
            for (int x = 0; x < source.width(); ++x) {
                float channels[4]{};
                for (int offset = -r; offset <= r; ++offset) {
                    const Color sample = source.at(
                        std::clamp(x + offset, 0, source.width() - 1), y);
                    channels[0] += sample.r * weight;
                    channels[1] += sample.g * weight;
                    channels[2] += sample.b * weight;
                    channels[3] += sample.a * weight;
                }
                horizontal.at(x, y) = {
                    channel(channels[0] / 255.0f),
                    channel(channels[1] / 255.0f),
                    channel(channels[2] / 255.0f),
                    channel(channels[3] / 255.0f)};
            }
        }
        for (int y = 0; y < source.height(); ++y) {
            for (int x = 0; x < source.width(); ++x) {
                float channels[4]{};
                for (int offset = -r; offset <= r; ++offset) {
                    const Color sample = horizontal.at(x,
                        std::clamp(y + offset, 0, source.height() - 1));
                    channels[0] += sample.r * weight;
                    channels[1] += sample.g * weight;
                    channels[2] += sample.b * weight;
                    channels[3] += sample.a * weight;
                }
                output.at(x, y) = {
                    channel(channels[0] / 255.0f),
                    channel(channels[1] / 255.0f),
                    channel(channels[2] / 255.0f),
                    channel(channels[3] / 255.0f)};
            }
        }
        return output;
    }
    static std::array<Vec2, 8> depthOffsets()
    {
        return {{{-1.0f, -1.0f}, {0.0f, -1.0f}, {1.0f, -1.0f},
                 {-1.0f, 0.0f}, {1.0f, 0.0f}, {-1.0f, 1.0f},
                 {0.0f, 1.0f}, {1.0f, 1.0f}}};
    }
    static bool intersectTriangle(const Ray3& ray, const Vec3& a,
                                  const Vec3& b, const Vec3& c,
                                  float& distance, float& u, float& v)
    {
        const Vec3 edge1 = b - a;
        const Vec3 edge2 = c - a;
        const Vec3 p = ray.direction.cross(edge2);
        const float determinant = edge1.dot(p);
        if (std::abs(determinant) < 1.0e-8f) {
            return false;
        }
        const float inverse = 1.0f / determinant;
        const Vec3 offset = ray.origin - a;
        u = offset.dot(p) * inverse;
        if (u < 0.0f || u > 1.0f) {
            return false;
        }
        const Vec3 q = offset.cross(edge1);
        v = ray.direction.dot(q) * inverse;
        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }
        distance = edge2.dot(q) * inverse;
        return distance >= 0.0f;
    }
    static Bounds3 transformBounds(const Bounds3& bounds,
                                   const Transform3D& transform)
    {
        if (bounds.isEmpty()) {
            return Bounds3::empty();
        }
        Bounds3 result = Bounds3::empty();
        for (const Vec3& corner : bounds.corners()) {
            result = result.expanded(transform.transformPoint(corner));
        }
        return result;
    }
    std::size_t pixelIndex(int x, int y) const
    {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(width())
             + static_cast<std::size_t>(x);
    }
    static float planeDistance(const ClipVertex& vertex, int plane)
    {
        const auto& p = vertex.position;
        switch (plane) {
        case 0: return p[0] + p[3];
        case 1: return p[3] - p[0];
        case 2: return p[1] + p[3];
        case 3: return p[3] - p[1];
        case 4: return p[2] + p[3];
        default: return p[3] - p[2];
        }
    }
    static ClipVertex interpolate(const ClipVertex& a, const ClipVertex& b, float t)
    {
        ClipVertex result;
        for (std::size_t i = 0; i < result.position.size(); ++i) {
            result.position[i] = a.position[i]
                + (b.position[i] - a.position[i]) * t;
        }
        result.uv = a.uv + (b.uv - a.uv) * t;
        result.normal = Vec3::lerp(a.normal, b.normal, t);
        return result;
    }
    static std::vector<ClipVertex> clipPolygon(std::vector<ClipVertex> polygon)
    {
        for (int plane = 0; plane < 6 && !polygon.empty(); ++plane) {
            std::vector<ClipVertex> output;
            output.reserve(polygon.size() + 1);
            ClipVertex previous = polygon.back();
            float previousDistance = planeDistance(previous, plane);
            bool previousInside = previousDistance >= 0.0f;
            for (const ClipVertex& current : polygon) {
                const float currentDistance = planeDistance(current, plane);
                const bool currentInside = currentDistance >= 0.0f;
                if (currentInside != previousInside) {
                    const float denominator = previousDistance - currentDistance;
                    if (std::abs(denominator) > 1.0e-20f) {
                        const float t = previousDistance / denominator;
                        output.push_back(interpolate(previous, current, t));
                    }
                }
                if (currentInside) {
                    output.push_back(current);
                }
                previous = current;
                previousDistance = currentDistance;
                previousInside = currentInside;
            }
            polygon = std::move(output);
        }
        return polygon;
    }
    static bool clipLine(ClipVertex& a, ClipVertex& b)
    {
        float first = 0.0f;
        float last = 1.0f;
        for (int plane = 0; plane < 6; ++plane) {
            const float da = planeDistance(a, plane);
            const float db = planeDistance(b, plane);
            if (da < 0.0f && db < 0.0f) {
                return false;
            }
            if ((da < 0.0f) != (db < 0.0f)) {
                const float t = da / (da - db);
                if (da < 0.0f) {
                    first = std::max(first, t);
                } else {
                    last = std::min(last, t);
                }
            }
        }
        const ClipVertex originalA = a;
        const ClipVertex originalB = b;
        a = interpolate(originalA, originalB, first);
        b = interpolate(originalA, originalB, last);
        return first <= last;
    }
    ScreenVertex toScreen(const ClipVertex& vertex) const
    {
        const float inverseW = 1.0f / vertex.position[3];
        return {
            (vertex.position[0] * inverseW * 0.5f + 0.5f) * (width() - 1),
            (0.5f - vertex.position[1] * inverseW * 0.5f) * (height() - 1),
            vertex.position[2] * inverseW,
            inverseW,
            vertex.uv,
            vertex.normal
        };
    }
    std::vector<ScreenVertex> projectPolygon(const std::vector<ClipVertex>& polygon) const
    {
        std::vector<ScreenVertex> result;
        result.reserve(polygon.size());
        for (const ClipVertex& vertex : polygon) {
            result.push_back(toScreen(vertex));
        }
        return result;
    }
    void beginFrame()
    {
        output_.clear(options_.clearColor);
        std::fill(depth_.begin(), depth_.end(),
                  std::numeric_limits<float>::infinity());
        statistics_ = {};
    }
    void rasterize(const ClipVertex& clipA, const ClipVertex& clipB,
                   const ClipVertex& clipC, const Vec3& faceNormal,
                   const Vec3& facePosition,
                   Color baseColor, const Material* material,
                   const ImageL3D* texture, bool doubleSided)
    {
        if (clipA.position[3] <= 0.0f || clipB.position[3] <= 0.0f
            || clipC.position[3] <= 0.0f) {
            return;
        }
        const ScreenVertex a = toScreen(clipA);
        const ScreenVertex b = toScreen(clipB);
        const ScreenVertex c = toScreen(clipC);
        const float area = edge(a.x, a.y, b.x, b.y, c.x, c.y);
        if (std::abs(area) < 1.0e-6f) {
            return;
        }
        const bool frontFacing = area < 0.0f;
        if (!doubleSided
            && ((options_.cullMode == CullMode3D::Back && !frontFacing)
                || (options_.cullMode == CullMode3D::Front && frontFacing))) {
            return;
        }
        const float minX = std::min({a.x, b.x, c.x});
        const float maxX = std::max({a.x, b.x, c.x});
        const float minY = std::min({a.y, b.y, c.y});
        const float maxY = std::max({a.y, b.y, c.y});
        const int left = std::max(0, static_cast<int>(std::floor(minX)));
        const int right = std::min(width() - 1, static_cast<int>(std::ceil(maxX)));
        const int top = std::max(0, static_cast<int>(std::floor(minY)));
        const int bottom = std::min(height() - 1, static_cast<int>(std::ceil(maxY)));
        if (left > right || top > bottom) {
            return;
        }
        ++statistics_.rasterizedTriangles;
        const float orientation = area < 0.0f ? -1.0f : 1.0f;
        const float areaMagnitude = std::abs(area);
        const Vec3 normal = doubleSided && !frontFacing ? -faceNormal : faceNormal;
        const float lighting = evaluateLighting(normal, facePosition, material);
        const Color litColor = scaleColor(baseColor, lighting);
        for (int y = top; y <= bottom; ++y) {
            for (int x = left; x <= right; ++x) {
                ++statistics_.testedPixels;
                const float px = static_cast<float>(x) + 0.5f;
                const float py = static_cast<float>(y) + 0.5f;
                const float wa = edge(b.x, b.y, c.x, c.y, px, py) * orientation
                               / areaMagnitude;
                const float wb = edge(c.x, c.y, a.x, a.y, px, py) * orientation
                               / areaMagnitude;
                const float wc = 1.0f - wa - wb;
                if (wa < 0.0f || wb < 0.0f || wc < 0.0f) {
                    continue;
                }
                const float depth = (wa * a.z + wb * b.z + wc * c.z) * 0.5f + 0.5f;
                if (depth < 0.0f || depth > 1.0f) {
                    continue;
                }
                const std::size_t index = static_cast<std::size_t>(y)
                    * static_cast<std::size_t>(width()) + static_cast<std::size_t>(x);
                if (depth >= depth_[index]) {
                    ++statistics_.depthRejectedPixels;
                    continue;
                }
                const float reciprocalW = wa * a.inverseW + wb * b.inverseW
                                        + wc * c.inverseW;
                Color shaded = litColor;
                Vec2 finalUv{};
                if (reciprocalW > 0.0f) {
                    finalUv = (a.uv * (wa * a.inverseW)
                        + b.uv * (wb * b.inverseW)
                        + c.uv * (wc * c.inverseW)) / reciprocalW;
                }
                if (texture != nullptr && !texture->empty()
                    && options_.useTextureCoordinates
                    && meshHasTextureCoordinates_) {
                    shaded = Color::multiply(shaded, texture->sample(finalUv));
                }
                if (material != nullptr && material->emissiveColor != Color::black()) {
                    shaded = addColor(shaded, material->emissiveColor);
                }
                if (pixelShader_) {
                    shaded = pixelShader_(shaded, depth, finalUv);
                }
                if (material != nullptr && options_.blendTransparent
                    && material->isTransparent()) {
                    const float opacity = std::isfinite(material->opacity)
                        ? std::clamp(material->opacity, 0.0f, 1.0f) : 0.0f;
                    const auto materialAlpha = static_cast<std::uint8_t>(
                        std::lround(opacity * 255.0f));
                    shaded.a = std::min(shaded.a, materialAlpha);
                    output_.pixels()[index] = Color::alphaBlend(shaded, output_.pixels()[index]);
                } else {
                    output_.pixels()[index] = shaded;
                }
                depth_[index] = depth;
                ++statistics_.shadedPixels;
            }
        }
    }
    float evaluateLighting(const Vec3& normal, const Vec3& position,
                           const Material* material) const
    {
        if (material != nullptr && material->shading == ShadingModel::Unlit) {
            return 1.0f;
        }
        const float ambientColor = options_.ambientColor.perceivedBrightness();
        float result = std::max(0.0f, options_.ambient) * ambientColor;
        if (lights_.empty()) {
            return std::clamp(result + std::max(0.0f, options_.diffuse) * std::max(
                0.0f, normal.normalized().dot((-options_.lightDirection).normalized())),
                0.0f, 4.0f);
        }
        for (const DirectionalLight3D& light : lights_) {
            float intensity = std::max(0.0f, light.intensity);
            Vec3 lightDirection;
            if (light.type == LightType::Directional) {
                lightDirection = (-light.direction).normalized();
            } else {
                const Vec3 toLight = light.position - position;
                const float distance = toLight.length();
                if (light.range <= 0.0f || distance >= light.range) {
                    continue;
                }
                lightDirection = toLight.normalized();
                const float normalizedDistance = distance / light.range;
                intensity *= (1.0f - normalizedDistance * normalizedDistance)
                    * (1.0f - normalizedDistance * normalizedDistance);
                if (light.type == LightType::Spot) {
                    const float cosine = (-lightDirection).dot(light.direction.normalized());
                    const float inner = std::cos(light.innerConeRadians);
                    const float outer = std::cos(light.outerConeRadians);
                    const float cone = std::abs(inner - outer) < 1.0e-6f
                        ? (cosine >= inner ? 1.0f : 0.0f)
                        : std::clamp((cosine - outer) / (inner - outer), 0.0f, 1.0f);
                    intensity *= cone;
                }
            }
            const float lambert = std::max(0.0f, normal.normalized().dot(lightDirection));
            const float lightBrightness = light.color.perceivedBrightness();
            result += std::max(0.0f, options_.diffuse)
                * intensity * lightBrightness * lambert;
        }
        return std::clamp(result, 0.0f, 4.0f);
    }
    static float edge(float ax, float ay, float bx, float by, float px, float py)
    {
        return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
    }
    static void interpolateDepth(const ScreenVertex& a, const ScreenVertex& b,
                                 float t, ScreenVertex& result)
    {
        result.x = a.x + (b.x - a.x) * t;
        result.y = a.y + (b.y - a.y) * t;
        result.z = a.z + (b.z - a.z) * t;
        result.inverseW = a.inverseW + (b.inverseW - a.inverseW) * t;
    }
    static bool clipScreenSegment(ScreenVertex& a, ScreenVertex& b,
                                  float minimumX, float minimumY,
                                  float maximumX, float maximumY)
    {
        const float dx = b.x - a.x;
        const float dy = b.y - a.y;
        float first = 0.0f;
        float last = 1.0f;
        const auto clip = [&first, &last](float p, float q) {
            if (p == 0.0f) {
                return q >= 0.0f;
            }
            const float amount = q / p;
            if (p < 0.0f) {
                if (amount > last) {
                    return false;
                }
                first = std::max(first, amount);
            } else {
                if (amount < first) {
                    return false;
                }
                last = std::min(last, amount);
            }
            return true;
        };
        if (!clip(-dx, a.x - minimumX)
            || !clip(dx, maximumX - a.x)
            || !clip(-dy, a.y - minimumY)
            || !clip(dy, maximumY - a.y)
            || first > last) {
            return false;
        }
        const ScreenVertex originalA = a;
        const ScreenVertex originalB = b;
        interpolateDepth(originalA, originalB, first, a);
        interpolateDepth(originalA, originalB, last, b);
        return true;
    }
    void drawDepthPoint(float x, float y, float depth,
                        Color color, float size)
    {
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(depth)
            || !std::isfinite(size) || size <= 0.0f) {
            return;
        }
        const float radius = std::max(0.5f, size * 0.5f);
        if (x + radius < 0.0f || y + radius < 0.0f
            || x - radius >= width() || y - radius >= height()) {
            return;
        }
        const int left = std::max(0, static_cast<int>(std::floor(x - radius)));
        const int right = std::min(width() - 1,
            static_cast<int>(std::ceil(x + radius)));
        const int top = std::max(0, static_cast<int>(std::floor(y - radius)));
        const int bottom = std::min(height() - 1,
            static_cast<int>(std::ceil(y + radius)));
        for (int py = top; py <= bottom; ++py) {
            for (int px = left; px <= right; ++px) {
                testAndSetDepth(px, py, depth, color);
            }
        }
    }
    void drawDepthLine(const ScreenVertex& a, const ScreenVertex& b,
                       Color color, float thickness)
    {
        const float dx = b.x - a.x;
        const float dy = b.y - a.y;
        const float lengthSquared = dx * dx + dy * dy;
        const float radius = std::max(0.5f, thickness * 0.5f);
        const int left = std::max(0, static_cast<int>(std::floor(
            std::min(a.x, b.x) - radius)));
        const int right = std::min(width() - 1, static_cast<int>(std::ceil(
            std::max(a.x, b.x) + radius)));
        const int top = std::max(0, static_cast<int>(std::floor(
            std::min(a.y, b.y) - radius)));
        const int bottom = std::min(height() - 1, static_cast<int>(std::ceil(
            std::max(a.y, b.y) + radius)));
        for (int y = top; y <= bottom; ++y) {
            for (int x = left; x <= right; ++x) {
                const float px = static_cast<float>(x) + 0.5f;
                const float py = static_cast<float>(y) + 0.5f;
                const float t = lengthSquared > 0.0f
                    ? std::clamp(((px - a.x) * dx + (py - a.y) * dy)
                                 / lengthSquared, 0.0f, 1.0f) : 0.0f;
                const float qx = a.x + dx * t;
                const float qy = a.y + dy * t;
                if ((px - qx) * (px - qx) + (py - qy) * (py - qy)
                    > radius * radius) {
                    continue;
                }
                const float depth = ((a.z + (b.z - a.z) * t) * 0.5f) + 0.5f;
                const std::size_t index = static_cast<std::size_t>(y)
                    * static_cast<std::size_t>(width()) + static_cast<std::size_t>(x);
                if (depth <= depth_[index]) {
                    output_.pixels()[index] = color;
                    depth_[index] = depth;
                }
            }
        }
    }

    Image output_;
    std::vector<float> depth_;
    RenderOptions3D options_;
    std::vector<DirectionalLight3D> lights_;
    RenderStatistics3D statistics_;
    bool meshHasTextureCoordinates_ = false;
    std::function<Color(Color, float, const Vec2&)> pixelShader_;
};

using Renderer3DSoftware = Renderer3DTarget;
using Renderer3DR = Renderer3DTarget;

} // namespace peco
