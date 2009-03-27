
/*
 * Python bindings for libfiu
 * Alberto Bertogli (albertito@blitiri.com.ar)
 *
 * This is the low-level module, used by the python one to construct
 * friendlier objects.
 */

#include <Python.h>

/* Unconditionally enable fiu, otherwise we get fake headers */
#define FIU_ENABLE 1

#include <fiu.h>
#include <fiu-control.h>


static PyObject *fail(PyObject *self, PyObject *args)
{
	char *name;
	PyObject *rv, *err;

	if (!PyArg_ParseTuple(args, "s:fail", &name))
		return NULL;

	rv = PyLong_FromLong(fiu_fail(name));
	err = PyErr_Occurred();

	if (rv == NULL || err != NULL) {
		Py_XDECREF(rv);
		return NULL;
	}

	return rv;
}

static PyObject *failinfo(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":failinfo"))
		return NULL;

	/* We assume failinfo is a python object; but the caller must be
	 * careful because if it's not, it can get into trouble.
	 * Note that we DO NOT TOUCH THE RC OF THE OBJECT. It's entirely up to
	 * the caller to make sure it's still alive. */
	return (PyObject *) fiu_failinfo();
}

static PyObject *enable(PyObject *self, PyObject *args)
{
	char *name;
	int failnum;
	PyObject *failinfo;
	unsigned int flags;

	if (!PyArg_ParseTuple(args, "siOI:enable", &name, &failnum, &failinfo,
				&flags))
		return NULL;

	/* See failinfo()'s comment regarding failinfo's RC */
	return PyLong_FromLong(fiu_enable(name, failnum, failinfo, flags));
}

static PyObject *enable_random(PyObject *self, PyObject *args)
{
	char *name;
	int failnum;
	PyObject *failinfo;
	unsigned int flags;
	double probability;

	if (!PyArg_ParseTuple(args, "siOId:enable_random", &name, &failnum,
				&failinfo, &flags, &probability))
		return NULL;

	/* See failinfo()'s comment regarding failinfo's RC */
	return PyLong_FromLong(fiu_enable_random(name, failnum, failinfo,
				flags, probability));
}


static int external_callback(const char *name, int *failnum, void **failinfo,
		unsigned int *flags)
{
	int rv;
	PyObject *cbrv;
	PyObject *args;
	PyGILState_STATE gil_state;

	/* We need to protect ourselves from the following case:
	 *  - fiu.enable_callback('x', cb)  (where cb is obviously a Python
	 *    function)
	 *  - Later on, call a function p1() inside a python C module, that
	 *    runs a C function c1() inside
	 *    Py_BEGIN_ALLOW_THREADS/Py_END_ALLOW_THREADS
	 *  - c1() calls fiu_fail("x")
	 *  - fiu_fail("x") calls external_callback(), and it should run cb()
	 *  - BUT! It can't run cb(), because it's inside
	 *    Py_BEGIN_ALLOW_THREADS/Py_END_ALLOW_THREADS so it's not safe to
	 *    execute any Python code!
	 *
	 * The solution is to ensure we're safe to run Python code using
	 * PyGILState_Ensure()/PyGILState_Release().
	 */

	gil_state = PyGILState_Ensure();
	args = Py_BuildValue("(siI)", name, *failnum, *flags);
	if (args == NULL) {
		PyGILState_Release(gil_state);
		return 0;
	}

	cbrv = PyEval_CallObject(*failinfo, args);
	Py_DECREF(args);

	if (cbrv == NULL) {
		PyGILState_Release(gil_state);
		return 0;
	}

	/* If PyLong_AsLong() causes an error, it will be handled by the
	 * PyErr_Occurred() check in fail(), so we don't need to worry about
	 * it now. */
	rv = PyLong_AsLong(cbrv);
	Py_DECREF(cbrv);

	PyGILState_Release(gil_state);

	return rv;
}

static PyObject *enable_external(PyObject *self, PyObject *args)
{
	char *name;
	int failnum;
	unsigned int flags;
	PyObject *py_external_cb;

	if (!PyArg_ParseTuple(args, "siIO:enable_random", &name, &failnum,
				&flags, &py_external_cb))
		return NULL;

	if (!PyCallable_Check(py_external_cb)) {
		PyErr_SetString(PyExc_TypeError, "parameter must be callable");
		return NULL;
	}

	/* We use failinfo to store Python's callback function. It'd be nice
	 * if we could keep both, but it's not easy without keeping state
	 * inside the C module.
	 *
	 * Similar to the way failinfo is handled, we DO NOT TOUCH THE RC OF
	 * THE EXTERNAL CALLBACK, assuming the caller will take care of making
	 * sure it doesn't dissapear from beneath us. */
	return PyLong_FromLong(fiu_enable_external(name, failnum,
				py_external_cb, flags, external_callback));
}

static PyObject *disable(PyObject *self, PyObject *args)
{
	char *name;

	if (!PyArg_ParseTuple(args, "s:fail", &name))
		return NULL;

	return PyLong_FromLong(fiu_disable(name));
}


static PyMethodDef fiu_methods[] = {
	{ "fail", (PyCFunction) fail, METH_VARARGS, NULL },
	{ "failinfo", (PyCFunction) failinfo, METH_VARARGS, NULL },
	{ "enable", (PyCFunction) enable, METH_VARARGS, NULL },
	{ "enable_random", (PyCFunction) enable_random, METH_VARARGS, NULL },
	{ "enable_external", (PyCFunction) enable_external, METH_VARARGS, NULL },
	{ "disable", (PyCFunction) disable, METH_VARARGS, NULL },
	{ NULL }
};

static PyModuleDef fiu_module = {
	PyModuleDef_HEAD_INIT,
	.m_name = "libfiu",
	.m_size = -1,
	.m_methods = fiu_methods,
};


PyMODINIT_FUNC PyInit_fiu_ll(void)
{
	PyObject *m;

	m = PyModule_Create(&fiu_module);

	PyModule_AddIntConstant(m, "FIU_ONETIME", FIU_ONETIME);

	fiu_init(0);

	return m;
}

