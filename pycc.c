#include <Python.h>

#include "ccctx.h"

static PyObject *ccctx_ErrorObject = NULL;

typedef struct {
    PyObject_HEAD

    p_ccctx_ref_t ctxref;
    custom_compiler_t custom_compiler;
} py_ccctx_t;


static PyObject *
py_ccctx_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    py_ccctx_t *self = NULL;

    self = (py_ccctx_t *) type->tp_alloc(type, 0);
    self->ctxref = NULL;
    self->custom_compiler = NULL;

    return (PyObject *) self;
}


static void
py_ccctx_dealloc(PyObject* object) {
    py_ccctx_t *self = (py_ccctx_t *) object;
    if (self->custom_compiler)
        self->ctxref->ctx->vtable.release_custom_compiler(
            self->ctxref->ctx, self->custom_compiler
        );


    if (self->ctxref) {
        ccctx_unload(self->ctxref);
        self->ctxref = NULL;
    }

    Py_TYPE(self)->tp_free(object);
}

static
void _seterr(const char *errmsg) {
    PyErr_SetString(ccctx_ErrorObject, errmsg);
}

static int
py_ccctx_init(PyObject *object, PyObject *args, PyObject *kwds)
{
    py_ccctx_t *self = (py_ccctx_t *) object;

    int pymaj;
    int pymin;
    const char *load_from = NULL;

    char *kwds_names[] = {
        "", "", "load_from", NULL
    };

    if (! PyArg_ParseTupleAndKeywords(
            args, kwds, "II|s", kwds_names,
            &pymaj, &pymin, &load_from)) {

        PyErr_SetString(ccctx_ErrorObject, "Invalid arguments");
        return -1;
    }

    p_ccctx_ref_t ref = ccctx_load(pymaj, pymin, load_from, _seterr);
    if (! ref)
        return -1;

    self->ctxref = ref;
    return 0;
}

static PyObject*
py_ccctx_compile(PyObject *object, PyObject *args, PyObject *kwds)
{
    py_ccctx_t *self = (py_ccctx_t *) object;

    PyObject *py_code_src = NULL;
    PyObject *py_code_decoded = NULL;
    const char *filename = "";
    int optflag = 2;
    PyObject *py_code = NULL;

    char *kwds_names[] = {
        "", "filename", "optflag", NULL
    };

    if (! PyArg_ParseTupleAndKeywords(
            args, kwds, "O|sI", kwds_names,
            &py_code_src, &filename, &optflag)) {

        PyErr_SetString(ccctx_ErrorObject, "Invalid arguments");
        return NULL;
    }

    if (PyUnicode_Check(py_code_src)) {
        py_code_decoded = py_code_src = PyUnicode_AsUTF8String(py_code_src);
    }

    if (!py_code_src)
        return NULL;

    if (!PyBytes_Check(py_code_src)) {
        PyErr_SetString(
            ccctx_ErrorObject, "Code source must be either bytes or unicode object"
        );
        return NULL;
    }

    py_code = self->ctxref->ctx->vtable.compile(
        self->ctxref->ctx,
        self->custom_compiler,
        PyBytes_AsString(py_code_src),
        filename, optflag,
        (py_make_blob_t) PyBytes_FromStringAndSize,
        _seterr
    );

    Py_XDECREF(py_code_decoded);

    return py_code;
}

static PyObject*
py_ccctx_set_compiler(PyObject *object, PyObject *args)
{
    py_ccctx_t *self = (py_ccctx_t *) object;

    PyObject *py_code_src = NULL;
    PyObject *py_code_decoded = NULL;

    custom_compiler_t compiler = NULL;

    const char *function = "";

    if (! PyArg_ParseTuple(
            args, "Os", &py_code_src, &function)) {

        PyErr_SetString(ccctx_ErrorObject, "Invalid arguments");
        return NULL;
    }

    if (PyUnicode_Check(py_code_src)) {
        py_code_decoded = py_code_src = PyUnicode_AsUTF8String(py_code_src);
    }

    if (!py_code_src)
        return NULL;

    if (!PyBytes_Check(py_code_src)) {
        PyErr_SetString(
            ccctx_ErrorObject, "Code source must be either bytes or unicode object"
        );
        return NULL;
    }

    compiler = self->ctxref->ctx->vtable.make_custom_compiler(
        self->ctxref->ctx, PyBytes_AsString(py_code_src),
        function,
        _seterr
    );

    Py_XDECREF(py_code_decoded);

    if (! compiler) {
        return NULL;
    }

    if (self->custom_compiler)
        self->ctxref->ctx->vtable.release_custom_compiler(
            self->ctxref->ctx, self->custom_compiler);

    self->custom_compiler = compiler;

    return PyBool_FromLong(1);
}

static PyMethodDef
py_ccctx_methods[] = {
    {
        "compile", (PyCFunction)py_ccctx_compile, METH_VARARGS | METH_KEYWORDS,
        "compile(code, filename="", optflag=2) - Compile & marshal text to blob",
    },
    {
        "set_compiler", (PyCFunction)py_ccctx_set_compiler, METH_VARARGS,
        "compile(compiler_code, compiler_function) - "
        "Load compiler_code and set compiler_function as compiler",
    },
    {NULL}
};


static PyTypeObject PyCcctxObject_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pycc.Ctx",                          /* tp_name */
    sizeof(py_ccctx_t),                  /* tp_basicsize */
    0,                                   /* tp_itemsize */
    py_ccctx_dealloc,                    /* tp_dealloc */
    0,                                   /* tp_print */
    0,                                   /* tp_getattr */
    0,                                   /* tp_setattr */
    0,                                   /* tp_compare */
    0,                                   /* tp_repr */
    0,                                   /* tp_as_number */
    0,                                   /* tp_as_sequence */
    0,                                   /* tp_as_mapping */
    0,                                   /* tp_hash */
    0,                                   /* tp_call */
    0,                                   /* tp_str */
    0,                                   /* tp_getattro */
    0,                                   /* tp_setattro */
    0,                                   /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                  /* tp_flags */

    "Ctx(pymaj, pymin, load_from=None)", /* tp_doc */

    0,                                    /* tp_traverse */
    0,                                    /* tp_clear */
    0,                                    /* tp_richcompare */
    0,                                    /* tp_weaklistoffset */
    0,                                    /* tp_iter */
    0,                                    /* tp_iternext */
    py_ccctx_methods,                     /* tp_methods */
    0,                                    /* tp_members */
    0,                                    /* tp_getset */
    0,                                    /* tp_base */
    0,                                    /* tp_dict */
    0,                                    /* tp_descr_get */
    0,                                    /* tp_descr_set */
    0,                                    /* tp_dictoffset */
    (initproc)py_ccctx_init,              /* tp_init */
    0,                                    /* tp_alloc */
    py_ccctx_new,                         /* tp_new */
};

static struct PyModuleDef pycc_module = {
    PyModuleDef_HEAD_INIT,
    "pycc",
    "Python bytecode cross-compiler",
};

PyMODINIT_FUNC
PyInit_pycc(void)
{
    PyObject *m;

    if (PyType_Ready(&PyCcctxObject_Type) < 0)
        return NULL;

    m = PyModule_Create(&pycc_module);
    if (m == NULL)
        return NULL;

    ccctx_ErrorObject = PyErr_NewException("pycc.error", NULL, NULL);
    Py_XINCREF(ccctx_ErrorObject);
    if (PyModule_AddObject(m, "error", ccctx_ErrorObject) < 0) {
        Py_CLEAR(ccctx_ErrorObject);
        Py_DECREF(m);
        return NULL;
    }

    if (PyModule_AddObject(m, "Ctx", (PyObject *)&PyCcctxObject_Type) < 0) {
        Py_CLEAR(ccctx_ErrorObject);
        Py_DECREF(m);
    }

    return m;
}
