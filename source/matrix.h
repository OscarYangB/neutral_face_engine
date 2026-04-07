#pragma once

#include <array>
#include <string>
#include <cassert>

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

	template<typename T>
	std::array<float, 4>& operator[](T index) {
		assert(index < 4);
		return data[index];
	}

	template<typename T>
	const std::array<float, 4>& operator[](T index) const{
		return data[index];
	}

	std::string debug_print();
};
