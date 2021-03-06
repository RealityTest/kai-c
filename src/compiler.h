#pragma once

// Requires package.h queue.h

// string.h
typedef struct InternedString InternedString;

// checker.h
typedef struct Sym Sym;

// package.h
typedef struct PackageMapEntry PackageMapEntry;

typedef enum Os {
    Os_Unknown,
    Os_Linux,
    Os_Darwin,
    Os_Windows,
    NUM_OSES,
} Os;

typedef enum Arch {
    Arch_Unknown,
    Arch_x86_64,
    Arch_x86,
    Arch_arm,
    Arch_arm64,
    NUM_ARCHES,
} Arch;

typedef struct TargetMetrics TargetMetrics;
struct TargetMetrics {
    u32 width;
    u32 align;
};

//typedef enum Output Output;
typedef enum Output {
    OutputType_Exec = 0,
    OutputType_Static,
    OutputType_Dynamic
} Output;

typedef enum CompilationStage {
    STAGE_NONE,
    STAGE_PARSE,
    STAGE_TYPECHECK,
    STAGE_BUILD,
    STAGE_EMIT_OBJECTS,
    STAGE_LINK_OBJECTS,
    STAGE_LINK_DEBUG_INFO,
} CompilationStage;

typedef struct CompilerFlags CompilerFlags;
struct CompilerFlags {
    b32 parse_comments;
    b32 error_codes;
    b32 error_colors;
    b32 error_source;
    b32 builtins;
    b32 verbose;
    b32 version;
    b32 help;
    b32 emit_ir;
    b32 emit_header;
    b32 dump_ir;
    b32 disable_all_passes;
    b32 assertions;
    b32 developer;
    b32 small;
    b32 debug;
    b32 link;
};

#define MAX_SEARCH_PATHS 16

typedef struct Compiler Compiler;
struct Compiler {
    int arg_count;
    const char **args;

    CompilationStage failure_stage;

    CompilerFlags flags;
    char input_name[MAX_PATH];
    char output_name[MAX_PATH];
    Os target_os;
    Arch target_arch;
    Output target_output;
    TargetMetrics target_metrics;

    const char *import_search_paths[MAX_SEARCH_PATHS];
    int num_import_search_paths;

    const char *library_search_paths[MAX_SEARCH_PATHS];
    int num_library_search_paths;

    const char *framework_search_paths[MAX_SEARCH_PATHS];
    int num_framework_search_paths;

    const char **libraries;
    const char **frameworks;

    InternedString *interns;
    Arena strings;
    Arena arena;

    Scope *global_scope;
    Package builtin_package;
    PackageMapEntry *packages;

    Sym **ordered_symbols;

    Queue parsing_queue;
    Queue checking_queue;
};

typedef struct CheckerWork CheckerWork;
struct CheckerWork {
    Package *package;
    Stmt *stmt;
};

extern Compiler compiler;

const char *compiler_stage_name(CompilationStage stage);
void compiler_init(Compiler *compiler, int argc, const char **argv);
bool compile(Compiler *compiler);
