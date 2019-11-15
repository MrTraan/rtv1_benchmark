#include <SDL.h>
#include "rtv1.h"
#include <math.h>
#include <float.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>

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

void PutPixelAt( SDL_Surface * surface, int x, int y, Uint8 r, Uint8 g, Uint8 b ) {
	Uint32 * pixel = (Uint32 *)( surface->pixels ) + x + y * surface->w;
	*pixel = SDL_MapRGBA( surface->format, r, g, b, 255 );
}

bool Sphere::Hit(const Ray & r, float t_min, float t_max, HitRecord & rec) const {
	Vec3 oc = r.origin - center;
	float a = Vec3::Dot( r.direction, r.direction );
	float b = Vec3::Dot( oc, r.direction );
	float c = Vec3::Dot( oc, oc ) - radius * radius;
	float discriminant = b * b - a * c;
	if (discriminant > 0) {
		float temp = ( -b - sqrtf(discriminant) ) / a;
		if ( temp < t_max && temp > t_min ) {
			rec.t = temp;
			rec.p = r.PointAtParameter( temp );
			rec.normal = ( rec.p - center ) / radius;
			return true;
		}
		temp = ( -b + sqrtf(discriminant) ) / a;
		if ( temp < t_max && temp > t_min ) {
			rec.t = temp;
			rec.p = r.PointAtParameter( temp );
			rec.normal = ( rec.p - center ) / radius;
			return true;
		}
	}
	return false;
}

Vec3 PhongShading( const Ray & r, const HitRecord & rec, const Material & mat) {
	static Vec3 light(-5, 5, 5);
	static Vec3 lightColor(255, 255, 255);

	Vec3 l = Vec3::Normalize( light - rec.p );
	float angle = Vec3::Dot( Vec3::Normalize(rec.normal), l );
	if (angle < 0) {
		return mat.color * mat.ambiant;
	}
	float d = Vec3::Dot( Vec3::Normalize(r.direction), l - (rec.normal * (2 * angle) ));
	float specularFactor = 0;
	if (d > 0) {
		specularFactor = powf(d, 50) * mat.specular;	
	}
	return mat.color * mat.ambiant + mat.color * (angle * mat.diffuse) + lightColor * specularFactor;
}

Vec3 computeRayColor( const Ray & r, Sphere * spheres, size_t numSpheres ) {
	HitRecord rec;
	HitRecord tempRec;
	Sphere * sphereHit = nullptr;
	float closestSoFar = FLT_MAX;
	for ( size_t i = 0; i < numSpheres; i++) {
		Sphere * s = spheres + i;
		if ( s->Hit(r, 0.001f, closestSoFar, tempRec) ) {
			sphereHit = s;
			closestSoFar = tempRec.t;
			rec = tempRec;
		}
	}
	if (sphereHit != nullptr) {
		return PhongShading( r, rec, sphereHit->material );
	}
	// Hit skybox
	Vec3	unitDirection = Vec3::Normalize( r.direction );
	float	t = 0.5f * ( unitDirection.y + 1.0f );
	return Vec3( 255, 255, 255 ) * ( 1.0f - t ) + Vec3( 0.5f, 0.7f, 1.0f ) * 255 * t;
}

//---------------------------------------------------------------------------
//  Main loop
//---------------------------------------------------------------------------
int main( int argc, char * argv[] ) {
	// Initialize
	SDL_Window * sdlWindow = nullptr;

	// Initialize SDL
	int rc = SDL_Init( SDL_INIT_VIDEO );
	assert( rc >= 0 );
	(void)rc;

	// Create SDL window
	sdlWindow = SDL_CreateWindow( "CMakeDemo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN );
	assert( sdlWindow );
	SDL_ShowWindow( sdlWindow );

	SDL_Surface * screenSurface = SDL_GetWindowSurface( sdlWindow );

	SDL_LockSurface( screenSurface );

	Camera cam(
		Vec3(0, 2, -3),
		Vec3(0, 0, 0),
		Vec3(0, 1, 0),
		90,
		(float)WIN_WIDTH / (float)WIN_HEIGHT
	);

	Material mat1;
	mat1.ambiant = 0.2;
	mat1.diffuse = 0.5;
	mat1.specular = 0.3;
	mat1.color = Vec3( 0.8, 0.8, 0.0 ) * 255;
	Material mat2 = mat1;
	mat2.color = Vec3( 0.8, 0.3, 0.3 ) * 255;

	size_t numSpheres = 20 * 20;
	Sphere * spheres = new Sphere[ numSpheres ];
	for ( int x = 0; x < 20; x++) {
		for ( int y = 0; y < 20; y++ ) {
			spheres[ y + x * 20 ].center = Vec3(x - 12, 0, y - 4);
			spheres[ y + x * 20 ].radius = 0.20;
			spheres[ y + x * 20 ].material = mat1;
		} 
	}

	constexpr int samples = 20;

	Uint32 startTime = SDL_GetTicks();
	for ( int y = 0; y < WIN_HEIGHT; y++ ) {
		for ( int x = 0; x < WIN_WIDTH; x++ ) {
			Vec3 color;
			for ( int s = 0; s < samples; s++ ) {
				float	u = ( (float)x + FltRand() - 0.5f ) / (float)WIN_WIDTH;
				float	v = ( (float)y + FltRand() - 0.5f ) / (float)WIN_HEIGHT;
				Ray		r = cam.GetRay( u, v );
				color += computeRayColor( r, spheres, numSpheres );
			}
			color = color / samples;
			PutPixelAt( screenSurface, x, WIN_HEIGHT - y - 1, ( Uint8 )( color.x ), ( Uint8 )( color.y ), ( Uint8 )( color.z ) );
		}
	}
	Uint32 endTime = SDL_GetTicks();
	printf("Total time: %dms\n", endTime - startTime);

	SDL_UnlockSurface( screenSurface );
	SDL_UpdateWindowSurface( sdlWindow );

	// Main loop
	Uint32	lastTick = SDL_GetTicks();
	int		running = 1;
	while ( running ) {
		Uint32 now = SDL_GetTicks();
		if ( now - lastTick < 16 ) {
			SDL_Delay( 16 - ( now - lastTick ) );
			now = SDL_GetTicks();
		}
		lastTick = now;

		// Handle SDL events
		SDL_Event event;
		while ( SDL_PollEvent( &event ) ) {
			switch ( event.type ) {
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP: {
					break;
				}

				case SDL_WINDOWEVENT: {
					if ( event.window.event == SDL_WINDOWEVENT_CLOSE ) {
						running = 0;
					}
					break;
				}
			}
		}

	}

	// Shutdown
	SDL_DestroyWindow( sdlWindow );
	SDL_Quit();
	return 0;
}
