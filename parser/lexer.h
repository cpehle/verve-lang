#include <fstream>
#include <memory>

#include "./token.h"

#pragma once

namespace Verve {
  struct Pos {
    int line;
    int column;
  };

  class Lexer {
    public:
      Lexer(std::string filename, const char *input) :
        m_filename(filename),
        m_input(input),
        m_pos(0),
        m_token(Token(Token::END)),
        m_prevToken(Token(Token::END))
      {
        nextToken();
      }

      Token &token();
      Token &token(Token::Type);

      void rewind();
      void rewind(Loc &loc);

      bool next(int c);
      bool skip(int c);
      void match(int c);

      void nextToken();

      void _Noreturn invalidToken();
      void printSource();
      void printSource(Loc loc);
      void _Noreturn error(Loc loc, const char *, ...);

      static std::string tokenType(Token &token);

    private:
      char nextChar();

      Pos getSourcePosition(Loc loc);

      static std::string basicTokenToString(int t);

      std::string m_filename;
      const char *m_input;
      unsigned m_pos;
      Token m_token;
      Token m_prevToken;
  };

}
