#pragma once

#include <array>

#include <peco\bounds3.hpp>
#include <peco\matrix4.hpp>
#include <peco\ray.hpp>

namespace peco {

enum class FrustumPlane : std::size_t {
    Left,
    Right,
    Bottom,
    Top,
    Near,
    Far
};

class Frustum {
public:
    Frustum() = default;

    static Frustum fromViewProjection(const Mat4& matrix)
    {
        Frustum result;
        result.planes_[static_cast<std::size_t>(FrustumPlane::Left)] =
            makePlane(matrix, 3, 0, 1.0f);
        result.planes_[static_cast<std::size_t>(FrustumPlane::Right)] =
            makePlane(matrix, 3, 0, -1.0f);
        result.planes_[static_cast<std::size_t>(FrustumPlane::Bottom)] =
            makePlane(matrix, 3, 1, 1.0f);
        result.planes_[static_cast<std::size_t>(FrustumPlane::Top)] =
            makePlane(matrix, 3, 1, -1.0f);
        result.planes_[static_cast<std::size_t>(FrustumPlane::Near)] =
            makePlane(matrix, 3, 2, 1.0f);
        result.planes_[static_cast<std::size_t>(FrustumPlane::Far)] =
            makePlane(matrix, 3, 2, -1.0f);
        return result;
    }

    const Plane& plane(FrustumPlane index) const
    {
        return planes_[static_cast<std::size_t>(index)];
    }
    const std::array<Plane, 6>& planes() const { return planes_; }
    bool contains(const Vec3& point) const
    {
        for (const Plane& boundary : planes_) {
            if (boundary.signedDistance(point) < 0.0f) {
                return false;
            }
        }
        return true;
    }
    bool intersects(const Bounds3& bounds) const
    {
        if (bounds.isEmpty()) {
            return false;
        }
        for (const Plane& boundary : planes_) {
            const Vec3 positive{
                boundary.normal.x >= 0.0f ? bounds.maximum.x : bounds.minimum.x,
                boundary.normal.y >= 0.0f ? bounds.maximum.y : bounds.minimum.y,
                boundary.normal.z >= 0.0f ? bounds.maximum.z : bounds.minimum.z
            };
            if (boundary.signedDistance(positive) < 0.0f) {
                return false;
            }
        }
        return true;
    }
    bool intersectsSphere(const Vec3& center, float radius) const
    {
        for (const Plane& boundary : planes_) {
            if (boundary.signedDistance(center) < -radius) {
                return false;
            }
        }
        return true;
    }

private:
    static Plane makePlane(const Mat4& matrix, int rowA, int rowB, float sign)
    {
        return Plane{
            {matrix(rowA, 0) + sign * matrix(rowB, 0),
             matrix(rowA, 1) + sign * matrix(rowB, 1),
             matrix(rowA, 2) + sign * matrix(rowB, 2)},
            matrix(rowA, 3) + sign * matrix(rowB, 3)
        }.normalized();
    }

    std::array<Plane, 6> planes_{};
};

} // namespace peco
