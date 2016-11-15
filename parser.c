#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

// OBJECT STRUCTURE THAT ALLOWS FOR ALL 3 OBJECTS
typedef struct {
  int kind; // 0 = camera, 1 = sphere, 2 = plane, 3 = light
  union {
    struct {
      double width;
      double height;
    } camera;
    struct {
      double position[3];
      double radius;
      double diffuse[3];
      double specular[3];
      double reflectivity;
      double refractivity;
      double ior;
    } sphere;
    struct {
      double position[3];
      double normal[3];
      double diffuse[3];
      double specular[3];
      double reflectivity;
      double refractivity;
      double ior;
    } plane;
    struct {
	  double position[3];
	  double color[3];
	  double theta;
	  double direction[3];
	  double radial[3];
	  double angular;
	} light;
  };
} Object;

// PIXEL STRUCTURE CREATED
typedef struct Pixel{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} Pixel;

// OBJECT ARRAY TO READ FROM JSON FILE INTO
Object* object_array[128];
int obj = 0;
int line = 1;

// next_c() wraps the getc() function and provides error checking and line
// number maintenance
int next_c(FILE* json) {
  int c = fgetc(json);
#ifdef DEBUG
  printf("next_c: '%c'\n", c);
#endif
  if (c == '\n') {
    line += 1;
  }
  if (c == EOF) {
    fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
    exit(1);
  }
  return c;
}


// expect_c() checks that the next character is d.  If it is not it emits
// an error.
void expect_c(FILE* json, int d) {
  int c = next_c(json);
  if (c == d) return;
  fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
  exit(1);    
}


// skip_ws() skips white space in the file.
void skip_ws(FILE* json) {
  int c = next_c(json);
  while (isspace(c)) {
    c = next_c(json);
  }
  ungetc(c, json);
}


// next_string() gets the next string from the file handle and emits an error
// if a string can not be obtained.
char* next_string(FILE* json) {
  char buffer[129];
  int c = next_c(json);
  if (c != '"') {
    fprintf(stderr, "Error: Expected string on line %d.\n", line);
    exit(1);
  }  
  c = next_c(json);
  int i = 0;
  while (c != '"') {
    if (i >= 128) {
      fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
      exit(1);      
    }
    if (c == '\\') {
      fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
      exit(1);      
    }
    if (c < 32 || c > 126) {
      fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
      exit(1);
    }
    buffer[i] = c;
    i += 1;
    c = next_c(json);
  }
  buffer[i] = 0;
  return strdup(buffer);
}

double next_number(FILE* json) {
  double value;
  fscanf(json, "%lf", &value);
  // Error check this..
  return value;
}

double* next_vector(FILE* json) {
  double* v = malloc(3*sizeof(double));
  expect_c(json, '[');
  skip_ws(json);
  v[0] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[1] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[2] = next_number(json);
  skip_ws(json);
  expect_c(json, ']');
  return v;
}


void read_scene(char* filename) {
  int c;
  FILE* json = fopen(filename, "r");

  if (json == NULL) {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
    exit(1);
  }
  
  skip_ws(json);
  
  // Find the beginning of the list
  expect_c(json, '[');

  skip_ws(json);

  // Find the objects

  while (1) {
    c = fgetc(json);
    if (c == ']') {
      fprintf(stderr, "Error: This is the worst scene file EVER.\n");
      fclose(json);
      return;
    }
    if (c == '{') {
      skip_ws(json);
    
      // Parse the object
      char* key = next_string(json);
      if (strcmp(key, "type") != 0) {
		fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
		exit(1);
      }

      skip_ws(json);

      expect_c(json, ':');

      skip_ws(json);

      char* value = next_string(json);
  	  object_array[obj] = malloc(sizeof(Object));
	  Object new;
			// IDENTIFYING OBJECT TYPES AND BUILDING OBJECT
      if (strcmp(value, "camera") == 0) {
				(*object_array[obj]).kind = 0;
				printf("Found camera\n");
      } else if (strcmp(value, "sphere") == 0) {
				(*object_array[obj]).kind = 1;
				printf("Found sphere\n");
      } else if (strcmp(value, "plane") == 0) {
				(*object_array[obj]).kind = 2;
				printf("Found plane\n");
      } else if (strcmp(value, "light") == 0) {
				(*object_array[obj]).kind = 3;
				printf("Found light\n");
      } else {
				fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
				exit(1);
      }

      skip_ws(json);

      while (1) {
		c = next_c(json);
		if (c == '}') {
	  	// stop parsing this object
	  		//object_array[obj] = &new;
			obj++;
	  		break;
	  } else if (c == ',') {
	  	// read another field
		  skip_ws(json);
		  char* key = next_string(json);
		  skip_ws(json);
		  expect_c(json, ':');
		  skip_ws(json);
	// BUILDING OBJECT DOUBLE FIELDS
	if ((strcmp(key, "width") == 0) ||
	  (strcmp(key, "height") == 0) ||
	  (strcmp(key, "radius") == 0) ||
	  (strcmp(key, "theta") == 0) ||
	  (strcmp(key, "radial-a2") == 0) ||
	  (strcmp(key, "radial-a1") == 0) ||
	  (strcmp(key, "radial-a0") == 0) ||
	  (strcmp(key, "angular-a0") == 0) ||
	  (strcmp(key, "reflectivity") == 0) ||
	  (strcmp(key, "refractivity") == 0) ||
	  (strcmp(key, "ior") == 0)) {
	    	double value = next_number(json);
			if(strcmp(key, "width") == 0){
				if((*object_array[obj]).kind == 0) (*object_array[obj]).camera.width = value;
			}
			else if(strcmp(key, "height") == 0){
				if((*object_array[obj]).kind == 0) (*object_array[obj]).camera.height = value;
			}
			else if(strcmp(key, "radius") == 0){
				(*object_array[obj]).sphere.radius = value;
			}
			else if(strcmp(key, "theta") == 0) {
				(*object_array[obj]).light.theta = value;
			}
			else if(strcmp(key, "radial-a2") == 0) {
				(*object_array[obj]).light.radial[2] = value;
			}
			else if(strcmp(key, "radial-a1") == 0) {
				(*object_array[obj]).light.radial[1] = value;
			}
			else if(strcmp(key, "radial-a0") == 0) {
				(*object_array[obj]).light.radial[0] = value;
			}
			else if(strcmp(key, "angular-a0") == 0) {
				(*object_array[obj]).light.angular = value;
			}
			else if(strcmp(key, "reflectivity") == 0){
				if((*object_array[obj]).kind == 1) {
					(*object_array[obj]).sphere.reflectivity = value;
				}
				else if ((*object_array[obj]).kind == 2) {
					(*object_array[obj]).plane.reflectivity = value;
				}
			}
			else if(strcmp(key, "refractivity") == 0){
				if((*object_array[obj]).kind == 1) {
					(*object_array[obj]).sphere.refractivity = value;
				}
				else if ((*object_array[obj]).kind == 2) {
					(*object_array[obj]).plane.refractivity = value;
				}
			}
			else if(strcmp(key, "ior") == 0){
				if((*object_array[obj]).kind == 1) {
					(*object_array[obj]).sphere.ior = value;
				}
				else if ((*object_array[obj]).kind == 2) {
					(*object_array[obj]).plane.ior = value;
				}
			}
		// BUILDING OBJECT VECTOR FIELDS
	  	} else if ((strcmp(key, "color") == 0) ||
		     (strcmp(key, "position") == 0) ||
		     (strcmp(key, "normal") == 0) ||
		     (strcmp(key, "diffuse_color") == 0) ||
		     (strcmp(key, "specular_color") == 0) ||
		     (strcmp(key, "direction") == 0)) {
	    	double* value = next_vector(json);
			if(strcmp(key, "color") == 0){
				(*object_array[obj]).light.color[0] = value[0];
				(*object_array[obj]).light.color[1] = value[1];
				(*object_array[obj]).light.color[2] = value[2];
			}
			else if(strcmp(key, "position") == 0){
				if((*object_array[obj]).kind == 1){
					(*object_array[obj]).sphere.position[0] = value[0];
					(*object_array[obj]).sphere.position[1] = value[1];
					(*object_array[obj]).sphere.position[2] = value[2];
				}
				else if((*object_array[obj]).kind == 2){
					(*object_array[obj]).plane.position[0] = value[0];
					(*object_array[obj]).plane.position[1] = value[1];
					(*object_array[obj]).plane.position[2] = value[2];
				}
				else if((*object_array[obj]).kind == 3){
					(*object_array[obj]).light.position[0] = value[0];
					(*object_array[obj]).light.position[1] = value[1];
					(*object_array[obj]).light.position[2] = value[2];
				}
			}
			else if(strcmp(key, "normal") == 0){
				(*object_array[obj]).plane.normal[0] = value[0];
				(*object_array[obj]).plane.normal[1] = value[1];
				(*object_array[obj]).plane.normal[2] = value[2];
			}
			else if(strcmp(key, "diffuse_color") == 0){
				if((*object_array[obj]).kind == 1){
					(*object_array[obj]).sphere.diffuse[0] = value[0];
					(*object_array[obj]).sphere.diffuse[1] = value[1];
					(*object_array[obj]).sphere.diffuse[2] = value[2];
				}
				else if((*object_array[obj]).kind == 2){
					(*object_array[obj]).plane.diffuse[0] = value[0];
					(*object_array[obj]).plane.diffuse[1] = value[1];
					(*object_array[obj]).plane.diffuse[2] = value[2];
				}
			}
			else if(strcmp(key, "specular_color") == 0){
				if((*object_array[obj]).kind == 1){
					(*object_array[obj]).sphere.specular[0] = value[0];
					(*object_array[obj]).sphere.specular[1] = value[1];
					(*object_array[obj]).sphere.specular[2] = value[2];
				}
				else if((*object_array[obj]).kind == 2){
					(*object_array[obj]).plane.specular[0] = value[0];
					(*object_array[obj]).plane.specular[1] = value[1];
					(*object_array[obj]).plane.specular[2] = value[2];
				}
			}
			else if(strcmp(key, "direction") == 0) {
				(*object_array[obj]).light.direction[0] = value[0];
				(*object_array[obj]).light.direction[1] = value[1];
				(*object_array[obj]).light.direction[2] = value[2];
			}
		// ERROR CHECK
	  } else {
	    fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
		    key, line);
	    //char* value = next_string(json);
	  }
	  skip_ws(json);
	} else {
	  fprintf(stderr, "Error: Unexpected value on line %d\n", line);
	  exit(1);
	}
      }
      skip_ws(json);
      c = next_c(json);
      if (c == ',') {
		// noop
		skip_ws(json);
		} else if (c == ']') {
		fclose(json);
		object_array[obj] = NULL;
		return;
     } else {
		fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
		exit(1);
     }
    }
  }
}
