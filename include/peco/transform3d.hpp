#pragma once

#include <peco\matrix4.hpp>
#include <peco\quaternion.hpp>

namespace peco {

class Transform3D {
public:
    Transform3D() = default;
    Transform3D(const Vec3& position, const Quaternion& rotation,
                const Vec3& scale = Vec3::one())
        : position_(position), rotation_(rotation.normalized()), scale_(scale) {}

    const Vec3& position() const { return position_; }
    const Quaternion& rotation() const { return rotation_; }
    const Vec3& scale() const { return scale_; }
    void setPosition(const Vec3& value) { position_ = value; }
    void setRotation(const Quaternion& value) { rotation_ = value.normalized(); }
    void setScale(const Vec3& value) { scale_ = value; }
    void translate(const Vec3& amount) { position_ += amount; }
    void rotate(const Quaternion& amount) { rotation_ = (amount * rotation_).normalized(); }

    Vec3 right() const { return rotation_.rotate(Vec3::unitX()); }
    Vec3 up() const { return rotation_.rotate(Vec3::unitY()); }
    Vec3 forward() const { return rotation_.rotate({0.0f, 0.0f, -1.0f}); }
    Vec3 transformPoint(const Vec3& point) const
    {
        return position_ + rotation_.rotate(point.hadamard(scale_));
    }
    Vec3 transformDirection(const Vec3& direction) const
    {
        return rotation_.rotate(direction);
    }
    Vec3 inverseTransformPoint(const Vec3& point) const
    {
        const Vec3 local = rotation_.inverse().rotate(point - position_);
        return {
            scale_.x != 0.0f ? local.x / scale_.x : 0.0f,
            scale_.y != 0.0f ? local.y / scale_.y : 0.0f,
            scale_.z != 0.0f ? local.z / scale_.z : 0.0f
        };
    }
    Mat4 matrix() const
    {
        const Quaternion q = rotation_.normalized();
        const float xx = q.x * q.x;
        const float yy = q.y * q.y;
        const float zz = q.z * q.z;
        const float xy = q.x * q.y;
        const float xz = q.x * q.z;
        const float yz = q.y * q.z;
        const float wx = q.w * q.x;
        const float wy = q.w * q.y;
        const float wz = q.w * q.z;

        Mat4 result = Mat4::identity();
        result(0, 0) = (1.0f - 2.0f * (yy + zz)) * scale_.x;
        result(0, 1) = (2.0f * (xy - wz)) * scale_.y;
        result(0, 2) = (2.0f * (xz + wy)) * scale_.z;
        result(1, 0) = (2.0f * (xy + wz)) * scale_.x;
        result(1, 1) = (1.0f - 2.0f * (xx + zz)) * scale_.y;
        result(1, 2) = (2.0f * (yz - wx)) * scale_.z;
        result(2, 0) = (2.0f * (xz - wy)) * scale_.x;
        result(2, 1) = (2.0f * (yz + wx)) * scale_.y;
        result(2, 2) = (1.0f - 2.0f * (xx + yy)) * scale_.z;
        result(0, 3) = position_.x;
        result(1, 3) = position_.y;
        result(2, 3) = position_.z;
        return result;
    }
    Transform3D combined(const Transform3D& parent) const
    {
        return {
            parent.transformPoint(position_),
            (parent.rotation_ * rotation_).normalized(),
            parent.scale_.hadamard(scale_)
        };
    }

private:
    Vec3 position_{};
    Quaternion rotation_{};
    Vec3 scale_{1.0f, 1.0f, 1.0f};
};

} // namespace peco
