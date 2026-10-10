#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <peco\color.hpp>
#include <peco\material.hpp>
#include <peco\bounds3.hpp>
#include <peco\result.hpp>
#include <peco\rect.hpp>
#include <peco\transform3d.hpp>
#include <peco\vector3.hpp>

namespace peco {

struct Triangle {
    unsigned int a = 0;
    unsigned int b = 0;
    unsigned int c = 0;
    unsigned int materialIndex = 0;
};

enum class MeshFileFormat {
    Auto,
    Obj
};

struct MeshLoadOptions {
    bool generateMissingNormals = true;
    bool flipTextureV = false;
    float scale = 1.0f;
    bool invertWinding = false;
};

struct MeshError {
    std::wstring path;
    std::string message;
    std::size_t line = 0;

    std::string describe() const;
};

struct SubmeshRange {
    std::string name;
    std::size_t firstTriangle = 0;
    std::size_t triangleCount = 0;
    unsigned int materialIndex = 0;
};

class Mesh {
public:
    Mesh() = default;
    Mesh(std::vector<Vec3> vertices, std::vector<Triangle> triangles,
         Color color = {220, 220, 230});

    const std::vector<Vec3>& vertices() const { return vertices_; }
    std::vector<Vec3>& vertices() { return vertices_; }
    const std::vector<Triangle>& triangles() const { return triangles_; }
    std::vector<Triangle>& triangles() { return triangles_; }
    const std::vector<Vec3>& normals() const { return normals_; }
    std::vector<Vec3>& normals() { return normals_; }
    const std::vector<Vec2>& textureCoordinates() const { return textureCoordinates_; }
    std::vector<Vec2>& textureCoordinates() { return textureCoordinates_; }
    const std::vector<Material>& materials() const { return materials_; }
    std::vector<Material>& materials() { return materials_; }
    const std::vector<SubmeshRange>& submeshes() const { return submeshes_; }
    std::vector<SubmeshRange>& submeshes() { return submeshes_; }
    Color color() const { return color_; }
    void setColor(Color color) { color_ = color; }
    std::size_t vertexCount() const { return vertices_.size(); }
    std::size_t triangleCount() const { return triangles_.size(); }
    std::size_t indexCount() const { return triangles_.size() * 3u; }
    bool empty() const { return vertices_.empty() || triangles_.empty(); }
    bool hasNormals() const { return normals_.size() == vertices_.size(); }
    bool hasTextureCoordinates() const
    {
        return textureCoordinates_.size() == vertices_.size();
    }
    bool hasMaterials() const { return !materials_.empty(); }
    bool isValid() const;
    Bounds3 bounds() const;
    float surfaceArea() const;
    float signedVolume() const;
    void clear();
    void setNormals(std::vector<Vec3> normals);
    void setTextureCoordinates(std::vector<Vec2> coordinates);
    void setMaterials(std::vector<Material> materials);
    void generateNormals(bool smooth = true);
    void flipWinding();
    void applyTransform(const Transform3D& transform);
    bool append(const Mesh& other, const Transform3D& transform = {});
    void recenter();
    void scaleToFit(float maximumDimension);
    void removeDegenerateTriangles(float epsilon = 0.000001f);
    void removeUnreferencedVertices();
    void recomputeBounds();
    const Bounds3& cachedBounds() const { return bounds_; }
    const Material* materialFor(const Triangle& triangle) const;

    static Mesh cube(float size = 1.0f, Color color = {220, 220, 230});
    static Mesh plane(float width = 1.0f, float depth = 1.0f,
                      Color color = {220, 220, 230});
    static Mesh uvSphere(float radius = 1.0f, unsigned int longitudeSegments = 24,
                         unsigned int latitudeSegments = 12,
                         Color color = {220, 220, 230});
    static Mesh cylinder(float radius = 0.5f, float height = 1.0f,
                         unsigned int segments = 24,
                         Color color = {220, 220, 230});
    static Mesh cone(float radius = 0.5f, float height = 1.0f,
                     unsigned int segments = 24,
                     Color color = {220, 220, 230});

    static Result<Mesh, MeshError> load(const std::wstring& path,
                                        MeshFileFormat format = MeshFileFormat::Auto,
                                        const MeshLoadOptions& options = {});
    static Result<Mesh, MeshError> loadObj(const std::wstring& path,
                                           const MeshLoadOptions& options = {});
    bool saveObj(const std::wstring& path, std::string* error = nullptr) const;

private:
    friend class ObjMeshLoader;
    void updateBounds();

    std::vector<Vec3> vertices_;
    std::vector<Triangle> triangles_;
    std::vector<Vec3> normals_;
    std::vector<Vec2> textureCoordinates_;
    std::vector<Material> materials_;
    std::vector<SubmeshRange> submeshes_;
    Color color_{};
    Bounds3 bounds_{};
};

} // namespace peco
