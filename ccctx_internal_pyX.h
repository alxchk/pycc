#ifndef CCCTX_INTERNAL_PYX
#define CCCTX_INTERNAL_PYX

#include "ccctx_internal.h"

typedef void* PyXObject;
typedef size_t PyX_ssize_t;

typedef void (*pyX_PyErr_Print)(void);

typedef void (*pyX_Py_InitializeEx)(int);
typedef int (*pyX_Py_IsInitialized)(void);
typedef int (*pyX_PyGILState_Ensure)(void);
typedef void (*pyX_PyGILState_Release)(int);
typedef void (*pyX_Py_SetProgramName)(const char *);
typedef void (*pyX_Py_SetPythonHome)(const void *);
typedef void (*pyX_Py_Finalize)(void);
typedef int (*pyX_PyRun_SimpleString)(const char *);
typedef PyXObject* (*pyX_PySys_GetObject)(const char *);
typedef PyXObject* (*pyX_PyDict_GetItemString)(PyXObject *, const char *);
typedef int (*pyX_PyDict_SetItemString)(PyXObject *, const char *, PyXObject* );

typedef PyXObject* (*pyX_Py_CompileStringFlags)(
    const char *, const char *, int, void *);
typedef PyXObject* (*pyX_PyEval_EvalCode)(
    PyXObject *, PyXObject *, PyXObject *);

typedef PyXObject* (*pyX_PyEval_GetBuiltins)(void);
typedef PyXObject* (*pyX_PyDict_New)(void);

typedef PyXObject* (*pyX_PyObject_CallFunction)(PyXObject *, char *, ...);
typedef PyXObject* (*pyX_PyMarshal_WriteObjectToString)(PyXObject *, int);
typedef PyXObject* (*pyX_PyErr_Occurred)(void);
typedef void (*pyX_PyErr_Clear)(void);
typedef void (*pyX_PyErr_Fetch)(PyXObject **, PyXObject **, PyXObject **);
typedef void (*pyX_PyErr_NormalizeException)(PyXObject**, PyXObject**, PyXObject**);

typedef PyXObject* (*pyX_PyObject_Str)(PyXObject *);

typedef int (*pyX_PyString_AsStringAndSize)(PyXObject *, char **, PyX_ssize_t*);
typedef const char* (*pyX_PyString_AsString)(PyXObject *);

typedef void (*pyX_Py_DecRef)(PyXObject *);
typedef void (*pyX_Py_IncRef)(PyXObject *);

#define PyX_XDECREF(ctx, x) do {             \
    if (x != NULL) {                         \
        ctx->Py_DecRef(x);  \
        x = NULL;           \
    }                       \
} while(0)

typedef struct ccctx_pyX_t {
    ccctx_vtable_t vtable;
    void *pyhome;

    HMODULE handle;

    int is_initialized;

    char **Py_FileSystemDefaultEncoding;
    int *Py_IgnoreEnvironmentFlag;
    int *Py_NoSiteFlag;
    int *Py_NoUserSiteDirectory;
    int *Py_OptimizeFlag;
    int *Py_DontWriteBytecodeFlag;

    pyX_Py_InitializeEx Py_InitializeEx;
    pyX_Py_IsInitialized Py_IsInitialized;
    pyX_PyGILState_Ensure PyGILState_Ensure;
    pyX_PyGILState_Release PyGILState_Release;
    pyX_Py_SetProgramName Py_SetProgramName;
    pyX_Py_SetPythonHome Py_SetPythonHome;
    pyX_Py_Finalize Py_Finalize;
    pyX_PyEval_GetBuiltins PyEval_GetBuiltins;
    pyX_PyRun_SimpleString PyRun_SimpleString;
    pyX_PySys_GetObject PySys_GetObject;
    pyX_PyDict_GetItemString PyDict_GetItemString;
    pyX_PyDict_SetItemString PyDict_SetItemString;
    pyX_PyObject_CallFunction PyObject_CallFunction;
    pyX_PyEval_EvalCode PyEval_EvalCode;
    pyX_PyDict_New PyDict_New;
    pyX_Py_CompileStringFlags Py_CompileStringFlags;
    pyX_PyMarshal_WriteObjectToString PyMarshal_WriteObjectToString;
    pyX_PyErr_Print PyErr_Print;
    pyX_PyErr_Occurred PyErr_Occurred;
    pyX_PyErr_Clear PyErr_Clear;
    pyX_PyErr_Fetch PyErr_Fetch;
    pyX_PyErr_NormalizeException PyErr_NormalizeException;
    pyX_PyObject_Str PyObject_Str;
    pyX_Py_IncRef Py_IncRef;
    pyX_Py_DecRef Py_DecRef;
    pyX_PyString_AsString PyString_AsString;
    pyX_PyString_AsStringAndSize PyString_AsStringAndSize;
} *p_ccctx_pyX_t;

p_ccctx_pyX_t pyX_load(
    int pymaj, const char *sobject, const wchar_t *pyhome,
    py_seterr_t errfcn
);

#ifndef PYX_FILE_SYSTEM_ENCODING
#define PYX_FILE_SYSTEM_ENCODING "UTF-8"
#endif

#endif
