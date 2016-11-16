#include <stdio.h>
#include "parser.c"

///////////////////////////////////////////////////////////////
// BEGINNING OF RAYCASTING FUNCTION
///////////////////////////////////////////////////////////////

Object* lights[128];
int light = 0;

static inline double sqr(double v) {
	return v*v;
}

static inline void normalize(double* v) {
	double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
	v[0] /= len;
	v[1] /= len;
	v[2] /= len;
}

double dot(double* a, double* b) {
	double result;
	result = a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
	return result;
}

double magnitude(double* v){
	return sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
}

void scale(double* v, double s){
	v[0] *= s;
	v[1] *= s;
	v[2] *= s;
}

void mult(double* v, double* w){
	v[0] *= w[0];
	v[1] *= w[1];
	v[2] *= w[2];
}

void subtract(double* v1, double* v2){
	v1[0] -= v2[0];
	v1[1] -= v2[1];
	v1[2] -= v2[2];
}

void addition(double* v1, double* v2){
	v1[0] += v2[0];
	v1[1] += v2[1];
	v1[2] += v2[2];
}

void reflect(double* v, double* n, double* r){
	double dotResult = dot(v, n);
	dotResult *= 2;
	double nNew[3] = {
		n[0],
		n[1],
		n[2]
	};
	scale(nNew, dotResult);
	r[0] = v[0];
	r[1] = v[1];
	r[2] = v[2];
	subtract(r, nNew);
}

void refract(double* v, double* n, double ior, double* R){
	double angle = acos(dot(v, n));
	double angle2 = sqr(1/ior) * (1-sqr(cos(angle)));
	double new[3] = {n[0], n[1], n[2]};
	double s = (1/ior) * cos(angle) - sqrt(1- angle2);
	scale(new, s);
	double vnew[3] = {v[0], v[1], v[2]};
	scale(vnew, (1/ior));
	for (int i = 0; i < 3; i++){
		R[i] = vnew[i] + new[i];
	}
}

double clamp(double number){
	// scaling number to 255 range, then clamping
	number *= 255;
	if (number < 0) {
		return 0;
	}
	else if (number > 255){
		return 255;
	}
	else {
		return number;
	}
}

void collect_lights(){
	// loops through my object array and puts lights in special light array
	int i = 0;
	for (i = 0; object_array[i] != 0; i++) {
		if (object_array[i]->kind == 3){
			lights[light] = object_array[i];
			light++;
		}
	}
}

//////////////////////////////////////////////////////////
// LIGHTING EQUATIONS                                   //
//////////////////////////////////////////////////////////

double fangular(double* Vo, double* Vl, double a1, double angle) {
	double dotResult = dot(Vo, Vl);
	if (acos(dotResult) > angle / 2) {
		return 0;
	} 
	else {
		return pow(dotResult, a1);
	}
}

double fradial(double a2, double a1, double a0, double d) {
	double quotient = a2 * sqr(d) + a1 * d + a0;
	if (quotient == 0) {
		return 0;
	}
	if (d == INFINITY) {
		return 1;
	} else {
		return 1.0 / quotient;
	}
}

double diffuse_l(double Kd, double Il, double* N, double* L) {
	double dotResult = dot(N, L);
	if (dotResult > 0) {
		return Kd * Il * dotResult;
	} else {
		return 0;
	}
}

double specular_l(double Ks, double Il, double* V, double* R, double* N, double* L, double ns) {
	double dotResult = dot(V, R);
	if (dotResult > 0 && dot(N, L) > 0) {
		return Ks * Il * pow(dotResult, ns);
	} else {
		return 0;
	}
}

double cylinder_intersection(double* Ro, double* Rd,
			     double* C, double r) {
  // Step 1. Find the equation for the object you are
  // interested in..  (e.g., cylinder)
  //
  // x^2 + z^2 = r^2
  //
  // Step 2. Parameterize the equation with a center point
  // if needed
  //
  // (x-Cx)^2 + (z-Cz)^2 = r^2
  //
  // Step 3. Substitute the eq for a ray into our object
  // equation.
  //
  // (Rox + t*Rdx - Cx)^2 + (Roz + t*Rdz - Cz)^2 - r^2 = 0
  //
  // Step 4. Solve for t.
  //
  // Step 4a. Rewrite the equation (flatten).
  //
  // -r^2 +
  // t^2 * Rdx^2 +
  // t^2 * Rdz^2 +
  // 2*t * Rox * Rdx -
  // 2*t * Rdx * Cx +
  // 2*t * Roz * Rdz -
  // 2*t * Rdz * Cz +
  // Rox^2 -
  // 2*Rox*Cx +
  // Cx^2 +
  // Roz^2 -
  // 2*Roz*Cz +
  // Cz^2 = 0
  //
  // Steb 4b. Rewrite the equation in terms of t.
  //
  // t^2 * (Rdx^2 + Rdz^2) +
  // t * (2 * (Rox * Rdx - Rdx * Cx + Roz * Rdz - Rdz * Cz)) +
  // Rox^2 - 2*Rox*Cx + Cx^2 + Roz^2 - 2*Roz*Cz + Cz^2 - r^2 = 0
  //
  // Use the quadratic equation to solve for t..
  double a = (sqr(Rd[0]) + sqr(Rd[2]));
  double b = (2 * (Ro[0] * Rd[0] - Rd[0] * C[0] + Ro[2] * Rd[2] - Rd[2] * C[2]));
  double c = sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0]) + sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2]) - sqr(r);

  double det = sqr(b) - 4 * a * c;
  if (det < 0) return -1;

  det = sqrt(det);
  
  double t0 = (-b - det) / (2*a);
  if (t0 > 0) return t0;

  double t1 = (-b + det) / (2*a);
  if (t1 > 0) return t1;

  return -1;
}

double sphere_intersection(double* Ro, double* Rd,
			     double* C, double r) {

	// SAME IDEA AS ABOVE, BUT INCLUDING A Y COMPONENT
	double a = (sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]));
	double b = (2*(Ro[0]*Rd[0] - Rd[0]*C[0] + Ro[1]*Rd[1] - Rd[1]*C[1] + Ro[2]*Rd[2] - Rd[2]*C[2]));
	double c = sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0]) + Ro[1] - 2*Ro[1]*C[1] + sqr(C[1]) + sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2]) - sqr(r);

	double det = sqr(b) - 4 * a * c;
	if (det < 0) return -1;

	det = sqrt(det);
	  
	double t0 = (-b - det) / (2*a);
	if (t0 > 0) return t0;

	double t1 = (-b + det) / (2*a);
	if (t1 > 0) return t1;

	return -1;

}

double plane_intersection(double* Ro, double* Rd,
			     double* C, double* N) {

	// USING EQUATIONS FOUND IN THE READING
	double subtract[3];
	subtract[0] = C[0]-Ro[0];
	subtract[1] = C[1]-Ro[1];
	subtract[2] = C[2]-Ro[2];
	double dot1 = N[0]*subtract[0] + N[1]*subtract[1] + N[2]*subtract[2];
	double dot2 = N[0]*Rd[0] + N[1]*Rd[1] + N[2]*Rd[2];
	
	return dot1/dot2;
}


void reflections(double* Ro, double* Rd, double* Rdc, Object* obj, double* ocolor, int depth){
	
	double color[3] = {0,0,0};
	int best;
	double best_t = INFINITY;
	for (int i=0; object_array[i] != 0; i++) {
		if (object_array[i] == obj){
			continue;
		}
		double t = 0;
		switch(object_array[i]->kind) {
			case 0:
				// pass, its a camera
				break;
			case 1:
				// CHECK INTERSECTION FOR SPHERE
				t = sphere_intersection(Ro, Rd,
						object_array[i]->sphere.position,
						object_array[i]->sphere.radius);
				break;
			case 2:
				// CHECK INTERSECTION FOR PLANE
				t = plane_intersection(Ro, Rd,
					object_array[i]->plane.position,
					object_array[i]->plane.normal);
				break;
			case 3:
				// pass, its a light
				break;
			default:
				// Horrible error
				exit(1);
		}
		if (t > 0 && t < best_t) {
			best_t = t;
			best = i;	
		}
	}
	double Ron[3] = {
		best_t * Rd[0] + Ro[0],
		best_t * Rd[1] + Ro[1],
		best_t * Rd[2] + Ro[2]
		};
	int closest_shadow_object;
	// shadow test loop, doesnt work
	if (best_t > 0 && best_t != INFINITY) {
		for (int i = 0; lights[i] != NULL; i++){
			double Rdn[3] = {
				lights[i]->light.position[0] - Ron[0],
				lights[i]->light.position[1] - Ron[1],
				lights[i]->light.position[2] - Ron[2]
				};
			normalize(Rdn);
			int t = 0;
			closest_shadow_object = 0;
			for (int j = 0; object_array[j] != 0; j++){
				double t = 0;
				if (object_array[j] == object_array[best]){
					continue;
				}
				switch(object_array[j]->kind) {
					case 0:
						// pass, its a camera
						break;
					case 1:
						// CHECK INTERSECTION FOR SPHERE
						t = sphere_intersection(Ron, Rdn,
								object_array[j]->sphere.position,
								object_array[j]->sphere.radius);
						break;
					case 2:
						// CHECK INTERSECTION FOR PLANE
						t = plane_intersection(Ron, Rdn,
							object_array[j]->plane.position,
							object_array[j]->plane.normal);
						break;
					case 3:
						// pass, its a light
						break;
					default:
						// Horrible error
						exit(1);
				}
				if (t > 0 && t < magnitude(Rdn)){
					closest_shadow_object = 1;
					break;
				}
			}
			if (closest_shadow_object == 0){
				// HOLDER VARIABLE FOR CLOSEST OBJECTS COLOR
				color[0] = 0;
				color[1] = 0;
				color[2] = 0;
				// same variable setting for light equation from project 3
				double N[3];
				if (object_array[best]->kind == 1){
					N[0] = Ron[0] - object_array[best]->sphere.position[0];
					N[1] = Ron[1] - object_array[best]->sphere.position[1];
					N[2] = Ron[2] - object_array[best]->sphere.position[2];
				}
				else if (object_array[best]->kind == 2){
					N[0] = object_array[best]->plane.normal[0];
					N[1] = object_array[best]->plane.normal[1];
					N[2] = object_array[best]->plane.normal[2];
				}
				normalize(N);
				double L[3] = {Rdn[0], Rdn[1], Rdn[2]};
				normalize(L);
				double nL[3] = {-L[0], -L[1], -L[2]};
				// reflected vector
				double R[3];
				reflect(L, N, R);
				// vector from camera
				double V[3] = {Rdc[0], Rdc[1], Rdc[2]};
				// position vector of light
				double pos[3] = {lights[i]->light.position[0],
					lights[i]->light.position[1],
					lights[i]->light.position[2]};
				// finding distance of light from object
				subtract(pos, Ron);
				double d = magnitude(pos);
				double col;
				// finds color using lighting equations from previous project
				for (int c = 0; c < 3; c++) {
					 col = 1;
					 if (lights[i]->light.angular != INFINITY && lights[i]->light.theta != 0) {
						col *= fangular(nL, lights[i]->light.direction, lights[i]->light.angular, (lights[i]->light.theta)*0.0174533);
					 }
					 if (lights[i]->light.radial[0] != INFINITY) {
						col *= fradial(lights[i]->light.radial[2], lights[i]->light.radial[1], lights[i]->light.radial[0], d);
					 }
					 if (object_array[best]->kind == 1){
						col *= (diffuse_l(object_array[best]->sphere.diffuse[c], lights[i]->light.color[c], N, L) + (specular_l(object_array[best]->sphere.specular[c], lights[i]->light.color[c], V, R, N, L, 20)));
						color[c] += col;
					 }
					 else if (object_array[best]->kind == 2){
						col *= (diffuse_l(object_array[best]->plane.diffuse[c], lights[i]->light.color[c], N, L) + (specular_l(object_array[best]->plane.specular[c], lights[i]->light.color[c], V, R, N, L, 20)));
						color[c] += col;
					 }
					 // makes sure colors are in correct range
					 color[c] = clamp(color[c]);
				}
			}
			// base case, only allowing a depth of 7 reflections
			if (depth < 7){
				double N[3];
				// setting new normal vector to find reflected vector
				if (object_array[best]->kind == 1){
					N[0] = Ron[0] - object_array[best]->sphere.position[0];
					N[1] = Ron[1] - object_array[best]->sphere.position[1];
					N[2] = Ron[2] - object_array[best]->sphere.position[2];
				}
				else if (object_array[best]->kind == 2){
					N[0] = Ron[0] - object_array[best]->plane.normal[0];
					N[1] = Ron[1] - object_array[best]->plane.normal[1];
					N[2] = Ron[2] - object_array[best]->plane.normal[2];
				}
				normalize(N);
				double Rdn[3];
				// REFLECTING THINGS
				double reflected[3];
				if (object_array[best]->kind == 1){
					if (object_array[best]->sphere.reflectivity > 0){
						// find reflected vector, then normalize
						reflect(Rd, N, Rdn);
						normalize(Rdn);
						// recursive call
						reflections(Ron, Rdn, Rdc, object_array[best], reflected, depth++);
					}
				}
				else if (object_array[best]->kind == 1){
					if (object_array[best]->plane.reflectivity > 0){
						// find reflected vector, then normalize
						reflect(Rd, N, Rdn);
						normalize(Rdn);
						// recursive call
						reflections(Ron, Rdn, Rdc, object_array[best], reflected, depth++);
					}
				}
				for (int i = 0; i < 3; i++){
					// add reflected color to the current color
					if (object_array[best]->kind == 1){
						color[i] += object_array[best]->sphere.reflectivity * reflected[i];
					}
					if (object_array[best]->kind == 2){
						color[i] += object_array[best]->plane.reflectivity * reflected[i];
					}
				}
			}
		}
	}
	// set output color to found reflective color
	ocolor[0] = color[0];
	ocolor[1] = color[1];
	ocolor[2] = color[2];
}

int main(int argc, char **argv) {

	// OPEN FILE
	FILE* output;
	output = fopen(argv[4], "wb+");
	// I SHOULD ERROR CHECK HERE FOR THE CORRECT TYPE OF OUTPUT FILE, BUT I'M OUT OF TIME
	// WRITING HEADER INFO
	fprintf(output, "P3\n");
	fprintf(output, "%d %d\n%d\n", atoi(argv[1]), atoi(argv[2]), 255);

	// READING JSON OBJECTS INTO ARRAY  
	read_scene(argv[3]);
	collect_lights();
	int i = 0;
	double w;
	double h;
	// FINDING CAMERA TO SET WIDTH AND HEIGHT VARIABLES
	while(1){
		if (object_array[i]->kind == 0){
			w = object_array[i]->camera.width;
			h = object_array[i]->camera.height;
			printf("Camera found and variables set.\n");
			break;
		}
		i++;
	}
	// camera position
	double cx = 0;
	double cy = 0;

	// picture width and height
	int M = atoi(argv[2]);
	int N = atoi(argv[1]);

	double pixheight = h / M;
	double pixwidth = w / N;
	
	// initializing color vector
	double color[3] = {0,0,0};
	
	// DECREMENTING Y COMPONENT TO FLIP PICTURE 
	for (int y = M; y > 0; y--) {
		for (int x = 0; x < N; x += 1) {
			double Ro[3] = {0, 0, 0};
			double Rd[3] = {
				cx - (w/2) + pixwidth * (x + 0.5),
				cy - (h/2) + pixheight * (y + 0.5),
				1
			};
			normalize(Rd);
			// first recursive call, which will return a color vector for that pixel
			reflections(Ro, Rd, Rd, NULL, color, 0);
			// CREATING A PIXEL
			Pixel new;
			// SETTING PIXELS COLOR TO CLOSEST OBJECTS COLOR
			new.red = color[0];
			new.green = color[1];
			new.blue = color[2];
			// WRITING TO FILE IMMEDIATELY
			fprintf(output, "%i %i %i ", new.red, new.green, new.blue);
    	}
  	}
  	fclose(output);
  	return 0;
}
