%module hammer
%begin %{
#define SWIG_PYTHON_STRICT_BYTE_CHAR
#include <stdbool.h>
%}

%nodefaultctor;

%include "stdint.i"

#if defined(SWIGPYTHON)
%ignore HCountedArray_;
%apply (char *STRING, size_t LENGTH) {(uint8_t* str, size_t len)}
%apply (uint8_t* str, size_t len) {(const uint8_t* input, size_t length)}
%apply (uint8_t* str, size_t len) {(const uint8_t* str, const size_t len)}
%apply (uint8_t* str, size_t len) {(const uint8_t* charset, size_t length)}


%rename("_%s") "";
// %rename(_h_ch) h_ch;

/* Internal references retained by register_helpers(); they are not part of the
 * language binding API and must not receive generated pointer setters. */
%ignore _helper_Placeholder;
%ignore _helper_ParseError;
%ignore _helper_ParseDiagnostic;
%ignore _helper_ParseFailure;
%ignore _helper_ParseExpectation;
%ignore _helper_SourceLocation;
%inline {
  static PyObject *_helper_Placeholder = NULL, *_helper_ParseError = NULL;
  static PyObject *_helper_ParseDiagnostic = NULL, *_helper_ParseFailure = NULL;
  static PyObject *_helper_ParseExpectation = NULL, *_helper_SourceLocation = NULL;

  static void register_helpers(PyObject* parse_error, PyObject *placeholder,
                               PyObject *parse_diagnostic, PyObject *parse_failure,
                               PyObject *parse_expectation, PyObject *source_location) {
    _helper_ParseError = parse_error;
    _helper_Placeholder = placeholder;
    _helper_ParseDiagnostic = parse_diagnostic;
    _helper_ParseFailure = parse_failure;
    _helper_ParseExpectation = parse_expectation;
    _helper_SourceLocation = source_location;
  }
 }

%pythoncode %{
  from dataclasses import dataclass

  try:
      INTEGER_TYPES = (int, long)
  except NameError:
      INTEGER_TYPES = (int,)

  try:
      TEXT_TYPE = unicode
      def bchr(i):
          return chr(i)
  except NameError:
      TEXT_TYPE = str
      def bchr(i):
          return bytes([i])

  class Placeholder(object):
      """The python equivalent of TT_NONE"""
      def __str__(self):
          return "Placeholder"
      def __repr__(self):
          return "Placeholder"
      def __eq__(self, other):
          return type(self) == type(other)
  class ParseError(Exception):
      """The parse failed; the message may have more information"""
      pass

  @dataclass(frozen=True)
  class SourceLocation:
      file_name: object
      function_name: object
      line: int
      column: int

  @dataclass(frozen=True)
  class ParseExpectation:
      kind: int
      lower: int
      upper: int

  @dataclass(frozen=True)
  class ParseFailure:
      index: int
      end_index: int
      actual: object
      bit_offset: int
      kind: int
      parser: object
      deepest_parsers: tuple
      context: tuple
      message: object
      source: object

  @dataclass(frozen=True)
  class ParseDiagnostic:
      error: ParseFailure
      expected: tuple
      execution_trace: object

  _hammer._register_helpers(ParseError,
                            Placeholder,
                            ParseDiagnostic,
                            ParseFailure,
                            ParseExpectation,
                            SourceLocation)
  %}

%typemap(in) void*[] {
  if (PyList_Check($input)) {
    int size = PyList_Size($input);
    int i = 0;
    int res = 0;
    $1 = (void**)malloc((size+1)*sizeof(HParser*));
    for (i=0; i<size; i++) {
      PyObject *o = PyList_GetItem($input, i);
      res = SWIG_ConvertPtr(o, &($1[i]), SWIGTYPE_p_HParser_, 0 | 0);
      if (!SWIG_IsOK(res)) {
	SWIG_exception_fail(SWIG_ArgError(res), "that wasn't an HParser" );
      }
    }
    $1[size] = NULL;
  } else {
    PyErr_SetString(PyExc_TypeError, "__a functions take lists of parsers as their argument");
    return NULL;
  }
 }
%typemap(freearg) void*[] { free($1); }
%typemap(in) uint8_t {
  if (PyLong_Check($input)) {
    long long value = PyLong_AsLongLong($input);
    if (PyErr_Occurred()) {
      PyErr_Clear();
      PyErr_SetString(PyExc_ValueError, "expected an integer byte in the range 0..255");
      return NULL;
    }
    if (value < 0 || value > 255) {
      PyErr_SetString(PyExc_ValueError, "expected an integer byte in the range 0..255");
      return NULL;
    }
    $1 = (uint8_t)value;
  }
  else if (!PyBytes_Check($input)) {
    PyErr_SetString(PyExc_ValueError, "Expecting an integer or bytes");
    return NULL;
  } else {
    if (PyBytes_Size($input) != 1) {
      PyErr_SetString(PyExc_ValueError, "Expecting a single byte");
      return NULL;
    }
    const char *buf = PyBytes_AsString($input);
    if (buf == NULL) {
      return NULL;
    }
    $1 = (uint8_t)(unsigned char)buf[0];
  }
 }
%typemap(out) HBytes* {
  $result = PyBytes_FromStringAndSize((char*)$1->token, $1->len);
 }
/*
 * A parser factory may return NULL after setting a Python exception. SWIG's
 * default pointer conversion turns that NULL into None, which leaves the
 * exception pending and makes Python raise SystemError instead of the
 * original exception.
 */
%typemap(out) HParser * {
  if ($1 == NULL && PyErr_Occurred())
    return NULL;
  $result = SWIG_NewPointerObj(SWIG_as_voidptr($1), SWIGTYPE_p_HParser_, 0 | 0);
 }
%typemap(out) struct HCountedArray_* {
  size_t i;
  $result = PyList_New($1->used);
  for (i=0; i<$1->used; i++) {
    HParsedToken *t = $1->elements[i];
    PyObject *o = SWIG_NewPointerObj(SWIG_as_voidptr(t), SWIGTYPE_p_HParsedToken_, 0 | 0);
    PyList_SetItem($result, i, o);
  }
 }
%typemap(out) struct HParseResult_* {
  if ($1 == NULL) {
    // Parse failure: return None (the documented Python binding behavior).
    Py_INCREF(Py_None);
    $result = Py_None;
  } else {
    $result = hpt_to_python($1->ast);
  }
 }
%typemap(newfree) struct HParseResult_* {
  h_parse_result_free($input);
 }
%inline %{
  static int h_tt_python;
  %}
%init %{
  h_tt_python = h_allocate_token_type("com.riversideresearch.hammer.python");
  %}




%typemap(in) (HPredicate pred, void* user_data) {
  Py_INCREF($input);
  $2 = $input;
  $1 = call_predicate;
 }

%typemap(in) (const HAction a, void* user_data) {
  Py_INCREF($input);
  $2 = $input;
  $1 = call_action;
 }

%typemap(in) PyObject *entries {
  $1 = $input;
}

%inline %{

  struct HParsedToken_;
  struct HParseResult_;
  static PyObject* hpt_to_python(const struct HParsedToken_ *token);

  static struct HParsedToken_* call_action(const struct HParseResult_ *p, void* user_data);
  static bool call_predicate(struct HParseResult_ *p, void* user_data);
 %}
// #elif !defined(SWIGJAVA)
//   #warning no uint8_t* typemaps defined
#endif

#if defined(SWIGJAVA)

%ignore HCountedArray_;

// Map byte[] ↔ (const uint8_t* input, size_t length) and all equivalent argument pairs.
%typemap(jni)    (const uint8_t* input, size_t length) "jbyteArray"
%typemap(jtype)  (const uint8_t* input, size_t length) "byte[]"
%typemap(jstype) (const uint8_t* input, size_t length) "byte[]"
%typemap(javain) (const uint8_t* input, size_t length) "$javainput"
%typemap(in) (const uint8_t* input, size_t length) {
  $1 = (uint8_t*)JCALL2(GetByteArrayElements, jenv, $input, 0);
  $2 = (size_t)JCALL1(GetArrayLength, jenv, $input);
}
%typemap(argout) (const uint8_t* input, size_t length) {
  JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte*)$1, JNI_ABORT);
}
%typemap(freearg) (const uint8_t* input, size_t length) ""
%apply (const uint8_t* input, size_t length) {
  (uint8_t* str, size_t len),
  (const uint8_t* str, const size_t len),
  (const uint8_t* charset, size_t length)
}

// uint8_t as short - Java's byte is signed; short avoids sign-extension confusion.
%typemap(jni)     uint8_t "jshort"
%typemap(jtype)   uint8_t "short"
%typemap(jstype)  uint8_t "short"
%typemap(javain)  uint8_t "(short)($javainput & 0xff)"
%typemap(in)      uint8_t { $1 = (uint8_t)($input & 0xff); }
%typemap(out)     uint8_t { $result = (jshort)$1; }
%typemap(javaout) uint8_t { return (short)($jnicall & 0xff); }

// void*[] (NULL-terminated parser array) - Java side passes HParser_[], marshalled via long[].
%typemap(jni)    void*[] "jlongArray"
%typemap(jtype)  void*[] "long[]"
%typemap(jstype) void*[] "HParser[]"
%typemap(javain) void*[] "parsersToHandles($javainput)"
%typemap(in) void*[] {
  int _sz = (int)JCALL1(GetArrayLength, jenv, $input);
  jlong *_elems = JCALL2(GetLongArrayElements, jenv, $input, 0);
  $1 = (void**)malloc((size_t)(_sz + 1) * sizeof(void *));
  for (int _i = 0; _i < _sz; _i++) $1[_i] = (void *)(intptr_t)_elems[_i];
  $1[_sz] = NULL;
  JCALL3(ReleaseLongArrayElements, jenv, $input, _elems, JNI_ABORT);
}
%typemap(freearg) void*[] { free($1); }

// Inject a helper into the hammer module class so callers can pass HParser[] where void*[] is
// expected. It lives in hammer.java (same package as HParser), so protected getCPtr is accessible.
%pragma(java) modulecode=%{
  static long[] parsersToHandles(HParser[] parsers) {
    long[] handles = new long[parsers.length];
    for (int i = 0; i < parsers.length; i++)
      handles[i] = HParser.getCPtr(parsers[i]);
    return handles;
  }
%}

// parse() returns a Java-owned HParseResult whose finalizer calls h_parse_result_free.
%newobject HParser_::parse;

#endif

 // All the include paths are relative to the build, i.e., ../../. If you need to build these manually (i.e., not with scons), keep that in mind.
// Suppress GCC attributes that SWIG cannot parse.
#define __attribute__(x)

// Ignore va_list variants - SWIG cannot generate correct wrappers for va_list parameters.
%ignore h_sequence__v;
%ignore h_sequence__mv;
%ignore h_drop_from___v;
%ignore h_drop_from___mv;
%ignore h_choice__v;
%ignore h_choice__mv;
%ignore h_permutation__v;
%ignore h_permutation__mv;

// Ignore varargs variants - Python uses the __a (array) variants instead.
// Without this SWIG generates wrappers that call these sentinel-terminated functions
// without the required NULL terminator, causing -Wmissing-sentinel warnings.
%ignore h_sequence;
%ignore h_sequence__m;
%ignore h_choice;
%ignore h_choice__m;
%ignore h_permutation;
%ignore h_permutation__m;

// Ignore functions declared in hammer.h but not present in the library.
%ignore h_get_backend_with_params_by_name__m;

#if defined(SWIGPYTHON)
// These APIs either require C-owned output pointers or FILE* values. Python
// receives safe value-object wrappers instead of raw aliases.
%ignore HParseDiagnostic;
%ignore HParseError_;
%ignore HParseExpectation_;
%ignore HSourceLocation_;
%ignore h_parse_debug;
%ignore h_parse_debug__m;
%ignore h_parse_diagnostic_error;
%ignore h_parse_diagnostic_expected_count;
%ignore h_parse_diagnostic_expected;
%ignore h_parse_diagnostic_execution_trace;
%ignore h_parse_diagnostic_trace_fprint;
%ignore h_parse_diagnostic_fprint;
%ignore h_parse_diagnostic_fprint_with_input;
%ignore h_parse_diagnostic_free;
%ignore h_parse_error_free;
%ignore h_pprint;
%ignore h_pprintln;
%ignore h_pprint_ast_indexed;

// Parsers borrow their child parsers, so they cannot safely be Python-owned.
%ignore h_parser_free;
%ignore h_parser_free__m;

// Action-collection internals are implementation details. The binding exposes
// only a zero-initialized collection object and the action helpers below.
%ignore HActionEntry_;
%ignore HActionCollection;
%ignore h_action_collection_reset;
%ignore h_action_stash;
%ignore h_action_stash__m;
%ignore h_action_apply;
%ignore h_action_apply__m;
#endif

%{
#include "allocator.h"
#include "hammer.h"
#ifndef SWIGPERL
// Perl's embed.h conflicts with err.h, which internal.h includes. Ugh.
#include "internal.h"
#endif
#include "glue.h"
%}

#ifdef SWIGPYTHON
%{
static PyObject *h_parse_python(const HParser *parser, const uint8_t *input, size_t length);
static PyObject *h_parse_debug_python(const HParser *parser, const uint8_t *input, size_t length,
                                      bool show_diagnostic);
struct HPythonActionCollection_;
static void h_action_collection_free(struct HPythonActionCollection_ *collection);
%}
#endif

%include "allocator.h"

%ignore HTokenData;
%ignore HResultTiming;
%ignore HCaseResult_::timestamp;
%ignore HParsedToken_::token_data;

/* HParseError is borrowed from HParseDiagnostic.  Its string fields are
 * diagnostic output, not caller-owned storage, so exposing setters would both
 * violate the ownership contract and make SWIG allocate strings it cannot
 * safely release. */
%immutable HParseError_::parser;
%immutable HParseError_::message;

/* These are caller-populated C input records whose const char pointers are
 * intentionally borrowed.  Keep their existing binding setters for
 * compatibility; SWIG cannot express that shallow lifetime contract and emits
 * its conservative char-pointer warning. */
%warnfilter(451) HSourceLocation_::file_name;
%warnfilter(451) HSourceLocation_::function_name;
%warnfilter(451) HParserTestcase_::output_unambiguous;
%warnfilter(451) HResultTiming;
%include "hammer.h"

// HArena_ is an opaque type (forward declaration only in allocator.h).
// Provide a body so SWIG can process the %extend below.
struct HArena_ {};

%extend HArena_ {
  ~HArena_() {
    h_delete_arena($self);
  }
 };
%extend HParseResult_ {
  ~HParseResult_() {
    h_parse_result_free($self);
  }
};

%newobject h_parse;
%delobject h_parse_result_free;
%newobject h_new_arena;
%delobject h_delete_arena;

#ifdef SWIGPYTHON
%newobject h_action_collection_new;
%ignore HPythonActionCollection_::collection;
%ignore h_action_collection_free;

%inline {
  static PyObject *h_string_to_python(const char *value) {
    if (value == NULL) {
      Py_INCREF(Py_None);
      return Py_None;
    }
    return PyUnicode_DecodeUTF8(value, (Py_ssize_t)strlen(value), "surrogateescape");
  }

  static PyObject *h_bytes_to_python_text(const char *value, size_t length) {
    if (value == NULL) {
      Py_INCREF(Py_None);
      return Py_None;
    }
    return PyUnicode_DecodeUTF8(value, (Py_ssize_t)length, "surrogateescape");
  }

  static PyObject *h_source_location_to_python(const HSourceLocation *source) {
    PyObject *args;
    PyObject *result;

    if (source == NULL) {
      Py_INCREF(Py_None);
      return Py_None;
    }

    args = PyTuple_New(4);
    if (args == NULL)
      return NULL;
    PyTuple_SET_ITEM(args, 0, h_string_to_python(source->file_name));
    PyTuple_SET_ITEM(args, 1, h_string_to_python(source->function_name));
    PyTuple_SET_ITEM(args, 2, PyLong_FromSize_t(source->line));
    PyTuple_SET_ITEM(args, 3, PyLong_FromSize_t(source->column));
    if (PyErr_Occurred()) {
      Py_DECREF(args);
      return NULL;
    }
    result = PyObject_CallObject(_helper_SourceLocation, args);
    Py_DECREF(args);
    return result;
  }

  static PyObject *h_string_array_to_python(const char *const *values, size_t count) {
    PyObject *result = PyTuple_New((Py_ssize_t)count);
    if (result == NULL)
      return NULL;
    for (size_t i = 0; i < count; i++) {
      PyObject *value = h_string_to_python(values[i]);
      if (value == NULL) {
        Py_DECREF(result);
        return NULL;
      }
      PyTuple_SET_ITEM(result, (Py_ssize_t)i, value);
    }
    return result;
  }

  static PyObject *h_parse_diagnostic_to_python(const HParseDiagnostic *diagnostic) {
    const HParseError *error;
    PyObject *expected;
    PyObject *failure_args;
    PyObject *failure;
    PyObject *diagnostic_args;
    PyObject *result;
    PyObject *trace;
    size_t expected_count;
    size_t trace_length = 0;
    const char *execution_trace;

    if (diagnostic == NULL) {
      Py_INCREF(Py_None);
      return Py_None;
    }

    error = h_parse_diagnostic_error(diagnostic);
    if (error == NULL) {
      PyErr_SetString(PyExc_RuntimeError, "Hammer returned an invalid parse diagnostic");
      return NULL;
    }

    expected_count = h_parse_diagnostic_expected_count(diagnostic);
    expected = PyTuple_New((Py_ssize_t)expected_count);
    if (expected == NULL)
      return NULL;
    for (size_t i = 0; i < expected_count; i++) {
      HParseExpectation expectation;
      PyObject *expectation_args;
      PyObject *expectation_value;

      if (!h_parse_diagnostic_expected(diagnostic, i, &expectation)) {
        Py_DECREF(expected);
        PyErr_SetString(PyExc_RuntimeError, "Hammer returned an invalid parse expectation");
        return NULL;
      }
      expectation_args = PyTuple_New(3);
      if (expectation_args == NULL) {
        Py_DECREF(expected);
        return NULL;
      }
      PyTuple_SET_ITEM(expectation_args, 0, PyLong_FromLong((long)expectation.kind));
      PyTuple_SET_ITEM(expectation_args, 1, PyLong_FromUnsignedLong((unsigned long)expectation.lower));
      PyTuple_SET_ITEM(expectation_args, 2, PyLong_FromUnsignedLong((unsigned long)expectation.upper));
      if (PyErr_Occurred()) {
        Py_DECREF(expectation_args);
        Py_DECREF(expected);
        return NULL;
      }
      expectation_value = PyObject_CallObject(_helper_ParseExpectation, expectation_args);
      Py_DECREF(expectation_args);
      if (expectation_value == NULL) {
        Py_DECREF(expected);
        return NULL;
      }
      PyTuple_SET_ITEM(expected, (Py_ssize_t)i, expectation_value);
    }

    failure_args = PyTuple_New(10);
    if (failure_args == NULL) {
      Py_DECREF(expected);
      return NULL;
    }
    PyTuple_SET_ITEM(failure_args, 0, PyLong_FromSize_t(error->index));
    PyTuple_SET_ITEM(failure_args, 1, PyLong_FromSize_t(error->end_index));
    if (error->has_actual)
      PyTuple_SET_ITEM(failure_args, 2, PyLong_FromUnsignedLong((unsigned long)error->actual));
    else {
      Py_INCREF(Py_None);
      PyTuple_SET_ITEM(failure_args, 2, Py_None);
    }
    PyTuple_SET_ITEM(failure_args, 3, PyLong_FromUnsignedLong((unsigned long)error->bit_offset));
    PyTuple_SET_ITEM(failure_args, 4, PyLong_FromLong((long)error->kind));
    PyTuple_SET_ITEM(failure_args, 5, h_string_to_python(error->parser));
    PyTuple_SET_ITEM(failure_args, 6,
                     h_string_array_to_python(error->deepest_parsers, error->n_deepest));
    PyTuple_SET_ITEM(failure_args, 7, h_string_array_to_python(error->context, error->n_context));
    PyTuple_SET_ITEM(failure_args, 8, h_string_to_python(error->message));
    PyTuple_SET_ITEM(failure_args, 9, h_source_location_to_python(error->source));
    if (PyErr_Occurred()) {
      Py_DECREF(failure_args);
      Py_DECREF(expected);
      return NULL;
    }
    failure = PyObject_CallObject(_helper_ParseFailure, failure_args);
    Py_DECREF(failure_args);
    if (failure == NULL) {
      Py_DECREF(expected);
      return NULL;
    }

    execution_trace = h_parse_diagnostic_execution_trace(diagnostic, &trace_length);
    trace = h_bytes_to_python_text(execution_trace, trace_length);
    if (trace == NULL) {
      Py_DECREF(failure);
      Py_DECREF(expected);
      return NULL;
    }

    diagnostic_args = PyTuple_New(3);
    if (diagnostic_args == NULL) {
      Py_DECREF(trace);
      Py_DECREF(failure);
      Py_DECREF(expected);
      return NULL;
    }
    PyTuple_SET_ITEM(diagnostic_args, 0, failure);
    PyTuple_SET_ITEM(diagnostic_args, 1, expected);
    PyTuple_SET_ITEM(diagnostic_args, 2, trace);
    result = PyObject_CallObject(_helper_ParseDiagnostic, diagnostic_args);
    Py_DECREF(diagnostic_args);
    return result;
  }

  static PyObject* hpt_to_python(const HParsedToken *token) {
    // Caller holds a reference to returned object
    PyObject *ret;
    if (PyErr_Occurred())
      return NULL;
    if (token == NULL) {
      Py_RETURN_NONE;
    }
    switch (token->token_type) {
    case TT_NONE:
      return PyObject_CallFunctionObjArgs(_helper_Placeholder, NULL);
      break;
    case TT_BYTES:
      return PyBytes_FromStringAndSize((char*)token->token_data.bytes.token, token->token_data.bytes.len);
    case TT_SINT:
      return PyLong_FromLongLong((long long)token->token_data.sint);
    case TT_UINT:
      return PyLong_FromUnsignedLongLong((unsigned long long)token->token_data.uint);
    case TT_DOUBLE:
      return PyFloat_FromDouble(token->token_data.dbl);
    case TT_FLOAT:
      return PyFloat_FromDouble((double)token->token_data.flt);
    case TT_SEQUENCE:
      ret = PyTuple_New(token->token_data.seq->used);
      for (size_t i = 0; i < token->token_data.seq->used; i++) {
	PyTuple_SET_ITEM(ret, i, hpt_to_python(token->token_data.seq->elements[i]));
      }
      return ret;
    default:
      if (token->token_type == (HTokenType)h_tt_python) {
	ret = (PyObject*)token->token_data.user;
	Py_INCREF(ret);
	return ret;
      } else {
	return SWIG_NewPointerObj((void*)token, SWIGTYPE_p_HParsedToken_, 0 | 0);
      }

    }
  }
  static struct HParsedToken_* call_action(const struct HParseResult_ *p, void* user_data) {
    PyObject *callable = user_data;
    PyObject *arg = hpt_to_python(p->ast);
    PyObject *ret;
    if (arg == NULL)
      return NULL;
    ret = PyObject_CallFunctionObjArgs(callable, arg, NULL);
    Py_DECREF(arg);
    if (ret == NULL)
      return NULL;
    HParsedToken *tok = h_make(p->arena, h_tt_python, ret);
    return tok;
   }

  static bool call_predicate(struct HParseResult_ *p, void* user_data) {
    PyObject *callable = user_data;
    PyObject *arg = hpt_to_python(p->ast);
    PyObject *ret;
    int rret;
    if (arg == NULL)
      return false;
    ret = PyObject_CallFunctionObjArgs(callable, arg, NULL);
    Py_DECREF(arg);
    if (ret == NULL)
      return false;
    rret = PyObject_IsTrue(ret);
    Py_DECREF(ret);
    if (rret < 0)
      return false;
    return rret != 0;
  }

  static PyObject *h_parse_python(const HParser *parser, const uint8_t *input, size_t length) {
    HParseResult *parse_result = h_parse(parser, input, length);
    PyObject *result;

    if (parse_result == NULL) {
      if (PyErr_Occurred())
        return NULL;
      Py_RETURN_NONE;
    }
    result = hpt_to_python(parse_result->ast);
    h_parse_result_free(parse_result);
    if (result == NULL)
      return NULL;
    if (PyErr_Occurred()) {
      Py_DECREF(result);
      return NULL;
    }
    return result;
  }

  static PyObject *h_parse_debug_python(const HParser *parser, const uint8_t *input, size_t length,
                                        bool show_diagnostic) {
    HParseDiagnostic *diagnostic = NULL;
    HParseResult *parse_result;
    PyObject *result;
    PyObject *python_diagnostic;
    PyObject *pair;

    parse_result = h_parse_debug(parser, input, length, &diagnostic, show_diagnostic);
    if (parse_result == NULL) {
      if (PyErr_Occurred()) {
        if (diagnostic != NULL)
          h_parse_diagnostic_free(diagnostic);
        return NULL;
      }
      Py_INCREF(Py_None);
      result = Py_None;
    } else {
      result = hpt_to_python(parse_result->ast);
      h_parse_result_free(parse_result);
      if (result == NULL || PyErr_Occurred()) {
        if (result != NULL)
          Py_DECREF(result);
        if (diagnostic != NULL)
          h_parse_diagnostic_free(diagnostic);
        return NULL;
      }
    }

    python_diagnostic = h_parse_diagnostic_to_python(diagnostic);
    if (diagnostic != NULL)
      h_parse_diagnostic_free(diagnostic);
    if (python_diagnostic == NULL) {
      Py_DECREF(result);
      return NULL;
    }

    pair = PyTuple_New(2);
    if (pair == NULL) {
      Py_DECREF(result);
      Py_DECREF(python_diagnostic);
      return NULL;
    }
    PyTuple_SET_ITEM(pair, 0, result);
    PyTuple_SET_ITEM(pair, 1, python_diagnostic);
    return pair;
  }

  typedef struct HPythonActionCollection_ {
    HActionCollection collection;
  } HPythonActionCollection;

  static HPythonActionCollection *h_action_collection_new(void) {
    return calloc(1, sizeof(HPythonActionCollection));
  }

  static void h_action_collection_free(HPythonActionCollection *collection) {
    free(collection);
  }

  static HParser *h_action_stash_python(const HParser *parser, const HAction a,
                                        void *user_data, HPythonActionCollection *collection) {
    return h_action_stash(parser, a, user_data,
                          collection == NULL ? NULL : &collection->collection);
  }

  static HParser *h_action_apply_python(HParser *parser, HPythonActionCollection *collection) {
    return h_action_apply(parser, collection == NULL ? NULL : &collection->collection);
  }

  static HParser *h_bit0_python(void) {
    return H_BIT0();
  }

  static HParser *h_bit1_python(void) {
    return H_BIT1();
  }

  static HParser *h_dispatch_from_python(HParser *discriminator, PyObject *entries,
                                         HParser *default_parser) {
    PyObject *sequence = NULL;
    OpcodeMap *map = NULL;
    Py_ssize_t count;
    HParser *result = NULL;

    sequence = PySequence_Fast(entries, "dispatch entries must be an iterable of (opcode, parser) pairs");
    if (sequence == NULL)
      goto done;
    count = PySequence_Fast_GET_SIZE(sequence);
    if (count <= 0) {
      PyErr_SetString(PyExc_ValueError, "dispatch requires at least one opcode/parser entry");
      goto done;
    }
    map = calloc((size_t)count, sizeof(*map));
    if (map == NULL) {
      PyErr_NoMemory();
      goto done;
    }
    for (Py_ssize_t i = 0; i < count; i++) {
      PyObject *entry = PySequence_Fast_GET_ITEM(sequence, i);
      PyObject *pair = PySequence_Fast(entry, "each dispatch entry must be an (opcode, parser) pair");
      unsigned long opcode;
      void *parser_pointer = NULL;
      int conversion;

      if (pair == NULL)
        goto done;
      if (PySequence_Fast_GET_SIZE(pair) != 2) {
        Py_DECREF(pair);
        PyErr_SetString(PyExc_ValueError, "each dispatch entry must contain exactly two values");
        goto done;
      }
      opcode = PyLong_AsUnsignedLong(PySequence_Fast_GET_ITEM(pair, 0));
      if (PyErr_Occurred() || opcode > UINT32_MAX) {
        Py_DECREF(pair);
        if (!PyErr_Occurred())
          PyErr_SetString(PyExc_ValueError, "dispatch opcodes must fit in an unsigned 32-bit integer");
        goto done;
      }
      conversion = SWIG_ConvertPtr(PySequence_Fast_GET_ITEM(pair, 1), &parser_pointer,
                                   SWIGTYPE_p_HParser_, 0);
      Py_DECREF(pair);
      if (!SWIG_IsOK(conversion)) {
        PyErr_SetString(PyExc_TypeError, "each dispatch parser must be an HParser");
        goto done;
      }
      map[i].opcode = (uint32_t)opcode;
      map[i].parser = (HParser *)parser_pointer;
    }
    result = h_dispatch__s(discriminator, map, (size_t)count, default_parser);

  done:
    free(map);
    Py_XDECREF(sequence);
    return result;
  }

  static HParser *h_context_python(HParser *parser, const char *label, const char *file_name,
                                   const char *function_name, size_t line, size_t column) {
    return h_with_context_at(parser, label, file_name, function_name, line, column);
  }
 }

%rename("%s") "";

%extend HPythonActionCollection_ {
  ~HPythonActionCollection_() {
    h_action_collection_free($self);
  }
};

%extend HParser_ {
    PyObject* parse(const uint8_t* input, size_t length) {
        return h_parse_python($self, input, length);
    }
    PyObject* parse_debug(const uint8_t* input, size_t length, bool show=false) {
        return h_parse_debug_python($self, input, length, show);
    }
    bool compile(HParserBackend backend) {
        return h_compile($self, backend, NULL) == 0;
    }
    PyObject* __dir__() {
        PyObject* ret = PyList_New(3);
        PyList_SET_ITEM(ret, 0, PyUnicode_FromString("parse"));
        PyList_SET_ITEM(ret, 1, PyUnicode_FromString("parse_debug"));
        PyList_SET_ITEM(ret, 2, PyUnicode_FromString("compile"));
        return ret;
    }
}

%pythoncode %{
def action(p, act):
    return _h_action(p, act)

def _retain(parser, *dependencies):
    parser._hammer_dependencies = dependencies
    return parser

def action_collection():
    return _h_action_collection_new()

def action_stash(p, act, c):
    return _retain(_h_action_stash_python(p, act, c), p, c)

def action_apply(p, c):
    return _retain(_h_action_apply_python(p, c), p, c)

def attr_bool(p, pred):
    return _h_attr_bool(p, pred)

def ch(ch):
    if isinstance(ch, (bytes, TEXT_TYPE)):
        return token(ch)
    else:
        return  _h_ch(ch)

def ch_range(c1, c2):
    dostr = isinstance(c1, bytes)
    dostr2 = isinstance(c2, bytes)
    if isinstance(c1, TEXT_TYPE) or isinstance(c2, TEXT_TYPE):
        raise TypeError("ch_range only works on bytes")
    if dostr != dostr2:
        raise TypeError("Both arguments to ch_range must be the same type")
    if dostr:
        return action(_h_ch_range(c1, c2), bchr)
    else:
        return _h_ch_range(c1, c2)
def epsilon_p(): return _h_epsilon_p()
def end_p():
    return _h_end_p()
def in_(charset):
    return action(_h_in(charset), bchr)
def not_in(charset):
    return action(_h_not_in(charset), bchr)
def not_(p): return _h_not(p)
def int_range(p, i1, i2):
    return _h_int_range(p, i1, i2)
def float_range(p, f1, f2):
    return _h_float_range(p, f1, f2)
def token(string):
    return _h_token(string)
def whitespace(p):
    return _h_whitespace(p)
def xor(p1, p2):
    return _h_xor(p1, p2)
def butnot(p1, p2):
    return _h_butnot(p1, p2)
def and_(p1):
    return _h_and(p1)
def difference(p1, p2):
    return _h_difference(p1, p2)

def sepBy(p, sep): return _h_sepBy(p, sep)
def sepBy1(p, sep): return _h_sepBy1(p, sep)
def many(p): return _h_many(p)
def many1(p): return _h_many1(p)
def many_cap(p, n): return _h_many_cap(p, n)
def many1_cap(p, n): return _h_many1_cap(p, n)
def repeat_n(p, n): return _h_repeat_n(p, n)
def bit0(): return _h_bit0_python()
def bit1(): return _h_bit1_python()

def dispatch(discriminator, entries, default=None):
    if hasattr(entries, "items"):
        entries = entries.items()
    entries = list(entries)
    parser = _h_dispatch_from_python(discriminator, entries, default)
    return _retain(parser, discriminator, entries, default)

def choice(*args): return _h_choice__a(list(args))
def sequence(*args): return _h_sequence__a(list(args))

def optional(p): return _h_optional(p)
def nothing_p(): return _h_nothing_p()
def ignore(p): return _h_ignore(p)

def left(p1, p2): return _h_left(p1, p2)
def middle(p1, p2, p3): return _h_middle(p1, p2, p3)
def right(p1, p2): return _h_right(p1, p2)


class HIndirectParser(_HParser_):
    def __init__(self):
        # Shoves the guts of an _HParser_ into a HIndirectParser.
        tret = _h_indirect()
        self.__dict__.clear()
        self.__dict__.update(tret.__dict__)

    def __dir__(self):
        return super(HIndirectParser, self).__dir__() + ['bind']
    def bind(self, parser):
        _h_bind_indirect(self, parser)

def indirect():
    return HIndirectParser()

def bind_indirect(indirect, new_parser):
    indirect.bind(new_parser)

def uint8():  return _h_uint8()
def uint16(): return _h_uint16()
def uint32(): return _h_uint32()
def uint64(): return _h_uint64()
def int8():  return _h_int8()
def int16(): return _h_int16()
def int32(): return _h_int32()
def int64(): return _h_int64()
def float16(): return _h_float16()
def float32(): return _h_float32()
def float64(): return _h_float64()

def _as_c_string(value):
    if value is None or isinstance(value, bytes):
        return value
    if isinstance(value, TEXT_TYPE):
        return value.encode("utf-8")
    raise TypeError("expected str, bytes, or None")

def set_label(parser, label):
    if not _h_parser_set_label(parser, _as_c_string(label)):
        raise MemoryError("could not set parser label")
    return parser

def set_error_message(parser, message):
    if not _h_parser_set_error_message(parser, _as_c_string(message)):
        raise MemoryError("could not set parser error message")
    return parser

def context(parser, label=None, file_name=None, function_name=None, line=None, column=0):
    if file_name is None or function_name is None or line is None:
        import inspect
        frame = inspect.currentframe().f_back
        try:
            if file_name is None:
                file_name = frame.f_code.co_filename
            if function_name is None:
                function_name = frame.f_code.co_name
            if line is None:
                line = frame.f_lineno
        finally:
            del frame
    return _retain(
        _h_context_python(
            parser,
            _as_c_string(label),
            _as_c_string(file_name),
            _as_c_string(function_name),
            line,
            column,
        ),
        parser,
    )

def _parser_set_label(self, label):
    return set_label(self, label)

def _parser_set_error_message(self, message):
    return set_error_message(self, message)

_HParser_.set_label = _parser_set_label
_HParser_.set_error_message = _parser_set_error_message

def put_value(p, name): return _h_put_value(p, name)
def get_value(name): return _h_get_value(name)
def free_value(name): return _h_free_value(name)

%}

#endif

#ifdef SWIGJAVA

%extend HParser_ {
    struct HParseResult_* parse(const uint8_t* input, size_t length) {
        return h_parse($self, input, length);
    }
    struct HParseResult_* parse_debug(const uint8_t* input, size_t length,
                                      HParseDiagnostic** diagnostic, bool show) {
        return h_parse_debug($self, input, length, diagnostic, show);
    }
    bool compile(HParserBackend backend) {
        return h_compile($self, backend, NULL) == 0;
    }
}

%extend HParsedToken_ {
    /* Token type as int - compare against TT_NONE, TT_BYTES, TT_SINT, TT_UINT, TT_SEQUENCE. */
    int tokenType() {
        return (int)$self->token_type;
    }
    long long sintValue() {
        return (long long)$self->token_data.sint;
    }
    long long uintValue() {
        return (long long)(unsigned long long)$self->token_data.uint;
    }
    size_t seqLength() {
        return ($self->token_type == TT_SEQUENCE) ? $self->token_data.seq->used : 0;
    }
    /* Returns the i-th element of a TT_SEQUENCE token, or NULL if out of range. */
    struct HParsedToken_* seqElement(size_t i) {
        if ($self->token_type == TT_SEQUENCE && i < $self->token_data.seq->used)
            return $self->token_data.seq->elements[i];
        return NULL;
    }
    size_t bytesLength() {
        return ($self->token_type == TT_BYTES) ? $self->token_data.bytes.len : 0;
    }
    /* Returns byte value at index i as a short (0-255), or -1 if out of range. */
    short byteAt(size_t i) {
        if ($self->token_type == TT_BYTES && i < $self->token_data.bytes.len)
            return (short)(unsigned short)$self->token_data.bytes.token[i];
        return -1;
    }
}

#endif


#ifdef SWIGGO

%extend HParser_ {
    struct HParseResult_* parse(const uint8_t* input, size_t length) {
        return h_parse($self, input, length);
    }
    bool compile(HParserBackend backend) {
        return h_compile($self, backend, NULL) == 0;
    }
}

%extend HParsedToken_ {
  
    /* Token type as int - compare against TT_NONE, TT_BYTES, TT_SINT, TT_UINT, TT_SEQUENCE. */
    int tokenType() {
        return (int)$self->token_type;
    }
    long long sintValue() {
        return (long long)$self->token_data.sint;
    }
    unsigned long long uintValue() {
        return (unsigned long long)$self->token_data.uint;
    }
    size_t seqLength() {
        return ($self->token_type == TT_SEQUENCE) ? $self->token_data.seq->used : 0;
    }
    /* Returns the i-th element of a TT_SEQUENCE token, or NULL if out of range. */
    struct HParsedToken_* seqElement(size_t i) {
        if ($self->token_type == TT_SEQUENCE && i < $self->token_data.seq->used)
            return $self->token_data.seq->elements[i];
        return NULL;
    }
    const uint8_t *bytesData() {
    return ($self->token_type == TT_BYTES)
        ? $self->token_data.bytes.token
        : NULL;
    }
    size_t bytesLength() {
        return ($self->token_type == TT_BYTES) ? $self->token_data.bytes.len : 0;
    }
    /* Returns byte value at index i as a short (0-255), or -1 if out of range. */
    short byteAt(size_t i) {
        if ($self->token_type == TT_BYTES && i < $self->token_data.bytes.len)
            return (short)(unsigned short)$self->token_data.bytes.token[i];
        return -1;
    }
}

%ignore HCountedArray_;

/*
 * Base typemap
 */

%typemap(gotype) (const uint8_t* input, size_t length) "[]byte"
%typemap(imtype) (const uint8_t* input, size_t length) "[]byte"
%typemap(goin) (const uint8_t* input, size_t length) "$1"

%typemap(in) (const uint8_t* input, size_t length) {
    $1 = (const uint8_t*)$input;
    $2 = (size_t)len($input);
}

/*
 * Reuse for equivalent signaturesGetToken_data().GetSint
 */

%apply (const uint8_t* input, size_t length) {
    (uint8_t* str, size_t len),
    (const uint8_t* str, const size_t len),
    (const uint8_t* charset, size_t length)
};

#endif
