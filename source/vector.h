#pragma once

struct Vector2 {
	float x = 0.f;
	float y = 0.f;

	bool operator==(const Vector2& other) const;
	Vector2 operator+(const Vector2& other) const;
	Vector2 operator-(const Vector2& other) const;
	float operator*(const Vector2& other) const;
	Vector2& operator+=(const Vector2& other);
	Vector2& operator-=(const Vector2& other);
	Vector2 normalized() const;
	float magnitude() const;

	template<typename T>
	Vector2 operator*(const T other) const {
		const float other_float = static_cast<float>(other);
		return Vector2{x * other_float, y * other_float};
	}

	template<typename T>
	Vector2 operator/(const T other) const {
		const float other_float = static_cast<float>(other);
		return Vector2{x / other_float, y / other_float};
	}

	template<typename T>
	Vector2& operator*=(const T other) {
		const float other_float = static_cast<float>(other);
		this->x *= other_float;
		this->y *= other_float;
		return *this;
	}

	template<typename T>
	Vector2& operator/=(const T other) {
		const float other_float = static_cast<float>(other);
		this->x /= other_float;
		this->y /= other_float;
		return *this;
	}

	static float distance(const Vector2& first, const Vector2& second);
	static Vector2 lerp(const Vector2& first, const Vector2& second, float scalar);

	static constexpr Vector2 zero() { return Vector2{0.f, 0.f}; }
	static constexpr Vector2 one() { return Vector2{1.f, 1.f}; }
	static constexpr Vector2 up() { return Vector2{0.f, 1.f}; }
	static constexpr Vector2 down() { return Vector2{0.f, -1.f}; }
	static constexpr Vector2 left() { return Vector2{-1.f, 0.f}; }
	static constexpr Vector2 right() { return Vector2{1.f, 0.f}; }
};

template<typename T>
Vector2 operator*(const T other, const Vector2& vector) {
	return vector * other;
}

template<typename T>
Vector2 operator/(const T other, const Vector2& vector) {
	return vector * other;
}

struct Vector3 {
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

	bool operator==(const Vector3& other) const;
	Vector3 operator+(const Vector3& other) const;
	Vector3 operator-(const Vector3& other) const;
	Vector3& operator+=(const Vector3& other);
	Vector3& operator-=(const Vector3& other);
	Vector3 normalized() const;
	float magnitude() const;

	inline float r() const { return x; }
	inline float g() const { return y; }
	inline float b() const { return z; }

	template<typename T>
	Vector3 operator*(const T other) const {
		const float other_float = static_cast<float>(other);
		return Vector3{x * other_float, y * other_float, z * other_float};
	}

	template<typename T>
	Vector3 operator/(const T other) const {
		const float other_float = static_cast<float>(other);
		return Vector3{x / other_float, y / other_float, z / other_float};
	}

	template<typename T>
	Vector3& operator*=(const T other) {
		const float other_float = static_cast<float>(other);
		this->x *= other_float;
		this->y *= other_float;
		this->z *= other_float;
		return *this;
	}

	template<typename T>
	Vector3& operator/=(const T other) {
		const float other_float = static_cast<float>(other);
		this->x /= other_float;
		this->y /= other_float;
		this->z /= other_float;
		return *this;
	}

	static float distance(const Vector3& first, const Vector3& second);
	static Vector3 lerp(const Vector3& first, const Vector3& second, float scalar);
	static float dot(const Vector3& first, const Vector3& second);
	static Vector3 cross(const Vector3& first, const Vector3& second);

	static constexpr Vector3 zero() { return Vector3{0.f, 0.f, 0.f}; }
	static constexpr Vector3 one() { return Vector3{1.f, 1.f, 1.f}; }
	static constexpr Vector3 up() { return Vector3{0.f, 0.f, 1.f}; }
	static constexpr Vector3 down() { return Vector3{0.f, 0.f, -1.f}; }
	static constexpr Vector3 i() { return Vector3{1.f, 0.f, 0.f}; }
	static constexpr Vector3 j() { return Vector3{0.f, 1.f, 0.f}; }
	static constexpr Vector3 k() { return Vector3{0.f, 0.f, 1.f}; }
};

template<typename T>
Vector3 operator*(const T other, const Vector3& vector) {
	return vector * other;
}

template<typename T>
Vector3 operator/(const T other, const Vector3& vector) {
	return vector * other;
}
