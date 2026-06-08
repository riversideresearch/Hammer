#ifndef HAMMER_HAMMER__HPP
#define HAMMER_HAMMER__HPP

#include "hammer.h"
#include <string>
#include <stdint.h>
#include <cstdarg>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused"

// system_allocator is declared in internal.h but that header is not
// C++-compatible. Forward-declare only what this header needs.
extern "C" {
    extern HAllocator system_allocator;
}

namespace hammer {
  class ParseResult;
  class Parser {
  public:
    const HParser *parser;

    Parser(const HParser* inner) : parser(inner) {}

    //static Parser nil = Parser(NULL);
    ParseResult parse(const std::string &string);
  };

  class ParsedToken {
    // This object can suddenly become invalid if the underlying parse
    // tree is destroyed.

    // This object should serve as a very thin wrapper around an HParsedToken*.
    // In particular sizeof(ParsedToken) should== sizeof(HParsedToken*)
    // This means that we only get one member variable and no virtual functions.
  protected:
    const HParsedToken *token;

  public:

    ParsedToken(const HParsedToken *inner) : token(inner) {}
    ParsedToken(const ParsedToken &other) : token(other.token) {}

    inline HTokenType getType() {
      return token->token_type;
    }

    void* getUser() const {return token->token_data.user;}
    uint64_t getUint() const {return token->token_data.uint;}
    int64_t getSint() const {return token->token_data.sint;}
    // getSeq() is not provided; access token_data.seq directly via the HParsedToken* if needed.
    std::string getBytes() const {return std::string((char*)token->token_data.bytes.token, token->token_data.bytes.len); }


    std::string asUnambiguous() {
      char* buf = h_write_result_unamb(token);
      std::string s = std::string(buf);
      (&system_allocator)->free(&system_allocator, buf);
      return s;
    }
  };

  class ParseResult {
  protected:
    HParseResult *_result;
  public:

    ParseResult(HParseResult *result) : _result(result) {}

    ParseResult(const ParseResult&) = delete;
    ParseResult& operator=(const ParseResult&) = delete;
    ParseResult(ParseResult&& other) noexcept : _result(other._result) { other._result = nullptr; }
    ParseResult& operator=(ParseResult&& other) noexcept {
      if (this != &other) {
        h_parse_result_free(_result);
        _result = other._result;
        other._result = nullptr;
      }
      return *this;
    }

    ParsedToken getAST() {
      return ParsedToken(_result->ast);
    }
    inline std::string asUnambiguous() {
      return getAST().asUnambiguous();
    }

    operator bool() {
      return _result != NULL;
    }
    bool operator !() {
      return _result == NULL;
    }

    ~ParseResult() {
      h_parse_result_free(_result);
      _result = NULL;
    }
  };

  inline ParseResult Parser::parse(const std::string &string) {
    return ParseResult(h_parse(parser, (uint8_t*)string.data(), string.length()));
  }


  static inline Parser Token(const std::string &str) {
    return Parser(h_token((const uint8_t*)str.data(), str.length()));
  }
  static inline Parser Token(const uint8_t *buf, size_t len) {
    return Parser(h_token(buf, len));
  }
  static inline Parser Ch(char ch) {
    return Parser(h_ch(ch));
  }
  static inline Parser ChRange(uint8_t lower, uint8_t upper) {
    return Parser(h_ch_range(lower,upper));
  }

  static inline Parser Int64() { return Parser(h_int64()); }
  static inline Parser Int32() { return Parser(h_int32()); }
  static inline Parser Int16() { return Parser(h_int16()); }
  static inline Parser Int8 () { return Parser(h_int8 ()); }

  static inline Parser Uint64() { return Parser(h_uint64()); }
  static inline Parser Uint32() { return Parser(h_uint32()); }
  static inline Parser Uint16() { return Parser(h_uint16()); }
  static inline Parser Uint8 () { return Parser(h_uint8 ()); }

  static inline Parser IntRange(Parser p, int64_t lower, int64_t upper) {
    return Parser(h_int_range(p.parser, lower, upper));
  }

  static inline Parser Bits(size_t len, bool sign) { return Parser(h_bits(len, sign)); }
  static inline Parser Whitespace(Parser p) { return Parser(h_whitespace(p.parser)); }
  static inline Parser Left(Parser p, Parser q) { return Parser(h_left(p.parser, q.parser)); }
  static inline Parser Right(Parser p, Parser q) { return Parser(h_right(p.parser, q.parser)); }
  static inline Parser Middle(Parser p, Parser q, Parser r) {
    return Parser(h_middle(p.parser, q.parser, r.parser));
  }

  // User is responsible for ensuring that function remains allocated.
  Parser Action(Parser p, HAction action, void* user_data) {
    return Parser(h_action(p.parser, action, user_data));
  }

  Parser Action(Parser p, HAction action) {
    return Parser(h_action(p.parser, action, NULL));
  }

  Parser AttrBool(Parser p, HPredicate pred, void* user_data) {
    return Parser(h_attr_bool(p.parser, pred, user_data));
  }

  Parser AttrBool(Parser p, HPredicate pred) {
    return Parser(h_attr_bool(p.parser, pred, NULL));
  }

  static inline Parser In(const std::string &charset) {
    return Parser(h_in((const uint8_t*)charset.data(), charset.length()));
  }
  static inline Parser In(const uint8_t *charset, size_t length) {
    return Parser(h_in(charset, length));
  }

  static inline Parser NotIn(const std::string &charset) {
    return Parser(h_not_in((const uint8_t*)charset.data(), charset.length()));
  }
  static inline Parser NotIn(const uint8_t *charset, size_t length) {
    return Parser(h_not_in(charset, length));
  }

  static inline Parser End() { return Parser(h_end_p()); }
  static inline Parser Nothing() { return Parser(h_nothing_p()); }

  static inline Parser Sequence(Parser p, ...) {
      va_list ap;
      va_start(ap, p);
      // Reinterpret Parser (which starts with a single HParser* member) as HParser*.
      HParser* ret = h_sequence__v(*(HParser**)(void*)&p, ap);
      va_end(ap);
      return Parser(ret);
  }

  static inline Parser Choice(Parser p, ...) {
      va_list ap;
      va_start(ap, p);
      HParser* ret = h_choice__v(*(HParser**)(void*)&p, ap);
      va_end(ap);
      return Parser(ret);
  }

  static inline Parser ButNot(Parser p1, Parser p2) { return Parser(h_butnot(p1.parser, p2.parser)); }
  static inline Parser Difference(Parser p1, Parser p2) { return Parser(h_difference(p1.parser, p2.parser)); }
  static inline Parser Xor(Parser p1, Parser p2) { return Parser(h_xor(p1.parser, p2.parser)); }
  static inline Parser Many(Parser p) { return Parser(h_many(p.parser)); }
  static inline Parser Many1(Parser p) { return Parser(h_many1(p.parser)); }
  static inline Parser RepeatN(Parser p, size_t n) { return Parser(h_repeat_n(p.parser, n)); }
  static inline Parser Optional(Parser p) { return Parser(h_optional(p.parser)); }
  static inline Parser Ignore(Parser p) { return Parser(h_ignore(p.parser)); }
  static inline Parser SepBy(Parser p, Parser sep) { return Parser(h_sepBy(p.parser, sep.parser)); }
  static inline Parser SepBy1(Parser p, Parser sep) { return Parser(h_sepBy1(p.parser, sep.parser)); }
  static inline Parser Epsilon() { return Parser(h_epsilon_p()); }
  static inline Parser LengthValue(Parser length, Parser value) { return Parser(h_length_value(length.parser, value.parser)); }

  static inline Parser And(Parser p) { return Parser(h_and(p.parser)); }
  static inline Parser Not(Parser p) { return Parser(h_not(p.parser)); }

  class Indirect : public Parser {
  public:
    Indirect() : Parser(h_indirect()) {}
    void bind(Parser p) {
      h_bind_indirect((HParser*)parser, p.parser);
    }
  };

  static inline Parser Bytes(size_t len) { return Parser(h_bytes(len)); }

  static inline Parser Permutation(Parser p, ...) {
    va_list ap;
    va_start(ap, p);
    HParser* ret = h_permutation__v(*(HParser**)(void*)&p, ap);
    va_end(ap);
    return Parser(ret);
  }

  // Indices to drop are terminated by -1, e.g. DropFrom(seq, 0, 2, -1).
  static inline Parser DropFrom(Parser p, ...) {
    va_list ap;
    va_start(ap, p);
    HParser* ret = h_drop_from___v(*(HParser**)(void*)&p, ap);
    va_end(ap);
    return Parser(ret);
  }

  static inline Parser WithEndianness(char endianness, Parser p) {
    return Parser(h_with_endianness(endianness, p.parser));
  }

  static inline Parser PutValue(Parser p, const char *name) {
    return Parser(h_put_value(p.parser, name));
  }
  static inline Parser GetValue(const char *name) {
    return Parser(h_get_value(name));
  }
  static inline Parser FreeValue(const char *name) {
    return Parser(h_free_value(name));
  }

  static inline Parser Bind(Parser p, HContinuation k, void *env) {
    return Parser(h_bind(p.parser, k, env));
  }
  static inline Parser Bind(Parser p, HContinuation k) {
    return Parser(h_bind(p.parser, k, NULL));
  }

  static inline Parser Skip(size_t n) { return Parser(h_skip(n)); }
  static inline Parser Seek(ssize_t offset, int whence) { return Parser(h_seek(offset, whence)); }
  static inline Parser Tell() { return Parser(h_tell()); }
}

#pragma GCC diagnostic pop
#endif
