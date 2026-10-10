#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include <limits>

#include <peco\vector3.hpp>

namespace peco {

class Mat4 {
public:
    Mat4() : values_{} {}

    static Mat4 identity()
    {
        Mat4 result;
        for (int i = 0; i < 4; ++i) {
            result(i, i) = 1.0f;
        }
        return result;
    }

    static Mat4 translation(const Vec3& offset)
    {
        Mat4 result = identity();
        result(0, 3) = offset.x;
        result(1, 3) = offset.y;
        result(2, 3) = offset.z;
        return result;
    }

    static Mat4 scale(const Vec3& amount)
    {
        Mat4 result;
        result(0, 0) = amount.x;
        result(1, 1) = amount.y;
        result(2, 2) = amount.z;
        result(3, 3) = 1.0f;
        return result;
    }

    static Mat4 rotationX(float radians)
    {
        Mat4 result = identity();
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        result(1, 1) = c;
        result(1, 2) = -s;
        result(2, 1) = s;
        result(2, 2) = c;
        return result;
    }

    static Mat4 rotationY(float radians)
    {
        Mat4 result = identity();
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        result(0, 0) = c;
        result(0, 2) = s;
        result(2, 0) = -s;
        result(2, 2) = c;
        return result;
    }

    static Mat4 rotationZ(float radians)
    {
        Mat4 result = identity();
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        result(0, 0) = c;
        result(0, 1) = -s;
        result(1, 0) = s;
        result(1, 1) = c;
        return result;
    }

    static Mat4 perspective(float verticalFovRadians, float aspect,
                            float nearPlane, float farPlane)
    {
        Mat4 result;
        if (aspect <= 0.0f || verticalFovRadians <= 0.0f
            || verticalFovRadians >= 3.14159265358979323846f
            || nearPlane <= 0.0f || farPlane <= nearPlane) {
            return result;
        }
        const float f = 1.0f / std::tan(verticalFovRadians * 0.5f);
        result(0, 0) = f / aspect;
        result(1, 1) = f;
        result(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result(2, 3) = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        result(3, 2) = -1.0f;
        return result;
    }

    static Mat4 orthographic(float left, float right, float bottom, float top,
                             float nearPlane, float farPlane)
    {
        Mat4 result;
        if (right == left || top == bottom || farPlane == nearPlane) {
            return result;
        }
        result(0, 0) = 2.0f / (right - left);
        result(1, 1) = 2.0f / (top - bottom);
        result(2, 2) = -2.0f / (farPlane - nearPlane);
        result(0, 3) = -(right + left) / (right - left);
        result(1, 3) = -(top + bottom) / (top - bottom);
        result(2, 3) = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result(3, 3) = 1.0f;
        return result;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up)
    {
        const Vec3 forward = (target - eye).normalized();
        const Vec3 right = forward.cross(up).normalized();
        const Vec3 correctedUp = right.cross(forward).normalized();
        Mat4 result = identity();
        result(0, 0) = right.x;
        result(0, 1) = right.y;
        result(0, 2) = right.z;
        result(0, 3) = -right.dot(eye);
        result(1, 0) = correctedUp.x;
        result(1, 1) = correctedUp.y;
        result(1, 2) = correctedUp.z;
        result(1, 3) = -correctedUp.dot(eye);
        result(2, 0) = -forward.x;
        result(2, 1) = -forward.y;
        result(2, 2) = -forward.z;
        result(2, 3) = forward.dot(eye);
        return result;
    }

    constexpr float& operator()(int row, int column)
    {
        return values_[static_cast<std::size_t>(row * 4 + column)];
    }
    constexpr float operator()(int row, int column) const
    {
        return values_[static_cast<std::size_t>(row * 4 + column)];
    }

    Mat4 operator*(const Mat4& rhs) const
    {
        Mat4 result;
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                for (int k = 0; k < 4; ++k) {
                    result(row, column) += (*this)(row, k) * rhs(k, column);
                }
            }
        }
        return result;
    }

    Mat4 transposed() const
    {
        Mat4 result;
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                result(row, column) = (*this)(column, row);
            }
        }
        return result;
    }

    bool inverse(Mat4& result, float epsilon = 0.000001f) const
    {
        float augmented[4][8]{};
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                augmented[row][column] = (*this)(row, column);
            }
            augmented[row][row + 4] = 1.0f;
        }
        for (int column = 0; column < 4; ++column) {
            int pivot = column;
            for (int row = column + 1; row < 4; ++row) {
                if (std::fabs(augmented[row][column])
                    > std::fabs(augmented[pivot][column])) {
                    pivot = row;
                }
            }
            if (std::fabs(augmented[pivot][column]) <= epsilon) {
                return false;
            }
            if (pivot != column) {
                for (int index = 0; index < 8; ++index) {
                    std::swap(augmented[pivot][index], augmented[column][index]);
                }
            }
            const float divisor = augmented[column][column];
            for (int index = 0; index < 8; ++index) {
                augmented[column][index] /= divisor;
            }
            for (int row = 0; row < 4; ++row) {
                if (row == column) {
                    continue;
                }
                const float factor = augmented[row][column];
                for (int index = 0; index < 8; ++index) {
                    augmented[row][index] -= factor * augmented[column][index];
                }
            }
        }
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                result(row, column) = augmented[row][column + 4];
            }
        }
        return true;
    }

    Vec3 transformPoint(const Vec3& point) const
    {
        const auto value = transform(point.x, point.y, point.z, 1.0f);
        if (value[3] == 0.0f) {
            return {value[0], value[1], value[2]};
        }
        return {value[0] / value[3], value[1] / value[3], value[2] / value[3]};
    }

    Vec3 transformDirection(const Vec3& direction) const
    {
        const auto value = transform(direction.x, direction.y, direction.z, 0.0f);
        return {value[0], value[1], value[2]};
    }

    std::array<float, 4> transform(float x, float y, float z, float w) const
    {
        return {
            (*this)(0, 0) * x + (*this)(0, 1) * y + (*this)(0, 2) * z + (*this)(0, 3) * w,
            (*this)(1, 0) * x + (*this)(1, 1) * y + (*this)(1, 2) * z + (*this)(1, 3) * w,
            (*this)(2, 0) * x + (*this)(2, 1) * y + (*this)(2, 2) * z + (*this)(2, 3) * w,
            (*this)(3, 0) * x + (*this)(3, 1) * y + (*this)(3, 2) * z + (*this)(3, 3) * w
        };
    }

    const std::array<float, 16>& data() const { return values_; }

private:
    std::array<float, 16> values_;
};

} // namespace peco
