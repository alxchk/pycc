#ifndef CCCTX_H
#define CCCTX_H

#include <stdlib.h>

typedef struct ccctx *p_ccctx_t;
typedef void *custom_compiler_t;

typedef void* (*py_make_blob_t)(const char *data, size_t size);
typedef void (*py_seterr_t)(const char *errmsg);

typedef void (*ccctx_free_t)(p_ccctx_t ccctx);
typedef custom_compiler_t (*ccctx_make_compiler_t)(
    p_ccctx_t ccctx, const char *compiler_code, const char *function,
    py_seterr_t errmsg
);
typedef void (*ccctx_release_compiler_t)(
    p_ccctx_t ccctx, custom_compiler_t custom_compiler
);
typedef void* (*ccctx_compile_t)(
    p_ccctx_t ccctx, custom_compiler_t custom_compiler,
    const char *code, const char *filename,
    int optflag, py_make_blob_t py_make_blob, py_seterr_t errmsg
);

typedef struct _ccctx_vtable {
    ccctx_compile_t compile;

    ccctx_make_compiler_t make_custom_compiler;
    ccctx_release_compiler_t release_custom_compiler;

    ccctx_free_t free;
} ccctx_vtable_t;

struct ccctx {
    ccctx_vtable_t vtable;
    // Private
};

typedef struct ccctx_ref_t {
    int pymaj;
    int pymin;
    int refcnt;

    p_ccctx_t ctx;

    struct ccctx_ref_t *next;
} *p_ccctx_ref_t;

p_ccctx_t _ccctx_load(
    int pymaj, int pymin, const char *from,
    py_seterr_t seterr
);

p_ccctx_ref_t ccctx_load(
    int pymaj, int pymin, const char *from,
    py_seterr_t seterr
);

void* ccctx_compile(
    p_ccctx_ref_t ccctx_ref,
    const char *code, const char *filename,
    int optflag, py_make_blob_t py_make_blob, py_seterr_t errmsg
);

void ccctx_unload(p_ccctx_ref_t ref);

#endif
