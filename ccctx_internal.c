#include <stdio.h>
#include "ccctx_internal.h"

HMODULE ccctx_load_shared_object(const char *path, void *ccctx, symrecord_t *symtable,
                                 size_t records, py_seterr_t errmsg)
{
    HMODULE hModule = NULL;
    size_t symidx;

    hModule = OSLoadLibrary(path);
    if (!hModule) {
        errmsg("Failed to load shared object");
        return NULL;
    }

    for (symidx = 0; symidx < records; symidx ++)
    {
        void *symptr = (void *) OSGetSym(hModule, symtable[symidx].symname);
        if (! symptr) {
            char buf[1024];
            snprintf(
                buf, sizeof(buf)-1,
                "Failed to resolve symbol: %s", symtable[symidx].symname
            );

            errmsg(buf);
            OSUnloadLibrary(hModule);
            return NULL;
        }

        *((void **) ((char *) ccctx + symtable[symidx].offset)) = symptr;
    }

    return hModule;
}
