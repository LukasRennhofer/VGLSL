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

#define ASSERT_STR_EQUALS(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("    Expected: \"%s\"\n", expected); \
            printf("    Actual:   \"%s\"\n", actual); \
            return false; \
        } \
    } while(0)

#define ASSERT_STR_CONTAINS(haystack, needle) \
    do { \
        if (strstr((haystack), (needle)) == NULL) { \
            printf("    String \"%s\" does not contain \"%s\"\n", haystack, needle); \
            return false; \
        } \
    } while(0)

/* Test basic memory parsing */
static bool test_basic_memory_parsing() {
    const char* source = "#version 330 core\nvoid main() {\n    gl_Position = vec4(0.0);\n}";
    VglslResult result = vglsl_parse_memory(source, "test.glsl");
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.output != NULL);
    ASSERT_STR_CONTAINS(result.output, "#version 330 core");
    ASSERT_STR_CONTAINS(result.output, "gl_Position = vec4(0.0);");
    
    vglsl_free_result(&result);
    return true;
}

/* Test simple defines */
static bool test_simple_defines() {
    const char* source = 
        "#define PI 3.14159\n"
        "float radius = PI * 2.0;";
    
    VglslResult result = vglsl_parse_memory(source, "test.glsl");
    
    ASSERT_TRUE(result.success);
    ASSERT_STR_CONTAINS(result.output, "float radius = 3.14159 * 2.0;");
    
    vglsl_free_result(&result);
    return true;
}

/* Test function-like macros */
static bool test_function_macros() {
    const char* source = 
        "#define MAX(a, b) ((a) > (b) ? (a) : (b))\n"
        "float value = MAX(x, y);";
    
    VglslResult result = vglsl_parse_memory(source, "test.glsl");
    
    ASSERT_TRUE(result.success);
    ASSERT_STR_CONTAINS(result.output, "float value = ((x) > (y) ? (x) : (y));");
    
    vglsl_free_result(&result);
    return true;
}

/* Test ifdef/ifndef conditionals */
static bool test_ifdef_conditionals() {
    const char* source = 
        "#define DEBUG\n"
        "#ifdef DEBUG\n"
        "// Debug code here\n"
        "float debug_value = 1.0;\n"
        "#endif\n"
        "#ifndef RELEASE\n"
        "float non_release = 2.0;\n"
        "#endif";
    
    VglslResult result = vglsl_parse_memory(source, "test.glsl");
    
    ASSERT_TRUE(result.success);
    ASSERT_STR_CONTAINS(result.output, "float debug_value = 1.0;");
    ASSERT_STR_CONTAINS(result.output, "float non_release = 2.0;");
    
    vglsl_free_result(&result);
    return true;
}

/* Test ifdef/else/endif */
static bool test_ifdef_else() {
    const char* source = 
        "#ifdef UNDEFINED_MACRO\n"
        "float if_value = 1.0;\n"
        "#else\n"
        "float else_value = 2.0;\n"
        "#endif";
    
    VglslResult result = vglsl_parse_memory(source, "test.glsl");
    
    ASSERT_TRUE(result.success);
    ASSERT_STR_CONTAINS(result.output, "float else_value = 2.0;");
    /* Should not contain the if branch */
    ASSERT_TRUE(strstr(result.output, "float if_value = 1.0;") == NULL);
    
    vglsl_free_result(&result);
    return true;
}

/* Test undef directive */
static bool test_undef() {
    const char* source = 
        "#define TEST_MACRO 42\n"
        "int before = TEST_MACRO;\n"
        "#undef TEST_MACRO\n"
        "int after = TEST_MACRO;";  /* This should remain unexpanded */
    
    VglslResult result = vglsl_parse_memory(source, "test.glsl");
    
    ASSERT_TRUE(result.success);
    ASSERT_STR_CONTAINS(result.output, "int before = 42;");
    ASSERT_STR_CONTAINS(result.output, "int after = TEST_MACRO;");
    
    vglsl_free_result(&result);
    return true;
}

/* Test comment removal */
static bool test_comment_removal() {
    const char* source = 
        "// Line comment\n"
        "float value = 1.0; // End of line comment\n"
        "/* Block comment */\n"
        "float other = 2.0; /* Inline block */ float third = 3.0;\n"
        "/* Multi-line\n"
        "   block comment */\n"
        "float final = 4.0;";
    
    VglslResult result = vglsl_parse_memory(source, "test.glsl");
    
    ASSERT_TRUE(result.success);
    ASSERT_STR_CONTAINS(result.output, "float value = 1.0;");
    ASSERT_STR_CONTAINS(result.output, "float other = 2.0;");
    ASSERT_STR_CONTAINS(result.output, "float third = 3.0;");
    ASSERT_STR_CONTAINS(result.output, "float final = 4.0;");
    
    /* Should not contain comments */
    ASSERT_TRUE(strstr(result.output, "//") == NULL);
    ASSERT_TRUE(strstr(result.output, "/*") == NULL);
    ASSERT_TRUE(strstr(result.output, "*/") == NULL);
    
    vglsl_free_result(&result);
    return true;
}

/* Test nested conditionals */
static bool test_nested_conditionals() {
    const char* source = 
        "#define OUTER\n"
        "#define INNER\n"
        "#ifdef OUTER\n"
        "float outer = 1.0;\n"
        "#ifdef INNER\n"
        "float nested = 2.0;\n"
        "#endif\n"
        "float outer_end = 3.0;\n"
        "#endif";
    
    VglslResult result = vglsl_parse_memory(source, "test.glsl");
    
    ASSERT_TRUE(result.success);
    ASSERT_STR_CONTAINS(result.output, "float outer = 1.0;");
    ASSERT_STR_CONTAINS(result.output, "float nested = 2.0;");
    ASSERT_STR_CONTAINS(result.output, "float outer_end = 3.0;");
    
    vglsl_free_result(&result);
    return true;
}

/* Test complex macro expansion */
static bool test_complex_macros() {
    const char* source = 
        "#define TRANSFORM_VERTEX(pos) (u_mvpMatrix * pos)\n"
        "#define DECLARE_UNIFORM(type, name) uniform type name\n"
        "DECLARE_UNIFORM(mat4, u_mvpMatrix);\n"
        "void main() {\n"
        "    gl_Position = TRANSFORM_VERTEX(a_position);\n"
        "}";
    
    VglslResult result = vglsl_parse_memory(source, "test.glsl");
    
    ASSERT_TRUE(result.success);
    ASSERT_STR_CONTAINS(result.output, "uniform mat4 u_mvpMatrix;");
    ASSERT_STR_CONTAINS(result.output, "gl_Position = (u_mvpMatrix * a_position);");
    
    vglsl_free_result(&result);
    return true;
}

/* Test error handling - invalid macro */
static bool test_error_handling() {
    const char* source = 
        "#define INVALID_MACRO(\n"  /* Incomplete macro definition */
        "float value = 1.0;";
    
    VglslResult result = vglsl_parse_memory(source, "test.glsl");
    
    ASSERT_TRUE(!result.success);
    ASSERT_TRUE(result.error_message != NULL);
    ASSERT_TRUE(result.error_line > 0);
    
    vglsl_free_result(&result);
    return true;
}

/* Test default configuration */
static bool test_default_config() {
    VglslConfig config = vglsl_default_config();
    
    ASSERT_TRUE(config.base_path != NULL);
    ASSERT_TRUE(strcmp(config.base_path, "./") == 0);
    ASSERT_TRUE(config.remove_comments == true);
    ASSERT_TRUE(config.preserve_lines == false);
    ASSERT_TRUE(config.max_include_depth > 0);
    ASSERT_TRUE(config.max_output_size > 0);
    
    return true;
}

/* Test with custom configuration */
static bool test_custom_config() {
    const char* source = 
        "// This comment should be preserved\n"
        "float value = 1.0;";
    
    VglslConfig config = vglsl_default_config();
    config.remove_comments = false;
    
    VglslResult result = vglsl_parse_memory_ex(source, "test.glsl", &config);
    
    ASSERT_TRUE(result.success);
    ASSERT_STR_CONTAINS(result.output, "// This comment should be preserved");
    
    vglsl_free_result(&result);
    return true;
}

/* Test free result function */
static bool test_free_result() {
    const char* source = "float value = 1.0;";
    VglslResult result = vglsl_parse_memory(source, "test.glsl");
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.output != NULL);
    
    vglsl_free_result(&result);
    
    /* After freeing, pointers should be NULL */
    ASSERT_TRUE(result.output == NULL);
    ASSERT_TRUE(result.error_message == NULL);
    ASSERT_TRUE(result.error_file == NULL);
    ASSERT_TRUE(result.success == false);
    ASSERT_TRUE(result.error_line == 0);
    
    return true;
}

int main() {
    printf("Running VGLSL tests...\n\n");
    
    /* Run all tests */
    TEST(basic_memory_parsing);
    TEST(simple_defines);
    TEST(function_macros);
    TEST(ifdef_conditionals);
    TEST(ifdef_else);
    TEST(undef);
    TEST(comment_removal);
    TEST(nested_conditionals);
    TEST(complex_macros);
    TEST(error_handling);
    TEST(default_config);
    TEST(custom_config);
    TEST(free_result);
    
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("All tests passed! ✓\n");
        return 0;
    } else {
        printf("Some tests failed! ✗\n");
        return 1;
    }
}
