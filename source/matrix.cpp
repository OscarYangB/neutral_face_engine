#include "matrix.h"

Matrix Matrix::operator*(const Matrix& other) const {
	Matrix result;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result[i][j] = (*this)[i][0] * other[0][j] + (*this)[i][1] * other[1][j] + (*this)[i][2] * other[2][j] + (*this)[i][3] * other[3][j];
		}
	}
	return result;
}

Matrix::Matrix(std::initializer_list<std::initializer_list<float>> args) {
	assert(args.size() == 4);
	int i{};
	for (auto& list : args) {
		assert(list.size() == 4);
		int j{};
		for (float value : list) {
			data[i][j] = value;
			j++;
		}
		i++;
	}
}

std::string Matrix::debug_print() {
	std::string result{};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.append(std::to_string(data[i][j]) + " ");
		}
		result.append("\n");
	}
	return result;
}
