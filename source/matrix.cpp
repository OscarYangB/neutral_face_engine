#include "matrix.h"
#include <cmath>

Matrix Matrix::operator*(const Matrix& other) const {
	Matrix result;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result[i][j] = data[i][0] * other[0][j] + data[i][1] * other[1][j] + data[i][2] * other[2][j] + data[i][3] * other[3][j];
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

Matrix Matrix::translate(const Matrix& matrix, const Vector3& translation) {
	Matrix result = matrix;
	result[0][3] += translation.x;
	result[1][3] += translation.y;
	result[2][3] += translation.z;
	return result;
}

Matrix Matrix::rotate(const Matrix& matrix, float angle, const Vector3& axis) {
	Matrix rotation_matrix = Matrix::identity();
	const Vector3 normalized_axis = axis.normalized();
	const float c = std::cos(angle);
	const float s = std::sin(angle);
	const float x = normalized_axis.x;
	const float y = normalized_axis.y;
	const float z = normalized_axis.z;
	// Rodrigues' Rotation Formula
	rotation_matrix[0][0] = x * x * (1 - c) + c;
	rotation_matrix[0][1] = x * y * (1 - c) + c;
	rotation_matrix[0][2] = x * z * (1 - c) + y * s;
	rotation_matrix[1][0] = y * x * (1 - c) + z * s;
	rotation_matrix[1][1] = y * y * (1 - c) + c;
	rotation_matrix[1][2] = y * z * (1 - c) - x * s;
	rotation_matrix[2][0] = x * z * (1 - c) - y * s;
	rotation_matrix[2][1] = y * z * (1 - c) + x * s;
	rotation_matrix[2][2] = z * z * (1 - c) + c;
	return rotation_matrix * matrix;
}

Matrix Matrix::scale(const Matrix& matrix, const Vector3& scaling) {
	Matrix result = matrix;
	result[0][0] *= scaling.x;
	result[1][1] *= scaling.y;
	result[2][2] *= scaling.z;
	return result;
}

Matrix Matrix::scale(const Matrix& matrix, float scaling) {
	return Matrix::scale(matrix, Vector3::one() * scaling);
}

Matrix Matrix::look_at(const Vector3& eye, const Vector3& center) {
	const Vector3 forward = (center - eye).normalized();
	const Vector3 right = Vector3::cross(forward, Vector3::up()).normalized();
	const Vector3 up = Vector3::cross(right, forward);
	Matrix result = identity();
	result[0][0] = right.x;
	result[0][1] = right.y;
	result[0][2] = right.z;
	result[1][0] = up.x;
	result[1][1] = up.y;
	result[1][2] = up.z;
	result[2][0] = -forward.x;
	result[2][1] = -forward.y;
	result[2][2] = -forward.z;
	result[0][3] = -1.f * Vector3::dot(right, eye);
	result[1][3] = -1.f * Vector3::dot(up, eye);
	result[2][3] = 1.f * Vector3::dot(forward, eye);
	return result;
}

Matrix Matrix::perspective(float angle, float aspect_ratio, float near_clip, float far_clip) {
	const float n = near_clip;
	const float f = far_clip;
	const float t = std::tan(angle / 2.f) * n;
	const float b = -t;
	const float r = (2 * t * aspect_ratio) / 2.f;
	const float l = -r;
	Matrix result = Matrix::zero();
	result[0][0] = (2.f * n) / (r - l);
	result[0][2] = (l + r) / (r - l);
	result[1][1] = (-2.f * n) / (t - b);
	result[1][2] = (b + t) / (t - b);
	result[2][2] = (-n - f) / (f - n);
	result[2][3] = (-2.f * n * f) / (f - n);
	result[3][2] = -1.f;
	return result;
}
