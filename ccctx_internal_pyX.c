#include <wchar.h>
#include <stddef.h>
#include <malloc.h>

#include "ccctx_internal_pyX.h"

#define mkstr(x) #x
#define REF(x) { mkstr(x), offsetof(struct ccctx_pyX_t, x) }
#define REFN(n, x) { n, offsetof(struct ccctx_pyX_t, x) }

static symrecord_t symtable_py2[] = {
    REF(Py_FileSystemDefaultEncoding),
    REF(Py_IgnoreEnvironmentFlag),
    REF(Py_NoSiteFlag),
    REF(Py_NoUserSiteDirectory),
    REF(Py_OptimizeFlag),
    REF(Py_DontWriteBytecodeFlag),

    REF(Py_InitializeEx),
    REF(Py_IsInitialized),
    REF(PyGILState_Ensure),
    REF(PyGILState_Release),
    REF(PyEval_GetBuiltins),
    REF(Py_SetProgramName),
    REF(Py_SetPythonHome),
    REF(Py_Finalize),
    REF(PyRun_SimpleString),
    REF(PySys_GetObject),
    REF(PyDict_GetItemString),
    REF(PyDict_SetItemString),
    REF(PyObject_CallFunction),
    REF(PyEval_EvalCode),
    REF(PyDict_New),
    REF(Py_CompileStringFlags),
    REF(PyMarshal_WriteObjectToString),
    REF(PyErr_Print),
    REF(PyErr_Occurred),
    REF(PyErr_Clear),
    REF(PyErr_Fetch),
    REF(PyErr_NormalizeException),
    REF(PyObject_Str),
    REF(Py_DecRef),
    REF(Py_IncRef),
    REF(PyString_AsStringAndSize),
    REF(PyString_AsString),
};

static symrecord_t symtable_py3[] = {
    REF(Py_FileSystemDefaultEncoding),
    REF(Py_IgnoreEnvironmentFlag),
    REF(Py_NoSiteFlag),
    REF(Py_NoUserSiteDirectory),
    REF(Py_OptimizeFlag),
    REF(Py_DontWriteBytecodeFlag),

    REF(Py_InitializeEx),
    REF(Py_IsInitialized),
    REF(PyGILState_Ensure),
    REF(PyGILState_Release),
    REF(PyEval_GetBuiltins),
    REF(Py_SetProgramName),
    REF(Py_SetPythonHome),
    REF(Py_Finalize),
    REF(PyRun_SimpleString),
    REF(PySys_GetObject),
    REF(PyDict_GetItemString),
    REF(PyDict_SetItemString),
    REF(PyObject_CallFunction),
    REF(PyEval_EvalCode),
    REF(PyDict_New),
    REF(Py_CompileStringFlags),
    REF(PyMarshal_WriteObjectToString),
    REF(PyErr_Print),
    REF(PyErr_Occurred),
    REF(PyErr_Clear),
    REF(PyErr_Fetch),
    REF(PyErr_NormalizeException),
    REF(PyObject_Str),
    REF(Py_DecRef),
    REF(Py_IncRef),
    REFN("PyBytes_AsStringAndSize", PyString_AsStringAndSize),
    REFN("PyBytes_AsString", PyString_AsString),
};

static
void* pyX_compile(
    p_ccctx_t ccctx, custom_compiler_t custom_compiler,
    const char *code, const char *filename,
    int optflag, py_make_blob_t py_make_blob, py_seterr_t seterr)
{
    int state;
    int optflag_orig;
    p_ccctx_pyX_t ctx;

    PyXObject *PyX_code = NULL;
    PyXObject *PyX_marshalled_code = NULL;

    void *compiled_code = NULL;

    ctx = (p_ccctx_pyX_t) ccctx;

    state = ctx->PyGILState_Ensure();

    optflag_orig = *ctx->Py_OptimizeFlag;

    *ctx->Py_OptimizeFlag = optflag;

    if (custom_compiler) {
        PyX_marshalled_code = ctx->PyObject_CallFunction(
            custom_compiler, "ss",
            code, filename
        );
    } else {
        PyX_code = ctx->Py_CompileStringFlags(
            code, filename, 257, NULL
        );

        if (PyX_code != NULL) {
            PyX_marshalled_code = ctx->PyMarshal_WriteObjectToString(
                PyX_code, 2);
        }
    }
    *ctx->Py_OptimizeFlag = optflag_orig;

    if (PyX_marshalled_code) {
        char *buffer = NULL;
        PyX_ssize_t length = 0;

        if (ctx->PyString_AsStringAndSize(
                PyX_marshalled_code, &buffer, &length) != -1)
        {
            compiled_code = py_make_blob(buffer, length);
        }
    }

    if (!compiled_code && ctx->PyErr_Occurred()) {
        PyXObject *ptype = NULL;
        PyXObject *pvalue = NULL;
        PyXObject *ptraceback = NULL;

        PyXObject *PyX_err = NULL;

        ctx->PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        ctx->PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

        if (pvalue)
            PyX_err = ctx->PyObject_Str(pvalue);

        if (PyX_err)
            seterr(ctx->PyString_AsString(PyX_err));

        PyX_XDECREF(ctx, PyX_err);
        PyX_XDECREF(ctx, ptype);
        PyX_XDECREF(ctx, pvalue);
        PyX_XDECREF(ctx, ptraceback);

        ctx->PyErr_Clear();
    }

    PyX_XDECREF(ctx, PyX_code);
    PyX_XDECREF(ctx, PyX_marshalled_code);

    ctx->PyGILState_Release(state);

    return compiled_code;
}

static
void pyX_unload(p_ccctx_t ccctx)
{
    p_ccctx_pyX_t ctx = (p_ccctx_pyX_t) ccctx;

    /* printf("pyX_unload(%p) - start\n", ccctx); */

    if (ctx->is_initialized) {
        /* printf("pyX_unload(%p) - deinitialize interpreter\n", ccctx); */
        ctx->Py_Finalize();
    }

    if (ctx->handle)
        OSUnloadLibrary(ctx->handle);

    if (ctx->pyhome)
        OSFree(ctx->pyhome);

    OSFree(ccctx);

    /* printf("pyX_unload(%p) - complete\n", ccctx); */
}

static
custom_compiler_t pyX_make_custom_compiler(
    p_ccctx_t ccctx, const char *compiler_code,
    const char *function, py_seterr_t seterr)
{
    custom_compiler_t result = NULL;
    p_ccctx_pyX_t ctx = (p_ccctx_pyX_t) ccctx;
    PyXObject code_object = ctx->Py_CompileStringFlags(
        compiler_code, "<compiler>", 257, NULL
    );

    if (code_object) {
        PyXObject *py_eval_result = NULL;
        PyXObject *dict = ctx->PyDict_New();
        PyXObject *builtins = ctx->PyEval_GetBuiltins();
        ctx->Py_IncRef(builtins);
        ctx->PyDict_SetItemString(dict, "__builtins__", builtins);

        if (dict) {
            py_eval_result = ctx->PyEval_EvalCode(
                code_object, dict, dict
            );

            if (py_eval_result) {
                PyXObject *py_function = ctx->PyDict_GetItemString(
                    dict, function
                );

                if (!py_function) {
                    seterr(
                        "Failed to find compiler function in evaluated object"
                    );
                }

                ctx->Py_IncRef(py_function);
                result = py_function;
            }
        }

        PyX_XDECREF(ctx, dict);
        PyX_XDECREF(ctx, py_eval_result);
        PyX_XDECREF(ctx, code_object);
    }

    if (ctx->PyErr_Occurred()) {
        PyXObject *ptype = NULL;
        PyXObject *pvalue = NULL;
        PyXObject *ptraceback = NULL;

        PyXObject *PyX_err = NULL;

        ctx->PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        ctx->PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

        if (pvalue)
            PyX_err = ctx->PyObject_Str(pvalue);

        if (PyX_err)
            seterr(ctx->PyString_AsString(PyX_err));

        PyX_XDECREF(ctx, PyX_err);
        PyX_XDECREF(ctx, ptype);
        PyX_XDECREF(ctx, pvalue);
        PyX_XDECREF(ctx, ptraceback);

        ctx->PyErr_Clear();

        PyX_XDECREF(ctx, result);
        return NULL;
    }

    return result;
}

static
void pyX_release_custom_compiler(p_ccctx_t ccctx, custom_compiler_t compiler)
{
    PyX_XDECREF(((p_ccctx_pyX_t) ccctx), compiler);
}

p_ccctx_pyX_t pyX_load(
    int pymaj, const char *sobject, const wchar_t *pyhome, py_seterr_t errfcn)
{
    p_ccctx_pyX_t ctx = NULL;

    ctx = (p_ccctx_pyX_t) OSAlloc(sizeof(struct ccctx_pyX_t));
    if (!ctx) {
        errfcn("Failed to allocate memory");
        return NULL;
    }

    ctx->pyhome = NULL;
    ctx->handle = ccctx_load_shared_object(
        sobject, ctx, pymaj == 2? symtable_py2 : symtable_py3,
        // !! Important - both tables are equal
        sizeof(symtable_py2) / sizeof(symtable_py2[0]),
        errfcn
    );

    if (!ctx->handle) {
        OSFree(ctx);
        return NULL;
    }

    if (ctx->Py_IsInitialized()) {
        /* printf("Already initialized\n"); */

        ctx->is_initialized = 0;
    } else {
        /* printf("Initialization required\n"); */

        *ctx->Py_NoSiteFlag = 1;
        *ctx->Py_IgnoreEnvironmentFlag = 1;
        *ctx->Py_NoUserSiteDirectory = 1;
        *ctx->Py_DontWriteBytecodeFlag = 1;

        if (pymaj == 2) {
            *ctx->Py_FileSystemDefaultEncoding = PYX_FILE_SYSTEM_ENCODING;
            if (pyhome) {
                size_t size = wcstombs(NULL, pyhome, 0);
                ctx->pyhome = OSAlloc(size);
                wcstombs(ctx->pyhome, pyhome, size);
                ctx->Py_SetPythonHome(ctx->pyhome);
            }
        } else if (pyhome) {
            ctx->pyhome = (void *) wcsdup(pyhome);
            ctx->Py_SetPythonHome(ctx->pyhome);
        }

        ctx->Py_SetProgramName("pycc");
        ctx->Py_InitializeEx(0);

        ctx->is_initialized = 1;
    }

    if (!ctx->Py_IsInitialized()) {
        errfcn("Failed to initialize python");
        OSUnloadLibrary(ctx->handle);
        if (ctx->pyhome)
            OSFree(ctx->pyhome);
        OSFree(ctx);
        return NULL;
    }

    ctx->vtable.compile = pyX_compile;
    ctx->vtable.make_custom_compiler = pyX_make_custom_compiler;
    ctx->vtable.release_custom_compiler = pyX_release_custom_compiler;
    ctx->vtable.free = pyX_unload;

    return ctx;
}
