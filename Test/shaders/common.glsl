// Common shader utilities
#define PI 3.14159265359
#define TWO_PI 6.28318530718

uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;

// Function to transform a vertex position
vec4 transformVertex(vec3 position) {
    return u_projectionMatrix * u_viewMatrix * u_modelMatrix * vec4(position, 1.0);
}
