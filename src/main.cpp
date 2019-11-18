#include "rtv1.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <chrono>
#include <thread>

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "extern/stb_image_write.h"

constexpr int WIN_WIDTH = 1200;
constexpr int WIN_HEIGHT = 600;

float FltRand() {
	static time_t seed;

	if ( seed == 0 ) {
		seed = time( NULL );
	}
	seed = 214013 * seed + 2531011;
	return ( ( (float)( ( seed >> 16 ) & 0x7FFF ) ) / 32767 );
}

Vec3 RandomInUnitSphere() {
	Vec3 p;
	do {
		p = Vec3( FltRand(), FltRand(), FltRand() ) * 2.0f - Vec3( 1, 1, 1 );
	} while ( p.SqLength() >= 1.0f );
	return p;
}

bool Sphere::Hit( const Ray & r, float t_min, float t_max, HitRecord & rec ) const {
	Vec3	oc = r.origin - center;
	float	a = Vec3::Dot( r.direction, r.direction );
	float	b = Vec3::Dot( oc, r.direction );
	float	c = Vec3::Dot( oc, oc ) - radius * radius;
	float	discriminant = b * b - a * c;
	if ( discriminant > 0 ) {
		float temp = ( -b - sqrtf( discriminant ) ) / a;
		if ( temp < t_max && temp > t_min ) {
			rec.t = temp;
			rec.p = r.PointAtParameter( temp );
			rec.normal = ( rec.p - center ) / radius;
			return true;
		}
		temp = ( -b + sqrtf( discriminant ) ) / a;
		if ( temp < t_max && temp > t_min ) {
			rec.t = temp;
			rec.p = r.PointAtParameter( temp );
			rec.normal = ( rec.p - center ) / radius;
			return true;
		}
	}
	return false;
}

Vec3 PhongShading( const Ray & r, const HitRecord & rec, const Material & mat ) {
	static Vec3 light( -5, 5, 5 );
	static Vec3 lightColor( 255, 255, 255 );

	Vec3	l = Vec3::Normalize( light - rec.p );
	float	angle = Vec3::Dot( Vec3::Normalize( rec.normal ), l );
	if ( angle < 0 ) {
		return mat.color * mat.ambiant;
	}
	float d = Vec3::Dot( Vec3::Normalize( r.direction ), l - ( rec.normal * ( 2 * angle ) ) );
	float specularFactor = 0;
	if ( d > 0 ) {
		specularFactor = powf( d, 50 ) * mat.specular;
	}
	return mat.color * mat.ambiant + mat.color * ( angle * mat.diffuse ) + lightColor * specularFactor;
}

Vec3 computeRayColor( const Ray & r, Sphere * spheres, size_t numSpheres ) {
	HitRecord	rec;
	HitRecord	tempRec;
	Sphere *	sphereHit = nullptr;
	float		closestSoFar = FLT_MAX;
	for ( size_t i = 0; i < numSpheres; i++ ) {
		Sphere * s = spheres + i;
		if ( s->Hit( r, 0.001f, closestSoFar, tempRec ) ) {
			sphereHit = s;
			closestSoFar = tempRec.t;
			rec = tempRec;
		}
	}
	if ( sphereHit != nullptr ) {
		return PhongShading( r, rec, sphereHit->material );
	}
	// Hit skybox
	Vec3	unitDirection = Vec3::Normalize( r.direction );
	float	t = 0.5f * ( unitDirection.y + 1.0f );
	return Vec3( 255, 255, 255 ) * ( 1.0f - t ) + Vec3( 0.5f, 0.7f, 1.0f ) * 255 * t;
}

void DrawImage( const Camera & cam, Sphere * spheres, size_t numSpheres, uint8 * imageData, int startX, int stopX, int startY, int stopY ) {
	constexpr int samples = 20;

	for ( int y = startY; y < stopY; y++ ) {
		for ( int x = startX; x < stopX; x++ ) {
			Vec3 color;
			for ( int s = 0; s < samples; s++ ) {
				float	u = ( (float)x + FltRand() - 0.5f ) / (float)WIN_WIDTH;
				float	v = ( (float)y + FltRand() - 0.5f ) / (float)WIN_HEIGHT;
				Ray		r = cam.GetRay( u, v );
				color += computeRayColor( r, spheres, numSpheres );
			}
			color = color / samples;
			imageData[ 0 ] = ( uint8 )( color.x );
			imageData[ 1 ] = ( uint8 )( color.y );
			imageData[ 2 ] = ( uint8 )( color.z );
			imageData[ 3 ] = 255;
			imageData += 4;
		}
	}
}

void PrintUsage() {
	fprintf( stderr, "Usage: ./rtv1 [ --threads [num_threads] ] " );
}

//---------------------------------------------------------------------------
//  Main loop
//---------------------------------------------------------------------------
int main( int argc, char * argv[] ) {
	int numThreads = 0;
	if ( argc > 1 ) {
		if ( strcmp( argv[ 1 ], "--threads" ) == 0 && argc >= 3 ) {
			numThreads = atoi( argv[ 2 ] );
		} else {
			PrintUsage();
			return 1;
		}
	}

	Camera cam( Vec3( 0, 2, -3 ), Vec3( 0, 0, 0 ), Vec3( 0, 1, 0 ), 90, (float)WIN_WIDTH / (float)WIN_HEIGHT );

	Material mat1;
	mat1.ambiant = 0.2f;
	mat1.diffuse = 0.5f;
	mat1.specular = 0.3f;
	mat1.color = Vec3( 0.8f, 0.8f, 0.0f ) * 255.0f;
	Material mat2 = mat1;
	mat2.color = Vec3( 0.8f, 0.3f, 0.3f ) * 255.0f;

	size_t		numSpheres = 20 * 20;
	Sphere *	spheres = new Sphere[ numSpheres ];
	for ( int x = 0; x < 20; x++ ) {
		for ( int y = 0; y < 20; y++ ) {
			spheres[ y + x * 20 ].center = Vec3( x - 12.0f, .0f, y - 4.0f );
			spheres[ y + x * 20 ].radius = 0.20f;
			spheres[ y + x * 20 ].material = mat1;
		}
	}

	uint8 * imageData = new uint8[ WIN_HEIGHT * WIN_WIDTH * 4 ];

	auto startTime = std::chrono::high_resolution_clock::now();

	if ( numThreads > 0 ) {
		std::thread * threads = new std::thread[ numThreads ];
		for ( int i = 0; i < numThreads; i++ ) {
			int startY = i * WIN_HEIGHT / numThreads;
			threads[ i ] = std::thread( DrawImage, cam, spheres, numSpheres, imageData + startY * WIN_WIDTH * 4, 0, WIN_WIDTH, startY, ( i + 1 ) * WIN_HEIGHT / numThreads );
		}

		for ( int i = 0; i < numThreads; i++ ) {
			threads[ i ].join();
		}
		delete[] threads;
	} else {
		DrawImage( cam, spheres, numSpheres, imageData, 0, WIN_WIDTH, 0, WIN_HEIGHT );
	}

	auto endTime = std::chrono::high_resolution_clock::now();
	printf( "Total time: %lldms\n", std::chrono::duration_cast< std::chrono::milliseconds >( endTime - startTime ).count() );

	// write resulting image as PNG
	stbi_flip_vertically_on_write( 1 );
	stbi_write_png( "output.png", WIN_WIDTH, WIN_HEIGHT, 4, imageData, WIN_WIDTH * 4 );

	delete[] spheres;
	delete[] imageData;
	return 0;
}
