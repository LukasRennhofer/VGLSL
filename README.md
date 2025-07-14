# VGLSL

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C99](https://img.shields.io/badge/C-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Platform](https://img.shields.io/badge/Platform-Cross--platform-green.svg)](https://github.com/LukasRennhofer/VGLSL)

A C99 single-header GLSL Extension library that extends GLSL with powerful preprocessing capabilities including includes, macros, and conditional compilation.

Originally developed for the [Vantor Engine](https://github.com/LukasRennhofer/Vantor), VGLSL provides essential preprocessing features missing from standard GLSL, making shader development more modular and maintainable.

## Features

- **Include System** - `#include` directive support with relative path resolution
- **Virtual Include Paths** - Map virtual paths to real directories (`#include <Vantor/Shader.vglsl>`)
- **Macro System** - Simple defines and function-like macros with parameters
- **Conditional Compilation** - `#ifdef`, `#ifndef`, `#else`, `#endif` support
- **Comment Removal** - Automatic removal of line (`//`) and block (`/* */`) comments
- **Error Reporting** - Detailed error messages with line numbers and file names
- **Configurable** - Customizable preprocessing options
- **Zero Dependencies** - Single header file, no external dependencies
- **Memory Safe** - Proper memory management with cleanup functions

## Basic Usage

```c
#define VGLSL_IMPLEMENTATION
#include "vglsl.h"

int main() {
    // Parse a shader file with includes
    VglslResult result = vglsl_parse_file("shader.vglsl", "shaders/");
    
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
```

## Advanced Configuration

```c
// Custom configuration
VglslConfig config = vglsl_default_config();
config.base_path = "assets/shaders/";
config.preserve_lines = true;        // Keep #line directives for debugging
config.remove_comments = false;      // Preserve comments
config.max_include_depth = 16;       // Custom include depth limit

VglslResult result = vglsl_parse_file_ex("main.vglsl", &config);
```

## Core Functions

| Function | Description |
|----------|-------------|
| `vglsl_parse_file(filename, base_path)` | Parse GLSL file with includes |
| `vglsl_parse_file_ex(filename, config)` | Parse file with custom configuration |
| `vglsl_parse_memory(source, filename)` | Parse GLSL from memory |
| `vglsl_parse_memory_ex(source, filename, config)` | Parse memory with custom config |
| `vglsl_free_result(result)` | Free result memory |
| `vglsl_default_config()` | Get default configuration |
| `vglsl_add_virtual_include_path(virtual_name, real_path)` | Map virtual path to real directory |
| `vglsl_remove_virtual_include_path(virtual_name)` | Remove virtual path mapping |
| `vglsl_clear_virtual_include_paths()` | Clear all virtual path mappings |

## GLSL Extensions

### Include System
```glsl
#version 330 core

#include "common.glsl"      // Include common definitions
#include "lighting.glsl"    // Include lighting functions

void main() {
    // Use functions from included files
    vec3 lighting = calculateLighting(worldPos, normal);
    gl_FragColor = vec4(lighting, 1.0);
}
```

### Macro Definitions
```glsl
// Simple macros
#define PI 3.14159265359
#define MAX_LIGHTS 8

// Function-like macros
#define TRANSFORM_VERTEX(pos) (u_mvpMatrix * vec4(pos, 1.0))
#define CLAMP01(x) clamp(x, 0.0, 1.0)

void main() {
    float angle = PI * 2.0;
    gl_Position = TRANSFORM_VERTEX(a_position);
}
```

### Conditional Compilation
```glsl
#define ENABLE_SHADOWS
#define QUALITY_HIGH

#ifdef ENABLE_SHADOWS
    uniform sampler2D shadowMap;
#endif

#ifdef QUALITY_HIGH
    #define SAMPLE_COUNT 16
#else
    #define SAMPLE_COUNT 4
#endif

void main() {
    #ifdef ENABLE_SHADOWS
        float shadow = sampleShadowMap(shadowCoord);
    #else
        float shadow = 1.0;
    #endif
}
```

### Compile-time Configuration

```c
#define VGLSL_MAX_LINE_LENGTH 4096      // Max line length
#define VGLSL_MAX_INCLUDE_DEPTH 32      // Max include depth
#define VGLSL_MAX_VIRTUAL_PATHS 32      // Max virtual includes
#define VGLSL_MAX_DEFINES 256            // Max macro definitions
#define VGLSL_MAX_OUTPUT_SIZE (1024*1024) // Max output size

// Custom memory allocators
#define VGLSL_MALLOC custom_malloc
#define VGLSL_FREE custom_free
#define VGLSL_REALLOC custom_realloc

#define VGLSL_IMPLEMENTATION
#include "vglsl.h"
```

## Error Handling

VGLSL provides detailed error information for debugging:

```c
VglslResult result = vglsl_parse_file("shader.vglsl", "./");

if (!result.success) {
    printf("Error in file: %s\n", result.error_file);
    printf("Line number: %d\n", result.error_line);
    printf("Description: %s\n", result.error_message);
}

vglsl_free_result(&result);
```

### Virtual Include Paths

Virtual include paths allow you to map logical names to physical directories, making your shader code more organized and portable.

```c
// Setup virtual include paths
vglsl_add_virtual_include_path("Vantor", "/path/to/vantor/shaders");
vglsl_add_virtual_include_path("Engine", "/path/to/engine/shaders");
vglsl_add_virtual_include_path("Game", "/path/to/game/shaders");

// Process Shader files ...

// Clear all virtual paths
vglsl_clear_virtual_include_paths();
```

Now you can use angle bracket includes with virtual paths:

```glsl
#version 330 core

// Virtual includes (resolved using registered paths)
#include <Vantor/VLighting.vglsl>      // -> /path/to/vantor/shaders/VLighting.vglsl
#include <Engine/Transform.vglsl>      // -> /path/to/engine/shaders/Transform.vglsl
#include <Game/PlayerEffects.vglsl>    // -> /path/to/game/shaders/PlayerEffects.vglsl
```
## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Contact

**Lukas Rennhofer**
- GitHub: [@LukasRennhofer](https://github.com/LukasRennhofer)
- Project: [VGLSL](https://github.com/LukasRennhofer/VGLSL)

## 🙏 Acknowledgments

- Thanks to the GLSL and OpenGL communities for inspiration
- Single-header library design inspired by the stb libraries
- Created for the [Vantor Engine](https://github.com/LukasRennhofer/Vantor)

---

**Happy shader preprocessing! ✨**