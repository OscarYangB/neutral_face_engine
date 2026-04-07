#pragma once

#include <array>
#include <string>
#include <cassert>
#include "vector.h"

/*
"Row-major order"
[0][0] [0][1] [0][2] [0][3]
[1][0] [1][1] [1][2] [1][3]
[2][0] [2][1] [2][2] [2][3]
[3][0] [3][1] [3][2] [3][3]
*/

struct Matrix {
	std::array<std::array<float, 4>, 4> data{};

	Matrix() = default;
	Matrix(std::initializer_list<std::initializer_list<float>> args);
	Matrix operator*(const Matrix& other) const;

	static Matrix translate(const Matrix& matrix, const Vector3& translation);
	static Matrix rotate(const Matrix& matrix, float angle, const Vector3& axis);
	static Matrix scale(const Matrix& matrix, const Vector3& scaling);
	static Matrix scale(const Matrix& matrix, float scaling);
	static Matrix look_at(const Vector3& eye, const Vector3& center);
	static Matrix perspective(float angle, float aspect_ratio, float near_clip, float far_clip);

	static constexpr Matrix zero() { return Matrix(); }
	static constexpr Matrix identity() { return {{1.f,0.f,0.f,0.f},{0.f,1.f,0.f,0.f},{0.f,0.f,1.f,0.f},{0.f,0.f,0.f,1.f}}; }

	template<typename T>
	std::array<float, 4>& operator[](T index) {
		assert(index < 4);
		return data[index];
	}

	template<typename T>
	const std::array<float, 4>& operator[](T index) const{
		return data[index];
	}

	template<typename T>
	Matrix operator*(T other) const {
		float other_float = static_cast<float>(other);
		Matrix matrix = *this;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				matrix[i][j] = data[i][j] * other_float;
			}
		}
	}

	std::string debug_print();
};
