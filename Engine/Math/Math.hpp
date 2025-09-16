#pragma once

#include <array>

class Math {

public:
	struct Vec2 { float x, y; };
	struct Vec3 { float x, y, z; };
	struct Vec4 { float x, y, z, w; };

	struct Mat2 { std::array<float, 4> values; };
	struct Mat3 { std::array<float, 9> values; };
	struct Mat4 { std::array<float, 16> values; };

	// Vector
	// Length normalize dot cross
	static float Length(const Vec2& vect);
	static float Length(const Vec3& vect);
	static float Length(const Vec4& vect);

	static float Dot(const Vec2& A, const Vec2& B);
	static float Dot(const Vec3& A, const Vec3& B);
	static float Dot(const Vec4& A, const Vec4& B);

	static Vec2 Normalize(const Vec2& vect);
	static Vec3 Normalize(const Vec3& vect);
	static Vec4 Normalize(const Vec4& vect);

	static Vec3 Cross(const Vec3& A, const Vec3& B);

	// Matrix
	// Identity, Translate, Scale, Rotate, Perspective, LookAt
	static Mat2 Identity2();
	static Mat3 Identity3();
	static Mat4 Identity4();

	static Mat4 Translate(const Vec3& t);
	static Mat4 Scale(const Vec3& s);
	static Mat4 Rotate(float rad, const Vec3& axis);

	static Mat4 Translate(const Mat4& mat, const Vec3& t);
	static Mat4 Scale(const Mat4& mat, const Vec3& s);
	static Mat4 Rotate(const Mat4& mat, float rad, const Vec3& axis);

	static Mat4 Perspective(float fov_rad, float aspect, float near_Z, float far_Z);
	static Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up);



};