#pragma once

#include <peco\matrix3.hpp>

namespace peco {

class Transform2D {
public:
    Transform2D() = default;
    Transform2D(const Vec2& position, float rotation, const Vec2& scale = Vec2::one())
        : position_(position), rotation_(rotation), scale_(scale) {}

    const Vec2& position() const { return position_; }
    float rotation() const { return rotation_; }
    const Vec2& scale() const { return scale_; }
    const Vec2& pivot() const { return pivot_; }
    void setPosition(const Vec2& value) { position_ = value; }
    void setRotation(float value) { rotation_ = value; }
    void setScale(const Vec2& value) { scale_ = value; }
    void setPivot(const Vec2& value) { pivot_ = value; }
    void translate(const Vec2& amount) { position_ += amount; }
    void rotate(float amount) { rotation_ += amount; }
    void scaleBy(const Vec2& amount) { scale_ = scale_.hadamard(amount); }

    Vec2 right() const { return Vec2::unitX().rotated(rotation_); }
    Vec2 up() const { return Vec2::unitY().rotated(rotation_); }
    Mat3 matrix() const
    {
        return Mat3::translation(position_)
             * Mat3::rotation(rotation_)
             * Mat3::scale(scale_)
             * Mat3::translation(-pivot_);
    }
    Vec2 transformPoint(const Vec2& point) const { return matrix().transformPoint(point); }
    Vec2 inverseTransformPoint(const Vec2& point) const
    {
        Mat3 inverse;
        const Mat3 world = matrix();
        return world.inverse(inverse) ? inverse.transformPoint(point) : point;
    }
    Vec2 transformDirection(const Vec2& direction) const
    {
        return direction.rotated(rotation_);
    }
    Transform2D combined(const Transform2D& parent) const
    {
        Transform2D result;
        result.position_ = parent.transformPoint(position_);
        result.rotation_ = parent.rotation_ + rotation_;
        result.scale_ = parent.scale_.hadamard(scale_);
        result.pivot_ = pivot_;
        return result;
    }

private:
    Vec2 position_{};
    float rotation_ = 0.0f;
    Vec2 scale_{1.0f, 1.0f};
    Vec2 pivot_{};
};

} // namespace peco
