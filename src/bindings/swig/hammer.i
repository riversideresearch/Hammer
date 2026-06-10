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

%warnfilter(454) register_helpers;
%inline {
  static PyObject *_helper_Placeholder = NULL, *_helper_ParseError = NULL;

  static void register_helpers(PyObject* parse_error, PyObject *placeholder) {
    _helper_ParseError = parse_error;
    _helper_Placeholder = placeholder;
  }
 }

%pythoncode %{
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

  _hammer._register_helpers(ParseError,
                            Placeholder)
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
    $1 = (uint8_t)PyLong_AsLong($input);
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

%{
#include "allocator.h"
#include "hammer.h"
#ifndef SWIGPERL
// Perl's embed.h conflicts with err.h, which internal.h includes. Ugh.
#include "internal.h"
#endif
#include "glue.h"
%}
%include "allocator.h"

%ignore HTokenData;
%ignore HResultTiming;
%ignore HCaseResult_::timestamp;
%ignore HParsedToken_::token_data;

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
%inline {
  static PyObject* hpt_to_python(const HParsedToken *token) {
    // Caller holds a reference to returned object
    PyObject *ret;
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
      return PyLong_FromLong(token->token_data.sint);
    case TT_UINT:
      return PyLong_FromUnsignedLong(token->token_data.uint);
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
    PyObject *ret = PyObject_CallFunctionObjArgs(callable,
						 hpt_to_python(p->ast),
						 NULL);
    if (ret == NULL) {
      PyErr_Print();
      assert(ret != NULL);
    }
    HParsedToken *tok = h_make(p->arena, h_tt_python, ret);
    return tok;
   }

  static bool call_predicate(struct HParseResult_ *p, void* user_data) {
    PyObject *callable = user_data;
    PyObject *ret = PyObject_CallFunctionObjArgs(callable,
						 hpt_to_python(p->ast),
						 NULL);
    bool rret = false;
    if (ret == NULL) {
      PyErr_Print();
      assert(ret != NULL);
    }
    rret = (bool)PyObject_IsTrue(ret);
    Py_DECREF(ret);
    return rret;
  }

 }

%rename("%s") "";

%extend HParser_ {
    HParseResult* parse(const uint8_t* input, size_t length) {
        return h_parse($self, input, length);
    }
    bool compile(HParserBackend backend) {
        return h_compile($self, backend, NULL) == 0;
    }
    PyObject* __dir__() {
        PyObject* ret = PyList_New(2);
        PyList_SET_ITEM(ret, 0, PyUnicode_FromString("parse"));
        PyList_SET_ITEM(ret, 1, PyUnicode_FromString("compile"));
        return ret;
    }
}

%pythoncode %{
def action(p, act):
    return _h_action(p, act)
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
def repeat_n(p, n): return _h_repeat_n(p, n)
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