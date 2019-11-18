#pragma once

#include <math.h>

constexpr float PI = 3.14159265358979323846f;

typedef unsigned char uint8;

struct Vec3 {
	float x;
	float y;
	float z;

	Vec3() : x( 0 ), y( 0 ), z( 0 ) {}
	Vec3( float _x, float _y, float _z ) : x( _x ), y( _y ), z( _z ) {}

	float Length() const { return sqrtf( x * x + y * y + z * z ); }
	float SqLength() const { return x * x + y * y + z * z; }

	Vec3 operator+( const Vec3 & v ) const { return Vec3( x + v.x, y + v.y, z + v.z ); }
	Vec3 operator-( const Vec3 & v ) const { return Vec3( x - v.x, y - v.y, z - v.z ); }
	Vec3 operator*( const Vec3 & v ) const { return Vec3( x * v.x, y * v.y, z * v.z ); }
	Vec3 operator*( float f ) const { return Vec3( x * f, y * f, z * f ); }
	Vec3 operator/( float f ) const { return Vec3( x / f, y / f, z / f ); }
	
	void operator+=( const Vec3 & v ) { x += v.x; y += v.y; z += v.z; }

	static float	Dot( Vec3 a, Vec3 b ) { return a.x * b.x + a.y * b.y + a.z * b.z; }
	static Vec3		Cross( Vec3 a, Vec3 b ) { return Vec3( a.y * b.z - a.z * b.y, -( a.x * b.z - a.z * b.x ), a.x * b.y - a.y * b.x ); }
	static Vec3		Normalize(Vec3 v) { float k = 1.0f / v.Length(); return Vec3(v.x * k, v.y * k, v.z * k); }

	bool IsNormalized() const { return fabs(SqLength() - 1.0f) < 0.01f; }
};

struct Ray {
	Ray() {}
	Ray( Vec3 _origin, Vec3 _direction ) : origin( _origin ), direction( _direction ) {}
	Vec3 origin;
	Vec3 direction;
	
	Vec3 PointAtParameter(float t)  const { return origin + direction * t; }
};

struct Material {
	Vec3 color;
	float ambiant;
	float diffuse;
	float specular;
};

struct HitRecord {
	float t;
	Vec3 p;
	Vec3 normal;
};

struct Sphere {
	Vec3 center;
	float radius;
	Material material;

	bool Hit( const Ray & r, float t_min, float t_max, HitRecord & rec) const;
};

struct		Camera
{
	Camera(Vec3 lookfrom, Vec3 lookat, Vec3 vup, float vfov, float aspect) {
		float theta = vfov * PI / 180;
		float halfHeight = tanf(theta / 2);
		float halfWidth = aspect * halfHeight;
		origin = lookfrom;
		Vec3 w = Vec3::Normalize(lookfrom - lookat);
		Vec3 u = Vec3::Normalize(Vec3::Cross(vup, w));
		Vec3 v = Vec3::Cross(w, u);
		lowerLeftCorner = origin - u * halfWidth - v * halfHeight - w;
		horizontal = u * halfWidth * 2;
		vertical = v * halfHeight * 2;
	}
	Vec3 origin;
	Vec3 lowerLeftCorner;
	Vec3 horizontal;
	Vec3 vertical;
	Vec3 light;

	Ray GetRay(float u, float v) {
		return Ray(origin, lowerLeftCorner + horizontal * u + vertical * v - origin);
	}
};