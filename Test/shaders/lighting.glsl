// Lighting utilities
#define MAX_LIGHTS 8

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform Light lights[MAX_LIGHTS];
uniform int numLights;

vec3 calculateLighting(vec3 worldPos, vec3 normal) {
    vec3 totalLight = vec3(0.0);
    
    for(int i = 0; i < numLights; i++) {
        vec3 lightDir = normalize(lights[i].position - worldPos);
        float diff = max(dot(normal, lightDir), 0.0);
        totalLight += lights[i].color * lights[i].intensity * diff;
    }
    
    return totalLight;
}
