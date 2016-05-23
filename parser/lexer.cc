#include "lexer.h"

#include "token.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <math.h>

#define BASIC_TOKEN(CHAR) \
  case CHAR: \
    m_token = Token(Token::BASIC); \
    m_token.value.number = CHAR; \
    break;

namespace ceos {

  char Lexer::nextChar() {
    char c = m_input[m_pos];
    if (c != '\0') {
      m_pos++;
    }
    return c;
  }

  void Lexer::nextToken() {
    char c;
    m_prevToken = std::move(m_token);

start:
    do {
      c = nextChar();
    } while(isspace(c));

    int start = m_pos - 1;
    switch (c) {
      BASIC_TOKEN('(')
      BASIC_TOKEN(')')
      BASIC_TOKEN('{')
      BASIC_TOKEN('}')
      BASIC_TOKEN('<')
      BASIC_TOKEN('>')
      BASIC_TOKEN(',')
      BASIC_TOKEN(':')

      case '\0':
        start = m_token.loc.end > 0 ? m_token.loc.end - 1 : 0;
        m_token = Token(Token::END);
        break;

      case '/': {
        char c = nextChar();

        if (c == '/') {
          do {
            c = nextChar();
          } while (c != '\n');
        } else if(c == '*') {
          char prev;
          do {
            prev = c;
            c = nextChar();
          } while (prev != '*' || c != '/');
        } else {
          printSource();
          throw std::runtime_error("Invalid token after /");
        }
        goto start;
      }

      case '"': {
        auto start = m_pos;
        unsigned length = 0;
        while ((c = nextChar()) != '"') {
          length++;
        }
        const char *str = (const char *)calloc(length + 1, 1);
        memcpy((void *)str, m_input+start, length);
        m_token = Token(Token::STRING, str);
        break;
      }

      case '\'': {
        int number = nextChar();
        assert(nextChar() == '\'');
        m_token = Token(Token::NUMBER, number);
        break;
      }

      default:
        if (isnumber(c)) {
          int number = 0;
          do {
            number *= 10;
            number += c - '0';
          } while (isnumber(c = nextChar()));

          m_pos--;

          m_token = Token(Token::NUMBER, number);
        } else if (isalpha(c) || c == '_') {
          auto start = m_pos - 1;
          unsigned length = 0;
          do {
            length++;
          } while (isalpha(c = nextChar()) || isnumber(c) || c == '_' || c == '-');

          m_pos--;

          const char *str = (const char *)calloc(length + 1, 1);
          memcpy((void *)str, m_input + start, length);
          m_token = Token(Token::ID, str);
        } else {
          // TODO: proper error here
          std::cerr << "Invalid token `" << c << "`\n";
          throw;
        }
    }

    m_token.loc.start = start;
    m_token.loc.end = m_pos;
  }

  Token &Lexer::token(void) {
    return m_token;
  }

  Token &Lexer::token(Token::Type type) {
    assert(m_token.type == type);
    nextToken();
    return m_prevToken;
  }

  void Lexer::rewind() {
    m_token = std::move(m_prevToken);
    m_prevToken = Token(Token::END);
    m_pos = m_token.loc.end;
  }

  void Lexer::rewind(Loc &loc) {
    m_pos = loc.start;
    nextToken();
  }

  bool Lexer::next(char c) {
    return m_token.type == Token::BASIC && m_token.number() == c;
  }

  bool Lexer::skip(char c) {
    if (next(c)) {
      nextToken();
      return true;
    }
    return false;
  }

  void Lexer::match(char c) {
    if (next(c)) {
      nextToken();
    } else {
      std::cerr << "Invalid token found: expected `" << Token::typeName(m_token.type) << "` to be `" << c << "`" << "\n";
      printSource();
      throw std::runtime_error("Parser error");
    }
  }

   void Lexer::invalidToken() {
    if (m_token.type == Token::END) {
      std::cerr << "Unexpected end of input\n";
    } else {
      Pos pos = getSourcePosition(m_token.loc);
      std::cerr << "Unexpected token `" << Token::typeName(m_token.type) << "` at " << pos.line << ":" << pos.column << std::endl;
    }
    printSource();
    throw std::runtime_error("Parse error");
  }

  Pos Lexer::getSourcePosition(Loc loc) {
    Pos pos = {1, 1};
    m_pos = 0;
    size_t i = loc.start >= m_offset ? m_offset : 0;
    for (; i < loc.start; i++) {
      if (m_input[i] == '\n') {
        pos.line++;
        pos.column = 1;
      } else {
        pos.column++;
      }
    }
    return pos;
  }

  void Lexer::printSource() {
    printSource(m_token.loc);
  }

  void Lexer::printSource(Loc loc) {
    Pos pos = getSourcePosition(loc);

    int start = loc.start;
    int actualStart = start;

    while(start > 0 && m_input[start - 1] != '\n') {
      start--;
    }
    m_pos = start;

    char line[256];
    unsigned i = 0;
    while ((line[i++] = nextChar()) != '\n');
    line[i] = 0;

    const char *separator = ": ";
    int lineNoWidth = ceil(log10(pos.line + 1));

    std::cerr << pos.line << separator << line;
    std::cerr << std::setw((actualStart - start) + 2 + strlen(separator) + lineNoWidth) << "^\n";
  }

  void Lexer::error(Loc loc, const char *message, ...) {
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fputc('\n', stderr);
    printSource(loc);
    throw std::runtime_error("Parser error");
  }
}
