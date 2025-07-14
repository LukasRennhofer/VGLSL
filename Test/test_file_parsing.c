#define VGLSL_IMPLEMENTATION
#include "../vglsl.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

/* Test utilities */
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        printf("Running test: %s\n", #name); \
        tests_run++; \
        if (test_##name()) { \
            tests_passed++; \
            printf("  ✓ PASSED\n"); \
        } else { \
            printf("  ✗ FAILED\n"); \
        } \
    } while(0)

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            printf("    Assertion failed: %s\n", #expr); \
            return false; \
        } \
    } while(0)

#define ASSERT_STR_CONTAINS(haystack, needle) \
    do { \
        if (strstr((haystack), (needle)) == NULL) { \
            printf("    String does not contain \"%s\"\n", needle); \
            printf("    Full output:\n%s\n", haystack); \
            return false; \
        } \
    } while(0)

/* Test file parsing with includes */
static bool test_file_parsing_with_includes() {
    VglslResult result = vglsl_parse_file("shaders/vertex.vglsl", "shaders/");
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.output != NULL);
    
    /* Should contain content from common.glsl */
    ASSERT_STR_CONTAINS(result.output, "#define PI 3.14159265359");
    ASSERT_STR_CONTAINS(result.output, "uniform mat4 u_modelMatrix;");
    ASSERT_STR_CONTAINS(result.output, "vec4 transformVertex(vec3 position)");
    
    /* Should contain content from lighting.glsl */
    ASSERT_STR_CONTAINS(result.output, "#define MAX_LIGHTS 8");
    ASSERT_STR_CONTAINS(result.output, "struct Light");
    ASSERT_STR_CONTAINS(result.output, "vec3 calculateLighting");
    
    /* Should contain the main vertex shader code */
    ASSERT_STR_CONTAINS(result.output, "in vec3 a_position;");
    ASSERT_STR_CONTAINS(result.output, "gl_Position = transformVertex(a_position);");
    
    vglsl_free_result(&result);
    return true;
}

/* Test fragment shader file parsing */
static bool test_fragment_shader_parsing() {
    VglslResult result = vglsl_parse_file("shaders/fragment.vglsl", "shaders/");
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.output != NULL);
    
    /* Should include common definitions */
    ASSERT_STR_CONTAINS(result.output, "#define PI 3.14159265359");
    ASSERT_STR_CONTAINS(result.output, "uniform mat4 u_modelMatrix;");
    
    /* Should include lighting functions */
    ASSERT_STR_CONTAINS(result.output, "vec3 calculateLighting");
    ASSERT_STR_CONTAINS(result.output, "#define MAX_LIGHTS 8");
    
    /* Should contain fragment shader specific code */
    ASSERT_STR_CONTAINS(result.output, "uniform sampler2D u_texture;");
    ASSERT_STR_CONTAINS(result.output, "FragColor = vec4(finalColor, texColor.a);");
    
    vglsl_free_result(&result);
    return true;
}

/* Test file parsing with custom config */
static bool test_file_parsing_custom_config() {
    VglslConfig config = vglsl_default_config();
    config.base_path = "shaders/";
    config.preserve_lines = true;
    
    VglslResult result = vglsl_parse_file_ex("shaders/vertex.vglsl", &config);
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.output != NULL);
    
    /* Should still contain the included content */
    ASSERT_STR_CONTAINS(result.output, "vec4 transformVertex(vec3 position)");
    ASSERT_STR_CONTAINS(result.output, "gl_Position = transformVertex(a_position);");
    
    vglsl_free_result(&result);
    return true;
}

/* Test error handling for non-existent file */
static bool test_nonexistent_file() {
    VglslResult result = vglsl_parse_file("nonexistent.glsl", "./");
    
    ASSERT_TRUE(!result.success);
    ASSERT_TRUE(result.error_message != NULL);
    ASSERT_TRUE(result.output == NULL);
    
    vglsl_free_result(&result);
    return true;
}

/* Test error handling for non-existent include */
static bool test_nonexistent_include() {
    const char* source = 
        "#version 330 core\n"
        "#include \"nonexistent.glsl\"\n"
        "void main() {}";
    
    VglslResult result = vglsl_parse_memory(source, "test.glsl");
    
    ASSERT_TRUE(!result.success);
    ASSERT_TRUE(result.error_message != NULL);
    
    vglsl_free_result(&result);
    return true;
}

int main() {
    printf("Running VGLSL file parsing tests...\n\n");
    
    TEST(file_parsing_with_includes);
    TEST(fragment_shader_parsing);
    TEST(file_parsing_custom_config);
    TEST(nonexistent_file);
    TEST(nonexistent_include);
    
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("All file parsing tests passed! ✓\n");
        return 0;
    } else {
        printf("Some file parsing tests failed! ✗\n");
        return 1;
    }
}
