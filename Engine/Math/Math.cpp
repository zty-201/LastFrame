#include "Math.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

// To convert own vector to glm and vice versa
static glm::vec2 ToGlm(const Math::Vec2& vect) { return { vect.x, vect.y }; }
static Math::Vec2 FromGlm(const glm::vec2& vect) { return { vect.x, vect.y }; }

static glm::vec3 ToGlm(const Math::Vec3& vect) { return { vect.x, vect.y, vect.z }; }
static Math::Vec3 FromGlm(const glm::vec3& vect) { return { vect.x, vect.y, vect.z }; }

static glm::vec4 ToGlm(const Math::Vec4& vect) { return { vect.x, vect.y, vect.z, vect.w }; }
static Math::Vec4 FromGlm(const glm::vec4& vect) { return { vect.x, vect.y, vect.z,vect.w }; }

// To convert own matrix to glm and vice versa
static glm::mat2 ToGlm(const Math::Mat2& mat) { return glm::make_mat2(mat.values.data()); }
static Math::Mat2 FromGlm(const glm::mat2& m) {
    Math::Mat2 result{};
    const float* p = glm::value_ptr(m);
    std::copy(p, p + 4, result.values.begin());
    return result;
}

static glm::mat3 ToGlm(const Math::Mat3& mat) { return glm::make_mat3(mat.values.data()); }
static Math::Mat3 FromGlm(const glm::mat3& m) {
    Math::Mat3 result{};
    const float* p = glm::value_ptr(m);
    std::copy(p, p + 9, result.values.begin());
    return result;
}

static glm::mat4 ToGlm(const Math::Mat4& mat) { return glm::make_mat4(mat.values.data()); }
static Math::Mat4 FromGlm(const glm::mat4& m) {
    Math::Mat4 result{};
    const float* p = glm::value_ptr(m);
    std::copy(p, p + 16, result.values.begin());
    return result;
}

// Vector functions
float Math::Length(const Vec2& vect) {
    return glm::length(ToGlm(vect));
}

float Math::Length(const Vec3& vect) {
    return glm::length(ToGlm(vect));
}

float Math::Length(const Vec4& vect) {
    return glm::length(ToGlm(vect));
}

// Dot functions
float Math::Dot(const Vec2& A, const Vec2& B) {
    return glm::dot(ToGlm(A), ToGlm(B));
}

float Math::Dot(const Vec3& A, const Vec3& B) {
    return glm::dot(ToGlm(A), ToGlm(B));
}

float Math::Dot(const Vec4& A, const Vec4& B) {
    return glm::dot(ToGlm(A), ToGlm(B));
}

// Normalize Functions
Math::Vec2 Math::Normalize(const Vec2& vect) {
    return FromGlm(glm::normalize(ToGlm(vect)));
}

Math::Vec3 Math::Normalize(const Vec3& vect) {
    return FromGlm(glm::normalize(ToGlm(vect)));
}

Math::Vec4 Math::Normalize(const Vec4& vect) {
    return FromGlm(glm::normalize(ToGlm(vect)));
}

Math::Vec3 Math::Cross(const Vec3& A, const Vec3& B) {
    return FromGlm(glm::cross(ToGlm(A), ToGlm(B)));
}

// Matrix functions
//Identity
Math::Mat2 Math::Identity2() {
    return FromGlm(glm::mat2(1.0f));
}

Math::Mat3 Math::Identity3() {
    return FromGlm(glm::mat3(1.0f));
}

Math::Mat4 Math::Identity4() {
    return FromGlm(glm::mat4(1.0f));
}

// Pure transform (start from identity)
Math::Mat4 Math::Translate(const Vec3& t) {
    return FromGlm(glm::translate(glm::mat4(1.0f), ToGlm(t)));
}

Math::Mat4 Math::Scale(const Vec3& s) {
    return FromGlm(glm::scale(glm::mat4(1.0f), ToGlm(s)));
}

Math::Mat4 Math::Rotate(float rad, const Vec3& axis) {
    return FromGlm(glm::rotate(glm::mat4(1.0f), rad, ToGlm(axis)));
}


// Apply to an existing matrix
Math::Mat4 Math::Translate(const Mat4& m, const Vec3& t) {
    return FromGlm(glm::translate(ToGlm(m), ToGlm(t)));
}

Math::Mat4 Math::Scale(const Mat4& m, const Vec3& s) {
    return FromGlm(glm::scale(ToGlm(m), ToGlm(s)));
}

Math::Mat4 Math::Rotate(const Mat4& m, float rad, const Vec3& axis) {
    return FromGlm(glm::rotate(ToGlm(m), rad, ToGlm(axis)));
}


