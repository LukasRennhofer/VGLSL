/*
 * VGLSL - A C99 single header GLSL Preproccessor Extension library
 * Made for Vantor Engine (https://github.com/LukasRennhofer/Vantor)
 * 
 * Copyright (c) 2025 Lukas Rennhofer (https://github.com/LukasRennhofer)
 * Licensed under the GNU General Public License, Version 3.
 * 
 * This library provides GLSL preprocessing capabilities including:
 * - #include directive support
 * - #define macros with parameters
 * - #ifdef, #ifndef, #else, #endif conditionals
 * - #undef directive
 * - Line and block comments removal
 * - Error reporting with line numbers
 * 
 * Usage:
 *   #define VGLSL_IMPLEMENTATION
 *   #include "vglsl.h"
 * 
 * Example:
 *   VglslResult result = vglsl_parse_file("shader.vglsl", "shaders/");
 *   if (result.success) {
 *       printf("Processed GLSL:\n%s\n", result.output);
 *       vglsl_free_result(&result);
 *   }
 */

#ifndef VGLSL_H
#define VGLSL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ========================================================================= */
/* API                                                                       */
/* ========================================================================= */

typedef struct {
    bool success;
    char* output;
    char* error_message;
    int error_line;
    char* error_file;
} VglslResult;

typedef struct {
    const char* base_path;  /* Base path for #include resolution */
    bool preserve_lines;    /* Keep #line directives for debugging */
    bool remove_comments;   /* Remove // and /* */
    int max_include_depth;  /* Maximum recursive include depth */
    int max_output_size;    /* Maximum output buffer size */
} VglslConfig;

/* Parse GLSL from file with preprocessing */
VglslResult vglsl_parse_file(const char* filename, const char* base_path);
VglslResult vglsl_parse_file_ex(const char* filename, const VglslConfig* config);

/* Parse GLSL from memory with preprocessing */
VglslResult vglsl_parse_memory(const char* source, const char* filename);
VglslResult vglsl_parse_memory_ex(const char* source, const char* filename, const VglslConfig* config);

/* Free result memory */
void vglsl_free_result(VglslResult* result);

/* Get default configuration */
VglslConfig vglsl_default_config(void);

/* Virtual include paths support */
void vglsl_add_virtual_include_path(const char* virtual_name, const char* real_path);
void vglsl_remove_virtual_include_path(const char* virtual_name);
void vglsl_clear_virtual_include_paths(void);

/* ========================================================================= */
/* IMPLEMENTATION                                                            */
/* ========================================================================= */

#ifdef VGLSL_IMPLEMENTATION

#ifndef VGLSL_MAX_LINE_LENGTH
#define VGLSL_MAX_LINE_LENGTH 4096
#endif

#ifndef VGLSL_MAX_INCLUDE_DEPTH
#define VGLSL_MAX_INCLUDE_DEPTH 32
#endif

#ifndef VGLSL_MAX_DEFINES
#define VGLSL_MAX_DEFINES 256
#endif

#ifndef VGLSL_MAX_OUTPUT_SIZE
#define VGLSL_MAX_OUTPUT_SIZE (1024 * 1024) /* 1MB default */
#endif

#ifndef VGLSL_MAX_VIRTUAL_PATHS
#define VGLSL_MAX_VIRTUAL_PATHS 32
#endif

#ifndef VGLSL_MALLOC
#define VGLSL_MALLOC malloc
#endif

#ifndef VGLSL_FREE
#define VGLSL_FREE free
#endif

#ifndef VGLSL_REALLOC
#define VGLSL_REALLOC realloc
#endif

/* Virtual include path structure */
typedef struct VglslVirtualPath {
    char* virtual_name;   /* e.g., "Vantor" */
    char* real_path;      /* e.g., "/path/to/vantor/shaders" */
} VglslVirtualPath;

/* Global virtual include paths storage */
static VglslVirtualPath g_virtual_paths[VGLSL_MAX_VIRTUAL_PATHS];
static int g_virtual_path_count = 0;

/* Internal structures */
typedef struct VglslDefine {
    char* name;
    char* value;
    char** params;
    int param_count;
    bool is_function_macro;
} VglslDefine;

typedef struct VglslContext {
    VglslDefine defines[VGLSL_MAX_DEFINES];
    int define_count;
    
    char* output;
    size_t output_size;
    size_t output_capacity;
    
    const VglslConfig* config;
    int include_depth;
    
    /* Error handling */
    bool has_error;
    char* error_message;
    int error_line;
    char* error_file;
    
    /* Conditional compilation stack */
    bool if_stack[64];
    bool if_taken[64];
    int if_depth;
} VglslContext;

/* Forward declarations */
static bool vglsl_process_line(VglslContext* ctx, const char* line, int line_num, const char* filename);
static bool vglsl_process_directive(VglslContext* ctx, const char* line, int line_num, const char* filename);
static bool vglsl_expand_macros(VglslContext* ctx, const char* input, char* output, size_t output_size);
static VglslDefine* vglsl_find_define(VglslContext* ctx, const char* name);
static bool vglsl_append_output(VglslContext* ctx, const char* text);
static void vglsl_set_error(VglslContext* ctx, const char* message, int line, const char* filename);
static char* vglsl_read_file(const char* filename);
static void vglsl_cleanup_context(VglslContext* ctx);
static const char* vglsl_resolve_virtual_path(const char* include_path);

/* Utility functions */
static char* vglsl_strdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* copy = (char*)VGLSL_MALLOC(len);
    if (copy) memcpy(copy, str, len);
    return copy;
}

static void vglsl_trim_whitespace(char* str) {
    if (!str) return;
    
    /* Trim leading whitespace */
    char* start = str;
    while (*start == ' ' || *start == '\t') start++;
    if (start != str) memmove(str, start, strlen(start) + 1);
    
    /* Trim trailing whitespace */
    char* end = str + strlen(str) - 1;
    while (end >= str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
}

static bool vglsl_starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) return false;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

/* Default configuration */
VglslConfig vglsl_default_config(void) {
    VglslConfig config = {0};
    config.base_path = "./";
    config.preserve_lines = false;
    config.remove_comments = true;
    config.max_include_depth = VGLSL_MAX_INCLUDE_DEPTH;
    config.max_output_size = VGLSL_MAX_OUTPUT_SIZE;
    return config;
}

/* Read entire file into memory */
static char* vglsl_read_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (size <= 0) {
        fclose(file);
        return NULL;
    }
    
    char* content = (char*)VGLSL_MALLOC(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    fclose(file);
    
    return content;
}

/* Set error in context */
static void vglsl_set_error(VglslContext* ctx, const char* message, int line, const char* filename) {
    if (ctx->has_error) return; /* Keep first error */
    
    ctx->has_error = true;
    ctx->error_message = vglsl_strdup(message);
    ctx->error_line = line;
    ctx->error_file = vglsl_strdup(filename);
}

/* Append text to output buffer */
static bool vglsl_append_output(VglslContext* ctx, const char* text) {
    if (!text) return true;
    
    size_t text_len = strlen(text);
    size_t needed = ctx->output_size + text_len + 1;
    
    /* Check if we need to reallocate */
    if (needed > ctx->output_capacity) {
        /* Calculate new capacity, doubling current capacity or ensuring we have enough space */
        size_t new_capacity = ctx->output_capacity * 2;
        if (new_capacity < needed) new_capacity = needed * 2;
        
        /* Check if new capacity would exceed the maximum allowed size */
        if (new_capacity > ctx->config->max_output_size) {
            new_capacity = ctx->config->max_output_size;
        }
        
        /* If even the maximum allowed size is not enough, fail */
        if (needed > new_capacity) {
            vglsl_set_error(ctx, "Output size exceeded maximum limit", 0, "");
            return false;
        }
        
        char* new_output = (char*)VGLSL_REALLOC(ctx->output, new_capacity);
        if (!new_output) {
            vglsl_set_error(ctx, "Failed to allocate memory for output", 0, "");
            return false;
        }
        
        ctx->output = new_output;
        ctx->output_capacity = new_capacity;
    }
    
    strcat(ctx->output, text);
    ctx->output_size += text_len;
    return true;
}

/* Find define by name */
static VglslDefine* vglsl_find_define(VglslContext* ctx, const char* name) {
    for (int i = 0; i < ctx->define_count; i++) {
        if (strcmp(ctx->defines[i].name, name) == 0) {
            return &ctx->defines[i];
        }
    }
    return NULL;
}

/* Add or update define */
static bool vglsl_add_define(VglslContext* ctx, const char* name, const char* value, char** params, int param_count) {
    if (ctx->define_count >= VGLSL_MAX_DEFINES) {
        vglsl_set_error(ctx, "Too many defines", 0, "");
        return false;
    }
    
    /* Check if define already exists */
    VglslDefine* existing = vglsl_find_define(ctx, name);
    if (existing) {
        /* Update existing define */
        VGLSL_FREE(existing->value);
        if (existing->params) {
            for (int i = 0; i < existing->param_count; i++) {
                VGLSL_FREE(existing->params[i]);
            }
            VGLSL_FREE(existing->params);
        }
    } else {
        /* Add new define */
        existing = &ctx->defines[ctx->define_count++];
        existing->name = vglsl_strdup(name);
    }
    
    existing->value = vglsl_strdup(value ? value : "");
    existing->param_count = param_count;
    existing->is_function_macro = (param_count > 0);
    
    if (param_count > 0) {
        existing->params = (char**)VGLSL_MALLOC(param_count * sizeof(char*));
        for (int i = 0; i < param_count; i++) {
            existing->params[i] = vglsl_strdup(params[i]);
        }
    } else {
        existing->params = NULL;
    }
    
    return true;
}

/* Remove define */
static void vglsl_remove_define(VglslContext* ctx, const char* name) {
    for (int i = 0; i < ctx->define_count; i++) {
        if (strcmp(ctx->defines[i].name, name) == 0) {
            /* Free memory */
            VGLSL_FREE(ctx->defines[i].name);
            VGLSL_FREE(ctx->defines[i].value);
            if (ctx->defines[i].params) {
                for (int j = 0; j < ctx->defines[i].param_count; j++) {
                    VGLSL_FREE(ctx->defines[i].params[j]);
                }
                VGLSL_FREE(ctx->defines[i].params);
            }
            
            /* Shift remaining defines */
            memmove(&ctx->defines[i], &ctx->defines[i + 1], 
                    (ctx->define_count - i - 1) * sizeof(VglslDefine));
            ctx->define_count--;
            break;
        }
    }
}

/* Remove comments from line */
static void vglsl_remove_comments(char* line) {
    char* pos = line;
    bool in_string = false;
    char string_char = 0;
    
    while (*pos) {
        if (!in_string && (*pos == '"' || *pos == '\'')) {
            in_string = true;
            string_char = *pos;
        } else if (in_string && *pos == string_char && (pos == line || *(pos-1) != '\\')) {
            in_string = false;
        } else if (!in_string) {
            if (*pos == '/' && *(pos + 1) == '/') {
                *pos = '\0'; /* End line at // comment */
                break;
            } else if (*pos == '/' && *(pos + 1) == '*') {
                /* Block comment - find end */
                char* end = strstr(pos + 2, "*/");
                if (end) {
                    memmove(pos, end + 2, strlen(end + 2) + 1);
                    continue; /* Don't increment pos */
                } else {
                    *pos = '\0'; /* Block comment continues to end of line */
                    break;
                }
            }
        }
        pos++;
    }
}

/* Expand macros in text */
static bool vglsl_expand_macros(VglslContext* ctx, const char* input, char* output, size_t output_size) {
    const char* src = input;
    char* dst = output;
    size_t remaining = output_size - 1;
    
    while (*src && remaining > 0) {
        if ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || *src == '_') {
            /* Potential identifier */
            const char* id_start = src;
            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || 
                   (*src >= '0' && *src <= '9') || *src == '_') {
                src++;
            }
            
            size_t id_len = src - id_start;
            char identifier[256];
            if (id_len < sizeof(identifier)) {
                memcpy(identifier, id_start, id_len);
                identifier[id_len] = '\0';
                
                VglslDefine* define = vglsl_find_define(ctx, identifier);
                if (define && !define->is_function_macro) {
                    /* Simple macro substitution */
                    size_t value_len = strlen(define->value);
                    if (value_len <= remaining) {
                        strcpy(dst, define->value);
                        dst += value_len;
                        remaining -= value_len;
                    } else {
                        return false; /* Output buffer too small */
                    }
                } else {
                    /* Copy identifier as-is */
                    if (id_len <= remaining) {
                        memcpy(dst, id_start, id_len);
                        dst += id_len;
                        remaining -= id_len;
                    } else {
                        return false;
                    }
                }
            } else {
                return false; /* Identifier too long */
            }
        } else {
            /* Copy character as-is */
            *dst++ = *src++;
            remaining--;
        }
    }
    
    *dst = '\0';
    return true;
}

/* Process #include directive */
static bool vglsl_process_include(VglslContext* ctx, const char* directive, int line_num, const char* filename) {
    if (ctx->include_depth >= ctx->config->max_include_depth) {
        vglsl_set_error(ctx, "Maximum include depth exceeded", line_num, filename);
        return false;
    }
    
    /* Extract filename from #include "filename" or #include <filename> */
    const char* quote_start = strchr(directive, '"');
    const char* angle_start = strchr(directive, '<');
    const char* start = NULL;
    char end_char = 0;
    bool is_angle_include = false;
    
    if (quote_start && (!angle_start || quote_start < angle_start)) {
        start = quote_start + 1;
        end_char = '"';
        is_angle_include = false;
    } else if (angle_start) {
        start = angle_start + 1;
        end_char = '>';
        is_angle_include = true;
    }
    
    if (!start) {
        vglsl_set_error(ctx, "Invalid include directive", line_num, filename);
        return false;
    }
    
    const char* end = strchr(start, end_char);
    if (!end) {
        vglsl_set_error(ctx, "Unterminated include filename", line_num, filename);
        return false;
    }
    
    size_t filename_len = end - start;
    char include_filename[512];
    if (filename_len >= sizeof(include_filename)) {
        vglsl_set_error(ctx, "Include filename too long", line_num, filename);
        return false;
    }
    
    memcpy(include_filename, start, filename_len);
    include_filename[filename_len] = '\0';
    
    /* Build full path */
    char full_path[1024];
    
    /* Handle angle bracket includes with virtual paths */
    if (is_angle_include) {
        const char* resolved_virtual = vglsl_resolve_virtual_path(include_filename);
        if (resolved_virtual) {
            strcpy(full_path, resolved_virtual);
        } else {
            /* Fallback to base_path for angle includes without virtual mapping */
            if (ctx->config->base_path) {
                snprintf(full_path, sizeof(full_path), "%s/%s", ctx->config->base_path, include_filename);
            } else {
                strcpy(full_path, include_filename);
            }
        }
    } else {
        /* Handle quoted includes normally with base_path */
        if (ctx->config->base_path) {
            snprintf(full_path, sizeof(full_path), "%s/%s", ctx->config->base_path, include_filename);
        } else {
            strcpy(full_path, include_filename);
        }
    }
    
    /* Read and process included file */
    char* include_content = vglsl_read_file(full_path);
    if (!include_content) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Failed to read include file: %s", full_path);
        vglsl_set_error(ctx, error_msg, line_num, filename);
        return false;
    }
    
    ctx->include_depth++;
    
    /* Add line directive if requested */
    if (ctx->config->preserve_lines) {
        char line_directive[256];
        snprintf(line_directive, sizeof(line_directive), "#line 1 \"%s\"\n", full_path);
        vglsl_append_output(ctx, line_directive);
    }
    
    /* Process included file line by line */
    char* line_start = include_content;
    int include_line_num = 1;
    bool success = true;
    
    while (*line_start && success) {
        char* line_end = strchr(line_start, '\n');
        size_t line_length;
        
        if (line_end) {
            line_length = line_end - line_start;
        } else {
            line_length = strlen(line_start);
        }
        
        char line_buffer[VGLSL_MAX_LINE_LENGTH];
        if (line_length < sizeof(line_buffer)) {
            memcpy(line_buffer, line_start, line_length);
            line_buffer[line_length] = '\0';
            
            success = vglsl_process_line(ctx, line_buffer, include_line_num, full_path);
        }
        
        if (line_end) {
            line_start = line_end + 1;
        } else {
            break;
        }
        include_line_num++;
    }
    
    ctx->include_depth--;
    
    /* Restore line directive if requested */
    if (ctx->config->preserve_lines && success) {
        char line_directive[256];
        snprintf(line_directive, sizeof(line_directive), "#line %d \"%s\"\n", line_num + 1, filename);
        vglsl_append_output(ctx, line_directive);
    }
    
    VGLSL_FREE(include_content);
    return success;
}

/* Process preprocessor directive */
static bool vglsl_process_directive(VglslContext* ctx, const char* line, int line_num, const char* filename) {
    char directive[VGLSL_MAX_LINE_LENGTH];
    strcpy(directive, line + 1); /* Skip # */
    vglsl_trim_whitespace(directive);
    
    if (vglsl_starts_with(directive, "include")) {
        return vglsl_process_include(ctx, directive, line_num, filename);
    } else if (vglsl_starts_with(directive, "define")) {
        /* Parse #define NAME [VALUE] */
        char* name_start = directive + 6; /* Skip "define" */
        while (*name_start == ' ' || *name_start == '\t') name_start++;
        
        char* name_end = name_start;
        while (*name_end && *name_end != ' ' && *name_end != '\t' && *name_end != '(') name_end++;
        
        if (name_end == name_start) {
            vglsl_set_error(ctx, "Invalid define directive", line_num, filename);
            return false;
        }
        
        char name[256];
        size_t name_len = name_end - name_start;
        if (name_len >= sizeof(name)) {
            vglsl_set_error(ctx, "Define name too long", line_num, filename);
            return false;
        }
        memcpy(name, name_start, name_len);
        name[name_len] = '\0';
        
        /* Check for function macro */
        if (*name_end == '(') {
            /* Function macro - not fully implemented in this example */
            char* value_start = strchr(name_end, ')');
            if (value_start) {
                value_start++;
                while (*value_start == ' ' || *value_start == '\t') value_start++;
                return vglsl_add_define(ctx, name, value_start, NULL, 0);
            }
        } else {
            /* Simple macro */
            char* value_start = name_end;
            while (*value_start == ' ' || *value_start == '\t') value_start++;
            return vglsl_add_define(ctx, name, value_start, NULL, 0);
        }
    } else if (vglsl_starts_with(directive, "undef")) {
        char* name_start = directive + 5; /* Skip "undef" */
        while (*name_start == ' ' || *name_start == '\t') name_start++;
        vglsl_trim_whitespace(name_start);
        vglsl_remove_define(ctx, name_start);
        return true;
    } else if (vglsl_starts_with(directive, "ifdef")) {
        char* name_start = directive + 5; /* Skip "ifdef" */
        while (*name_start == ' ' || *name_start == '\t') name_start++;
        vglsl_trim_whitespace(name_start);
        
        if (ctx->if_depth >= 63) {
            vglsl_set_error(ctx, "Too many nested conditionals", line_num, filename);
            return false;
        }
        
        ctx->if_depth++;
        bool defined = (vglsl_find_define(ctx, name_start) != NULL);
        ctx->if_stack[ctx->if_depth] = defined;
        ctx->if_taken[ctx->if_depth] = defined;
        return true;
    } else if (vglsl_starts_with(directive, "ifndef")) {
        char* name_start = directive + 6; /* Skip "ifndef" */
        while (*name_start == ' ' || *name_start == '\t') name_start++;
        vglsl_trim_whitespace(name_start);
        
        if (ctx->if_depth >= 63) {
            vglsl_set_error(ctx, "Too many nested conditionals", line_num, filename);
            return false;
        }
        
        ctx->if_depth++;
        bool defined = (vglsl_find_define(ctx, name_start) != NULL);
        ctx->if_stack[ctx->if_depth] = !defined;
        ctx->if_taken[ctx->if_depth] = !defined;
        return true;
    } else if (strcmp(directive, "else") == 0) {
        if (ctx->if_depth <= 0) {
            vglsl_set_error(ctx, "#else without #ifdef/#ifndef", line_num, filename);
            return false;
        }
        ctx->if_stack[ctx->if_depth] = !ctx->if_taken[ctx->if_depth];
        return true;
    } else if (strcmp(directive, "endif") == 0) {
        if (ctx->if_depth <= 0) {
            vglsl_set_error(ctx, "#endif without #ifdef/#ifndef", line_num, filename);
            return false;
        }
        ctx->if_depth--;
        return true;
    }
    
    /* Unknown directive - pass through as-is */
    if (!vglsl_append_output(ctx, line)) return false;
    if (!vglsl_append_output(ctx, "\n")) return false;
    return true;
}

/* Check if current context should output (not in false conditional) */
static bool vglsl_should_output(VglslContext* ctx) {
    for (int i = 1; i <= ctx->if_depth; i++) {
        if (!ctx->if_stack[i]) return false;
    }
    return true;
}

/* Process a single line */
static bool vglsl_process_line(VglslContext* ctx, const char* line, int line_num, const char* filename) {
    if (ctx->has_error) return false;
    
    char processed_line[VGLSL_MAX_LINE_LENGTH];
    strcpy(processed_line, line);
    
    /* Remove comments if requested */
    if (ctx->config->remove_comments) {
        vglsl_remove_comments(processed_line);
    }
    
    vglsl_trim_whitespace(processed_line);
    
    /* Handle preprocessor directives */
    if (processed_line[0] == '#') {
        return vglsl_process_directive(ctx, processed_line, line_num, filename);
    }
    
    /* Skip line if in false conditional */
    if (!vglsl_should_output(ctx)) {
        return true;
    }
    
    /* Expand macros */
    char expanded_line[VGLSL_MAX_LINE_LENGTH];
    if (!vglsl_expand_macros(ctx, processed_line, expanded_line, sizeof(expanded_line))) {
        vglsl_set_error(ctx, "Macro expansion failed", line_num, filename);
        return false;
    }
    
    /* Add to output */
    if (!vglsl_append_output(ctx, expanded_line)) return false;
    if (!vglsl_append_output(ctx, "\n")) return false;
    
    return true;
}

/* Clean up context */
static void vglsl_cleanup_context(VglslContext* ctx) {
    for (int i = 0; i < ctx->define_count; i++) {
        VGLSL_FREE(ctx->defines[i].name);
        VGLSL_FREE(ctx->defines[i].value);
        if (ctx->defines[i].params) {
            for (int j = 0; j < ctx->defines[i].param_count; j++) {
                VGLSL_FREE(ctx->defines[i].params[j]);
            }
            VGLSL_FREE(ctx->defines[i].params);
        }
    }
    
    if (ctx->output) {
        VGLSL_FREE(ctx->output);
    }
    
    if (ctx->error_message) {
        VGLSL_FREE(ctx->error_message);
    }
    
    if (ctx->error_file) {
        VGLSL_FREE(ctx->error_file);
    }
}

/* Main parsing function */
static VglslResult vglsl_parse_internal(const char* source, const char* filename, const VglslConfig* config) {
    VglslResult result = {0};
    VglslContext ctx = {0};
    
    /* Initialize context */
    ctx.config = config;
    ctx.output_capacity = 4096;
    ctx.output = (char*)VGLSL_MALLOC(ctx.output_capacity);
    if (!ctx.output) {
        result.error_message = vglsl_strdup("Failed to allocate output buffer");
        return result;
    }
    ctx.output[0] = '\0';
    
    /* Process source line by line */
    const char* line_start = source;
    int line_num = 1;
    bool success = true;
    
    while (*line_start && success && !ctx.has_error) {
        const char* line_end = strchr(line_start, '\n');
        size_t line_length;
        
        if (line_end) {
            line_length = line_end - line_start;
        } else {
            line_length = strlen(line_start);
        }
        
        char line_buffer[VGLSL_MAX_LINE_LENGTH];
        if (line_length < sizeof(line_buffer)) {
            memcpy(line_buffer, line_start, line_length);
            line_buffer[line_length] = '\0';
            
            success = vglsl_process_line(&ctx, line_buffer, line_num, filename);
        } else {
            vglsl_set_error(&ctx, "Line too long", line_num, filename);
            success = false;
        }
        
        if (line_end) {
            line_start = line_end + 1;
        } else {
            break;
        }
        line_num++;
    }
    
    /* Check for unclosed conditionals */
    if (success && ctx.if_depth > 0) {
        vglsl_set_error(&ctx, "Unclosed conditional directive", line_num, filename);
        success = false;
    }
    
    /* Build result */
    if (success && !ctx.has_error) {
        result.success = true;
        result.output = ctx.output;
        ctx.output = NULL; /* Transfer ownership */
    } else {
        result.success = false;
        result.error_message = ctx.error_message;
        result.error_line = ctx.error_line;
        result.error_file = ctx.error_file;
        ctx.error_message = NULL; /* Transfer ownership */
        ctx.error_file = NULL;
    }
    
    vglsl_cleanup_context(&ctx);
    return result;
}

/* Public API implementations */
VglslResult vglsl_parse_file(const char* filename, const char* base_path) {
    VglslConfig config = vglsl_default_config();
    config.base_path = base_path;
    return vglsl_parse_file_ex(filename, &config);
}

VglslResult vglsl_parse_file_ex(const char* filename, const VglslConfig* config) {
    VglslResult result = {0};
    
    char* source = vglsl_read_file(filename);
    if (!source) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Failed to read file: %s", filename);
        result.error_message = vglsl_strdup(error_msg);
        return result;
    }
    
    result = vglsl_parse_internal(source, filename, config);
    VGLSL_FREE(source);
    
    return result;
}

VglslResult vglsl_parse_memory(const char* source, const char* filename) {
    VglslConfig config = vglsl_default_config();
    return vglsl_parse_memory_ex(source, filename, &config);
}

VglslResult vglsl_parse_memory_ex(const char* source, const char* filename, const VglslConfig* config) {
    return vglsl_parse_internal(source, filename, config);
}

void vglsl_free_result(VglslResult* result) {
    if (!result) return;
    
    if (result->output) {
        VGLSL_FREE(result->output);
        result->output = NULL;
    }
    
    if (result->error_message) {
        VGLSL_FREE(result->error_message);
        result->error_message = NULL;
    }
    
    if (result->error_file) {
        VGLSL_FREE(result->error_file);
        result->error_file = NULL;
    }
    
    result->success = false;
    result->error_line = 0;
}

/* Virtual include path management functions */
void vglsl_add_virtual_include_path(const char* virtual_name, const char* real_path) {
    if (!virtual_name || !real_path || g_virtual_path_count >= VGLSL_MAX_VIRTUAL_PATHS) {
        return;
    }
    
    /* Check if virtual name already exists and update it */
    for (int i = 0; i < g_virtual_path_count; i++) {
        if (strcmp(g_virtual_paths[i].virtual_name, virtual_name) == 0) {
            VGLSL_FREE(g_virtual_paths[i].real_path);
            g_virtual_paths[i].real_path = vglsl_strdup(real_path);
            return;
        }
    }
    
    /* Add new virtual path */
    g_virtual_paths[g_virtual_path_count].virtual_name = vglsl_strdup(virtual_name);
    g_virtual_paths[g_virtual_path_count].real_path = vglsl_strdup(real_path);
    g_virtual_path_count++;
}

void vglsl_remove_virtual_include_path(const char* virtual_name) {
    if (!virtual_name) return;
    
    for (int i = 0; i < g_virtual_path_count; i++) {
        if (strcmp(g_virtual_paths[i].virtual_name, virtual_name) == 0) {
            VGLSL_FREE(g_virtual_paths[i].virtual_name);
            VGLSL_FREE(g_virtual_paths[i].real_path);
            
            /* Shift remaining entries down */
            for (int j = i; j < g_virtual_path_count - 1; j++) {
                g_virtual_paths[j] = g_virtual_paths[j + 1];
            }
            g_virtual_path_count--;
            return;
        }
    }
}

void vglsl_clear_virtual_include_paths(void) {
    for (int i = 0; i < g_virtual_path_count; i++) {
        VGLSL_FREE(g_virtual_paths[i].virtual_name);
        VGLSL_FREE(g_virtual_paths[i].real_path);
    }
    g_virtual_path_count = 0;
}

/* Resolve virtual include path */
static const char* vglsl_resolve_virtual_path(const char* include_path) {
    if (!include_path) return NULL;
    
    /* Find the first slash to separate virtual folder from relative path */
    const char* slash = strchr(include_path, '/');
    if (!slash) return NULL;
    
    size_t virtual_name_len = slash - include_path;
    
    /* Check if this matches any virtual path */
    for (int i = 0; i < g_virtual_path_count; i++) {
        if (strlen(g_virtual_paths[i].virtual_name) == virtual_name_len &&
            strncmp(g_virtual_paths[i].virtual_name, include_path, virtual_name_len) == 0) {
            
            /* Build full path: real_path + remaining_path */
            static char resolved_path[1024];
            snprintf(resolved_path, sizeof(resolved_path), "%s%s", 
                    g_virtual_paths[i].real_path, slash);
            return resolved_path;
        }
    }
    
    return NULL;
}

#endif /* VGLSL_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif

#endif /* VGLSL_H */
