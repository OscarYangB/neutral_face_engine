#include "vector.h"
#include <cmath>
#include <cassert>

bool Vector2::operator==(const Vector2& other) const {
	return x == other.x && y == other.y;
}

Vector2 Vector2::operator+(const Vector2& other) const {
	return Vector2{x + other.x, y + other.y};
}

Vector2 Vector2::operator-(const Vector2& other) const {
	return Vector2{x - other.x, y - other.y};
}

float Vector2::operator*(const Vector2& other) const {
	return x * other.x + y * other.y;
}

Vector2& Vector2::operator+=(const Vector2& other) {
	this->x += other.x;
	this->y += other.y;
	return *this;
}

Vector2& Vector2::operator-=(const Vector2& other) {
	this->x -= other.x;
	this->y -= other.y;
	return *this;
}

Vector2 Vector2::normalized() const {
	assert(!(x == 0.f && y == 0.f));
	float mag = magnitude();
	return Vector2{x / mag, y / mag};
}

float Vector2::magnitude() const {
	return std::sqrt(x * x + y * y);
}

float Vector2::distance(const Vector2& first, const Vector2& second) {
	return (first - second).magnitude();
}

Vector2 Vector2::lerp(const Vector2& first, const Vector2& second, float scalar) {
	return { std::lerp(first.x, second.x, scalar), std::lerp(first.y, second.y, scalar) };
}

// ================================================


bool Vector3::operator==(const Vector3& other) const {
	return x == other.x && y == other.y && z == other.z;
}

Vector3 Vector3::operator+(const Vector3& other) const {
	return Vector3{x + other.x, y + other.y, z + other.z};
}

Vector3 Vector3::operator-(const Vector3& other) const {
	return Vector3{x - other.x, y - other.y, z - other.z};
}

Vector3& Vector3::operator+=(const Vector3& other) {
	this->x += other.x;
	this->y += other.y;
	this->z += other.z;
	return *this;
}

Vector3& Vector3::operator-=(const Vector3& other) {
	this->x -= other.x;
	this->y -= other.y;
	this->z -= other.z;
	return *this;
}

Vector3 Vector3::normalized() const {
	assert(!(x == 0.f && y == 0.f && z == 0.f));
	float mag = magnitude();
	return Vector3{x / mag, y / mag, z / mag};
}

float Vector3::magnitude() const {
	return std::sqrt(x * x + y * y + z * z);
}

float Vector3::distance(const Vector3& first, const Vector3& second) {
	return (first - second).magnitude();
}

Vector3 Vector3::lerp(const Vector3& first, const Vector3& second, float scalar) {
	return { std::lerp(first.x, second.x, scalar), std::lerp(first.y, second.y, scalar), std::lerp(first.z, second.z, scalar) };
}

float Vector3::dot(const Vector3& first, const Vector3& second) {
	return first.x * second.x + first.y * second.y + first.z * second.z;
}

Vector3 Vector3::cross(const Vector3& first, const Vector3& second) {
	return { first.y * second.z - first.z * second.y,
			 first.z * second.x - first.x * second.z,
			 first.x * second.y - first.y * second.x};
}
