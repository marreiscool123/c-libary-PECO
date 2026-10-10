#pragma once

#include <array>
#include <cmath>

#include <peco\vector2.hpp>

namespace peco {

class Mat3 {
public:
    Mat3() : values_{} {}

    static Mat3 identity()
    {
        Mat3 matrix;
        matrix(0, 0) = matrix(1, 1) = matrix(2, 2) = 1.0f;
        return matrix;
    }
    static Mat3 translation(const Vec2& offset)
    {
        Mat3 matrix = identity();
        matrix(0, 2) = offset.x;
        matrix(1, 2) = offset.y;
        return matrix;
    }
    static Mat3 scale(const Vec2& amount)
    {
        Mat3 matrix = identity();
        matrix(0, 0) = amount.x;
        matrix(1, 1) = amount.y;
        return matrix;
    }
    static Mat3 rotation(float radians)
    {
        Mat3 matrix = identity();
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        matrix(0, 0) = cosine;
        matrix(0, 1) = -sine;
        matrix(1, 0) = sine;
        matrix(1, 1) = cosine;
        return matrix;
    }

    constexpr float& operator()(int row, int column)
    {
        return values_[static_cast<std::size_t>(row * 3 + column)];
    }
    constexpr float operator()(int row, int column) const
    {
        return values_[static_cast<std::size_t>(row * 3 + column)];
    }
    Mat3 operator*(const Mat3& rhs) const
    {
        Mat3 result;
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                for (int index = 0; index < 3; ++index) {
                    result(row, column) += (*this)(row, index) * rhs(index, column);
                }
            }
        }
        return result;
    }
    Vec2 transformPoint(const Vec2& point) const
    {
        const float x = (*this)(0, 0) * point.x + (*this)(0, 1) * point.y + (*this)(0, 2);
        const float y = (*this)(1, 0) * point.x + (*this)(1, 1) * point.y + (*this)(1, 2);
        const float w = (*this)(2, 0) * point.x + (*this)(2, 1) * point.y + (*this)(2, 2);
        return w == 0.0f ? Vec2{x, y} : Vec2{x / w, y / w};
    }
    constexpr Vec2 transformVector(const Vec2& vector) const
    {
        return {
            (*this)(0, 0) * vector.x + (*this)(0, 1) * vector.y,
            (*this)(1, 0) * vector.x + (*this)(1, 1) * vector.y
        };
    }
    float determinant() const
    {
        return (*this)(0, 0) * ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1))
             - (*this)(0, 1) * ((*this)(1, 0) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 0))
             + (*this)(0, 2) * ((*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0));
    }
    bool inverse(Mat3& result, float epsilon = 0.000001f) const
    {
        const float det = determinant();
        if (std::fabs(det) <= epsilon) {
            return false;
        }
        const float reciprocal = 1.0f / det;
        result(0, 0) = ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1)) * reciprocal;
        result(0, 1) = ((*this)(0, 2) * (*this)(2, 1) - (*this)(0, 1) * (*this)(2, 2)) * reciprocal;
        result(0, 2) = ((*this)(0, 1) * (*this)(1, 2) - (*this)(0, 2) * (*this)(1, 1)) * reciprocal;
        result(1, 0) = ((*this)(1, 2) * (*this)(2, 0) - (*this)(1, 0) * (*this)(2, 2)) * reciprocal;
        result(1, 1) = ((*this)(0, 0) * (*this)(2, 2) - (*this)(0, 2) * (*this)(2, 0)) * reciprocal;
        result(1, 2) = ((*this)(0, 2) * (*this)(1, 0) - (*this)(0, 0) * (*this)(1, 2)) * reciprocal;
        result(2, 0) = ((*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0)) * reciprocal;
        result(2, 1) = ((*this)(0, 1) * (*this)(2, 0) - (*this)(0, 0) * (*this)(2, 1)) * reciprocal;
        result(2, 2) = ((*this)(0, 0) * (*this)(1, 1) - (*this)(0, 1) * (*this)(1, 0)) * reciprocal;
        return true;
    }
    Mat3 transposed() const
    {
        Mat3 result;
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                result(row, column) = (*this)(column, row);
            }
        }
        return result;
    }
    const std::array<float, 9>& data() const { return values_; }

private:
    std::array<float, 9> values_;
};

} // namespace peco
