#ifndef CCCTX_INTERNAL_H
#define CCCTX_INTERNAL_H

#include "ccctx.h"

#ifdef _WIN32
#include <windows.h>

#define Py2SOBJ "PYTHON27.DLL"
#define Py3SOBJ "PYTHON38.DLL"

#define OSLoadLibrary(name) LoadLibrary(name)
#define OSGetSym(lib, name) GetProcAddress(lib, name)
#define OSUnloadLibrary(lib) FreeLibrary(lib)
#define OSAlloc(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size)
#define OSFree(ptr) HeapFree(GetProcessHeap(), 0, ptr)

#else
#include <dlfcn.h>
typedef void* HMODULE;

#define Py2SOBJ "libpython2.7.so"
#define Py3SOBJ "libpython3.8.so"

#define OSLoadLibrary(name) dlopen(name, RTLD_NOW | RTLD_DEEPBIND | RTLD_LOCAL)
#define OSGetSym(lib, name) dlsym(lib, name)
#define OSUnloadLibrary(lib) dlclose(lib)
#define OSAlloc(size) calloc(1, size)
#define OSFree(ptr) free(ptr)
#endif

#ifndef PY2_FILE_SYSTEM_ENCODING
#define PY2_FILE_SYSTEM_ENCODING "UTF-8"
#endif

typedef struct {
    const char *symname;
    size_t offset;
} symrecord_t;


HMODULE ccctx_load_shared_object(
    const char *path,
    void *ccctx, symrecord_t *symtable, size_t records,
    py_seterr_t errmsg
);



#endif
