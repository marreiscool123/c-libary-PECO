#pragma once

#include <peco\matrix4.hpp>
#include <peco\ray.hpp>

namespace peco {

class Camera {
public:
    void setPosition(const Vec3& position) { position_ = position; }
    void setTarget(const Vec3& target) { target_ = target; }
    void setUp(const Vec3& up) { up_ = up; }
    void setFieldOfView(float radians) { fieldOfView_ = radians; }
    void setClipPlanes(float nearPlane, float farPlane)
    {
        nearPlane_ = nearPlane;
        farPlane_ = farPlane;
    }

    const Vec3& position() const { return position_; }
    const Vec3& target() const { return target_; }
    const Vec3& up() const { return up_; }
    Vec3 forward() const { return (target_ - position_).normalized(); }
    Vec3 right() const { return forward().cross(up_).normalized(); }
    Vec3 correctedUp() const { return right().cross(forward()).normalized(); }
    float fieldOfView() const { return fieldOfView_; }
    float nearPlane() const { return nearPlane_; }
    float farPlane() const { return farPlane_; }
    Mat4 viewMatrix() const;
    Mat4 projectionMatrix(float aspect) const;
    Mat4 viewProjection(float aspect) const;
    void translate(const Vec3& offset) { position_ += offset; target_ += offset; }
    void pan(float horizontal, float vertical)
    {
        const Vec3 offset = right() * horizontal + correctedUp() * vertical;
        translate(offset);
    }
    void dolly(float amount, float minimumDistance = 0.01f)
    {
        const Vec3 offset = position_ - target_;
        const float distance = offset.length();
        const float nextDistance = distance - amount;
        if (distance > 0.0f) {
            position_ = target_ + offset * (std::max(minimumDistance, nextDistance) / distance);
        }
    }
    void orbit(float yawRadians, float pitchRadians,
               float minimumPitch = -1.55334306f, float maximumPitch = 1.55334306f)
    {
        const Vec3 offset = position_ - target_;
        const float radius = offset.length();
        if (radius <= 0.0f) {
            return;
        }
        float yaw = std::atan2(offset.x, offset.z) + yawRadians;
        float pitch = std::asin(std::clamp(offset.y / radius, -1.0f, 1.0f)) + pitchRadians;
        pitch = std::clamp(pitch, minimumPitch, maximumPitch);
        const float horizontal = radius * std::cos(pitch);
        position_ = target_ + Vec3{
            horizontal * std::sin(yaw),
            radius * std::sin(pitch),
            horizontal * std::cos(yaw)
        };
    }
    Ray3 rayFromScreen(const Vec2& pixel, const Vec2& viewport) const
    {
        if (viewport.x <= 0.0f || viewport.y <= 0.0f) {
            return {position_, forward()};
        }
        Mat4 inverse;
        if (!viewProjection(viewport.x / viewport.y).inverse(inverse)) {
            return {position_, forward()};
        }
        const float x = 2.0f * pixel.x / viewport.x - 1.0f;
        const float y = 1.0f - 2.0f * pixel.y / viewport.y;
        const auto nearClip = inverse.transform(x, y, -1.0f, 1.0f);
        const auto farClip = inverse.transform(x, y, 1.0f, 1.0f);
        if (nearClip[3] == 0.0f || farClip[3] == 0.0f) {
            return {position_, forward()};
        }
        const Vec3 nearPoint{nearClip[0] / nearClip[3],
                             nearClip[1] / nearClip[3],
                             nearClip[2] / nearClip[3]};
        const Vec3 farPoint{farClip[0] / farClip[3],
                            farClip[1] / farClip[3],
                            farClip[2] / farClip[3]};
        return {nearPoint, (farPoint - nearPoint).normalized()};
    }

private:
    Vec3 position_{0.0f, 0.0f, 5.0f};
    Vec3 target_{0.0f, 0.0f, 0.0f};
    Vec3 up_{0.0f, 1.0f, 0.0f};
    float fieldOfView_ = 1.04719755f;
    float nearPlane_ = 0.1f;
    float farPlane_ = 100.0f;
};

} // namespace peco
