///*
//*****************************************************************/
///*!
//\file   Math.hpp
//\author Neo HuiZong
//\par    neo.h\@digipen.edu
//\date   September 2025
//\brief
//This file contains the declarations of the functions that are needed for
//the math class.
//Copyright (C) 2025 DigiPen Institute of Technology.
//Reproduction or disclosure of this file or its contents without the
//prior written consent of DigiPen Institute of Technology is prohibited.
//*/
///* *******************************************************************/
//
//#pragma once
//
//#include <array>
//
//class Math {
//
//public:
//	struct Vec2 { float x, y; };
//	struct Vec3 { float x, y, z; };
//	struct Vec4 { float x, y, z, w; };
//
//	struct Mat2 { std::array<float, 4> values; };
//	struct Mat3 { std::array<float, 9> values; };
//	struct Mat4 { std::array<float, 16> values; };
//
//	// Vector
//	// Length normalize dot cross
//	static float Length(const Vec2& vect);
//	static float Length(const Vec3& vect);
//	static float Length(const Vec4& vect);
//
//	static float Dot(const Vec2& A, const Vec2& B);
//	static float Dot(const Vec3& A, const Vec3& B);
//	static float Dot(const Vec4& A, const Vec4& B);
//
//	static Vec2 Normalize(const Vec2& vect);
//	static Vec3 Normalize(const Vec3& vect);
//	static Vec4 Normalize(const Vec4& vect);
//
//	static Vec3 Cross(const Vec3& A, const Vec3& B);
//
//	// Matrix
//	// Identity, Translate, Scale, Rotate, Perspective, LookAt
//	static Mat2 Identity2();
//	static Mat3 Identity3();
//	static Mat4 Identity4();
//
//	static Mat4 Translate(const Vec3& t);
//	static Mat4 Scale(const Vec3& s);
//	static Mat4 Rotate(float rad, const Vec3& axis);
//
//	static Mat4 Translate(const Mat4& mat, const Vec3& t);
//	static Mat4 Scale(const Mat4& mat, const Vec3& s);
//	static Mat4 Rotate(const Mat4& mat, float rad, const Vec3& axis);
//
//	static Mat4 Perspective(float fov_rad, float aspect, float near_Z, float far_Z);
//	static Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
//	static Mat4 Orthographic(float left, float right, float bottom, float top, float near_z, float far_z);
//
//	// Inverse, transpose, determinant, Reflect and Refract
//	static Mat2 Inverse(const Mat2& mat);
//	static Mat3 Inverse(const Mat3& mat);
//	static Mat4 Inverse(const Mat4& mat);
//
//	static Mat2 Transpose(const Mat2& mat);
//	static Mat3 Transpose(const Mat3& mat);
//	static Mat4 Transpose(const Mat4& mat);
//
//	static float Determinant(const Mat2& mat);
//	static float Determinant(const Mat3& mat);
//	static float Determinant(const Mat4& mat);
//
//	static float Distance(const Vec2& A, const Vec2& B);
//	static float Distance(const Vec3& A, const Vec3& B);
//	static float Distance(const Vec4& A, const Vec4& B);
//
//	static Vec3 Reflect(const Vec3& Incident, const Vec3& Normal);
//	static Vec3 Refract(const Vec3& Incident, const Vec3& Normal, float eta);
//};