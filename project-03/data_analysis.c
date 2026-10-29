#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>

// Match the baseline's integer/float rules, including rejection of booleans.
int read_number(PyObject *value, double *number, const char *message)
{
    if (PyBool_Check(value) || (!PyLong_Check(value) && !PyFloat_Check(value)))
    {
        PyErr_SetString(PyExc_TypeError, message);
        return 1;
    }
    *number = PyFloat_AsDouble(value);
    return PyErr_Occurred() ? 1 : 0;
}

static PyObject *analyze(PyObject *self, PyObject *args, PyObject *keywords)
{
    (void)self;
    PyObject *values;
    PyObject *threshold_object = NULL;
    static char *names[] = {"values", "threshold", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywords, "O|O", names, &values, &threshold_object))
        return NULL;

    PyObject *sequence = PySequence_Fast(values, "Dataset must be iterable");
    if (sequence == NULL)
        return NULL;

    Py_ssize_t size = PySequence_Fast_GET_SIZE(sequence);
    double *numbers = NULL;
    PyObject *normalized = NULL;
    PyObject *result = NULL;

    if (size == 0)
    {
        PyErr_SetString(PyExc_ValueError, "Dataset must not be empty");
        goto cleanup;
    }
    if ((size_t)size > ((size_t)-1) / sizeof(double))
    {
        PyErr_NoMemory();
        goto cleanup;
    }
    numbers = PyMem_Malloc((size_t)size * sizeof(double));
    if (numbers == NULL)
    {
        PyErr_NoMemory();
        goto cleanup;
    }

    for (Py_ssize_t i = 0; i < size; i++)
    {
        PyObject *item = PySequence_Fast_GET_ITEM(sequence, i);
        if (read_number(item, &numbers[i], "Dataset values must be integers or floats") != 0)
            goto cleanup;
        if (!isfinite(numbers[i]))
        {
            PyErr_SetString(PyExc_ValueError, "Dataset values must be finite");
            goto cleanup;
        }
    }

    double threshold = 2.0;
    if (threshold_object != NULL &&
        read_number(threshold_object, &threshold, "Threshold must be an integer or float") != 0)
        goto cleanup;
    if (!isfinite(threshold) || threshold < 0.0)
    {
        PyErr_SetString(PyExc_ValueError, "Threshold must be finite and nonnegative");
        goto cleanup;
    }

    // Use the same operation order and population variance as the baseline.
    double mean = 0.0;
    double squared_deviations = 0.0;
    double minimum = numbers[0];
    double maximum = numbers[0];
    for (Py_ssize_t i = 0; i < size; i++)
    {
        double delta = numbers[i] - mean;
        mean += delta / (double)(i + 1);
        squared_deviations += delta * (numbers[i] - mean);
        if (numbers[i] < minimum)
            minimum = numbers[i];
        if (numbers[i] > maximum)
            maximum = numbers[i];
    }

    double variance = squared_deviations / (double)size;
    if (variance < 0.0)
        variance = 0.0;
    double value_range = maximum - minimum;
    if (!isfinite(mean) || !isfinite(variance) || !isfinite(value_range))
    {
        PyErr_SetString(PyExc_OverflowError, "Dataset exceeds the supported floating-point range");
        goto cleanup;
    }

    double cutoff = threshold * sqrt(variance);
    Py_ssize_t outlier_count = 0;
    normalized = PyList_New(size);
    if (normalized == NULL)
        goto cleanup;

    for (Py_ssize_t i = 0; i < size; i++)
    {
        if (fabs(numbers[i] - mean) > cutoff)
            outlier_count++;
        double scaled = value_range == 0.0 ? 0.0 : (numbers[i] - minimum) / value_range;
        PyObject *number = PyFloat_FromDouble(scaled);
        if (number == NULL)
            goto cleanup;
        // PyList_SET_ITEM transfers ownership of this new float to the list.
        PyList_SET_ITEM(normalized, i, number);
    }

    result = Py_BuildValue("{s:d,s:d,s:d,s:d,s:n,s:O}", "mean", mean, "variance", variance, "minimum", minimum, "maximum", maximum, "outlier_count", outlier_count, "normalized", normalized);

cleanup:
    Py_XDECREF(normalized);
    PyMem_Free(numbers);
    Py_DECREF(sequence);
    return result;
}

static PyMethodDef methods[] = {
    {"analyze", (PyCFunction)(void (*)(void))analyze,
     METH_VARARGS | METH_KEYWORDS,
     "Calculate statistics, outliers, and min-max normalized values."},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "data_analysis",
    "Data analysis using the CPython C API.",
    -1,
    methods,
    NULL, NULL, NULL, NULL};

PyMODINIT_FUNC PyInit_data_analysis(void)
{
    return PyModule_Create(&module);
}
