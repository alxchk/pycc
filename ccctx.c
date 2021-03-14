#include <stdio.h>

#include "ccctx.h"
#include "ccctx_internal_pyX.h"

p_ccctx_t _ccctx_load(
    int pymaj, int pymin, const char *from,
    py_seterr_t seterr)
{
    char buf[512];

    if (!from) {
#ifdef _WIN32
        snprintf(buf, sizeof(buf)-1, "PYTHON%d%d.DLL", pymaj, pymin);
#else
        if (pymaj == 3 && pymin < 8)
            snprintf(buf, sizeof(buf)-1, "libpython%d.%dm.so.1.0", pymaj, pymin);
        else
            snprintf(buf, sizeof(buf)-1, "libpython%d.%d.so.1.0", pymaj, pymin);
#endif
        from = buf;
    }

    switch (pymaj) {
        case 2:
        case 3:
            return (p_ccctx_t)pyX_load(pymaj, from, seterr);
        default:
            seterr("Required python major version is not supported");
            return NULL;
    }
}

static p_ccctx_ref_t ccctx_refs = NULL;

p_ccctx_ref_t ccctx_load(
    int pymaj, int pymin, const char *from,
    py_seterr_t seterr)
{
    p_ccctx_ref_t ref;
    p_ccctx_t ctx;

    for (ref=ccctx_refs; ref != NULL; ref = ref->next) {
        if (ref->pymaj == pymaj && ref->pymin == pymin) {
            ++ ref->refcnt;
            /* printf("Using cached ref: %p (%d)\n", ref, ref->refcnt); */
            return ref;
        }
        /*  else { */
        /*     printf( */
        /*         "Incompatible ref %p (%d != %d or %d != %d)\n", */
        /*         ref, */
        /*         ref->pymaj, pymaj, ref->pymin, pymin */
        /*     ); */
        /* } */
    }

    ctx = _ccctx_load(pymaj, pymin, from, seterr);
    if (!ctx)
        return NULL;

    ref = OSAlloc(sizeof(struct ccctx_ref_t));
    if (!ref) {
        seterr("Failed to alloc context reference");
        ctx->vtable.free(ctx);
        return NULL;
    }

    ref->ctx = ctx;
    ref->refcnt = 1;
    ref->next = ccctx_refs;
    ref->pymaj = pymaj;
    ref->pymin = pymin;

    ccctx_refs = ref;

    /* printf( */
    /*     "Create new ref: %p (%d) head=%p next=%p\n", */
    /*     ref, ref->refcnt, ccctx_refs, ccctx_refs->next */
    /* ); */

    return ref;
}


void ccctx_unload(p_ccctx_ref_t ref) {
    p_ccctx_ref_t ref2;

    -- ref->refcnt;

    if (ref->refcnt) {
        /* printf("Ref still used: %p (%d)\n", ref, ref->refcnt); */
        return;
    }

    /* printf("Release ref: %p (%d)\n", ref, ref->refcnt); */

    if (ccctx_refs == ref) {
        ccctx_refs = ccctx_refs->next;
        ref->ctx->vtable.free(ref->ctx);
        OSFree(ref);
        return;
    }

    for (ref2=ccctx_refs; ref2 != NULL && ref2->next != ref; ref2 = ref2->next);

    if (ref2)
        ref2->next = ref2->next->next;

    ref->ctx->vtable.free(ref->ctx);
    OSFree(ref);
}
