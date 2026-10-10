#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include <peco\mesh.hpp>
#include <peco\bounds3.hpp>
#include <peco\transform3d.hpp>

namespace peco {

class MeshBuilder {
public:
    MeshBuilder& setColor(Color color) { color_ = color; return *this; }
    MeshBuilder& addVertex(const Vec3& vertex)
    {
        vertices_.push_back(vertex);
        return *this;
    }
    MeshBuilder& addTriangle(unsigned int a, unsigned int b, unsigned int c)
    {
        triangles_.push_back({a, b, c});
        return *this;
    }
    MeshBuilder& addQuad(unsigned int a, unsigned int b,
                         unsigned int c, unsigned int d)
    {
        addTriangle(a, b, c);
        addTriangle(a, c, d);
        return *this;
    }
    MeshBuilder& addMesh(const Mesh& mesh, const Transform3D& transform = {})
    {
        const auto offset = static_cast<unsigned int>(vertices_.size());
        for (const Vec3& vertex : mesh.vertices()) {
            vertices_.push_back(transform.transformPoint(vertex));
        }
        for (const Triangle& triangle : mesh.triangles()) {
            triangles_.push_back({triangle.a + offset, triangle.b + offset,
                                  triangle.c + offset});
        }
        return *this;
    }
    Mesh build() const { return Mesh{vertices_, triangles_, color_}; }
    void clear()
    {
        vertices_.clear();
        triangles_.clear();
    }
    const std::vector<Vec3>& vertices() const { return vertices_; }
    const std::vector<Triangle>& triangles() const { return triangles_; }
    Bounds3 bounds() const
    {
        Bounds3 result = Bounds3::empty();
        for (const Vec3& vertex : vertices_) {
            result = result.expanded(vertex);
        }
        return result;
    }

    static Mesh plane(float width = 1.0f, float depth = 1.0f,
                      Color color = {210, 210, 220})
    {
        const float x = width * 0.5f;
        const float z = depth * 0.5f;
        return Mesh{
            {{-x, 0.0f, -z}, {x, 0.0f, -z},
             {x, 0.0f, z}, {-x, 0.0f, z}},
            {{0, 2, 1}, {0, 3, 2}}, color
        };
    }

    static Mesh grid(float width, float depth, unsigned int columns,
                     unsigned int rows, Color color = {210, 210, 220})
    {
        if (columns == 0 || rows == 0) {
            return {};
        }
        std::vector<Vec3> vertices;
        std::vector<Triangle> triangles;
        const unsigned int rowWidth = columns + 1;
        vertices.reserve(static_cast<std::size_t>(rowWidth) * (rows + 1));
        triangles.reserve(static_cast<std::size_t>(columns) * rows * 2);
        for (unsigned int row = 0; row <= rows; ++row) {
            const float v = static_cast<float>(row) / static_cast<float>(rows);
            for (unsigned int column = 0; column <= columns; ++column) {
                const float u = static_cast<float>(column) / static_cast<float>(columns);
                vertices.push_back({(u - 0.5f) * width, 0.0f, (v - 0.5f) * depth});
            }
        }
        for (unsigned int row = 0; row < rows; ++row) {
            for (unsigned int column = 0; column < columns; ++column) {
                const unsigned int topLeft = row * rowWidth + column;
                const unsigned int topRight = topLeft + 1;
                const unsigned int bottomLeft = topLeft + rowWidth;
                const unsigned int bottomRight = bottomLeft + 1;
                triangles.push_back({topLeft, bottomRight, topRight});
                triangles.push_back({topLeft, bottomLeft, bottomRight});
            }
        }
        return {std::move(vertices), std::move(triangles), color};
    }

    static Mesh uvSphere(float radius = 1.0f, unsigned int longitudeSegments = 24,
                         unsigned int latitudeSegments = 12,
                         Color color = {220, 220, 230})
    {
        longitudeSegments = std::max(3u, longitudeSegments);
        latitudeSegments = std::max(2u, latitudeSegments);
        std::vector<Vec3> vertices;
        std::vector<Triangle> triangles;
        const unsigned int rowWidth = longitudeSegments + 1;
        vertices.reserve(static_cast<std::size_t>(rowWidth) * (latitudeSegments + 1));
        triangles.reserve(static_cast<std::size_t>(longitudeSegments)
                          * latitudeSegments * 2);
        constexpr float pi = 3.14159265358979323846f;
        for (unsigned int row = 0; row <= latitudeSegments; ++row) {
            const float latitude = pi * static_cast<float>(row)
                / static_cast<float>(latitudeSegments);
            const float ring = std::sin(latitude);
            const float y = std::cos(latitude);
            for (unsigned int column = 0; column <= longitudeSegments; ++column) {
                const float longitude = 2.0f * pi * static_cast<float>(column)
                    / static_cast<float>(longitudeSegments);
                vertices.push_back({radius * ring * std::cos(longitude),
                                    radius * y,
                                    radius * ring * std::sin(longitude)});
            }
        }
        for (unsigned int row = 0; row < latitudeSegments; ++row) {
            for (unsigned int column = 0; column < longitudeSegments; ++column) {
                const unsigned int a = row * rowWidth + column;
                const unsigned int b = a + rowWidth;
                triangles.push_back({a, b, a + 1});
                triangles.push_back({a + 1, b, b + 1});
            }
        }
        return {std::move(vertices), std::move(triangles), color};
    }

    static Mesh cylinder(float radius = 0.5f, float height = 1.0f,
                         unsigned int segments = 24,
                         Color color = {220, 220, 230})
    {
        segments = std::max(3u, segments);
        std::vector<Vec3> vertices;
        std::vector<Triangle> triangles;
        vertices.reserve(static_cast<std::size_t>(segments) * 2 + 2);
        constexpr float tau = 6.28318530717958647692f;
        for (unsigned int index = 0; index < segments; ++index) {
            const float angle = tau * static_cast<float>(index)
                / static_cast<float>(segments);
            const float x = std::cos(angle) * radius;
            const float z = std::sin(angle) * radius;
            vertices.push_back({x, -height * 0.5f, z});
            vertices.push_back({x, height * 0.5f, z});
        }
        const unsigned int bottomCenter = static_cast<unsigned int>(vertices.size());
        vertices.push_back({0.0f, -height * 0.5f, 0.0f});
        const unsigned int topCenter = static_cast<unsigned int>(vertices.size());
        vertices.push_back({0.0f, height * 0.5f, 0.0f});
        for (unsigned int index = 0; index < segments; ++index) {
            const unsigned int next = (index + 1) % segments;
            const unsigned int bottomA = index * 2;
            const unsigned int topA = bottomA + 1;
            const unsigned int bottomB = next * 2;
            const unsigned int topB = bottomB + 1;
            triangles.push_back({bottomA, topA, topB});
            triangles.push_back({bottomA, topB, bottomB});
            triangles.push_back({bottomCenter, bottomB, bottomA});
            triangles.push_back({topCenter, topA, topB});
        }
        return {std::move(vertices), std::move(triangles), color};
    }

    static Mesh cone(float radius = 0.5f, float height = 1.0f,
                     unsigned int segments = 24,
                     Color color = {220, 220, 230})
    {
        segments = std::max(3u, segments);
        std::vector<Vec3> vertices;
        std::vector<Triangle> triangles;
        vertices.reserve(static_cast<std::size_t>(segments) + 2);
        constexpr float tau = 6.28318530717958647692f;
        for (unsigned int index = 0; index < segments; ++index) {
            const float angle = tau * static_cast<float>(index)
                / static_cast<float>(segments);
            vertices.push_back({std::cos(angle) * radius, -height * 0.5f,
                                std::sin(angle) * radius});
        }
        const unsigned int baseCenter = static_cast<unsigned int>(vertices.size());
        vertices.push_back({0.0f, -height * 0.5f, 0.0f});
        const unsigned int tip = static_cast<unsigned int>(vertices.size());
        vertices.push_back({0.0f, height * 0.5f, 0.0f});
        for (unsigned int index = 0; index < segments; ++index) {
            const unsigned int next = (index + 1) % segments;
            triangles.push_back({index, next, tip});
            triangles.push_back({baseCenter, next, index});
        }
        return {std::move(vertices), std::move(triangles), color};
    }

private:
    std::vector<Vec3> vertices_;
    std::vector<Triangle> triangles_;
    Color color_{220, 220, 230};
};

} // namespace peco
