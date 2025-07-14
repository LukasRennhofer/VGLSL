#define VGLSL_IMPLEMENTATION
#include "vglsl.h"

int main() {
    // Parse a shader file with includes
    VglslResult result = vglsl_parse_file("Examples/shaders/shader.vglsl", "Examples/shaders/");
    
    if (result.success) {
        printf("Processed GLSL:\n%s\n", result.output);
        
        // Don't forget to free the result
        vglsl_free_result(&result);
    } else {
        fprintf(stderr, "Error in %s:%d - %s\n", 
                result.error_file, result.error_line, result.error_message);
        vglsl_free_result(&result);
    }
    
    return 0;
}