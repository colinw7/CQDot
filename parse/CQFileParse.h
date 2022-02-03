#ifndef CQFileParse_H
#define CQFileParse_H

#include <QString>

#include <deque>
#include <iostream>
#include <sys/types.h>

class QFile;

using uchar = unsigned char;

/*!
 * Parser for file
 */
class CQFileParse {
 public:
  CQFileParse(const QString &fileName);

 ~CQFileParse();

  //! get/set is stream (stream is file based, non-stream is line based)
  void setStream(bool stream) { stream_ = stream; }
  bool getStream() const { return stream_; }

  //! get/set filename
  const QString &fileName() const { return fileName_; }
  void setFileName(const QString &v) { fileName_ = v; }

  //! get/set current line number
  int lineNum() const { return lineNum_; }
  void setLineNum(int i) { lineNum_ = i; }

  //! get/set current char number
  int charNum() const { return charNum_; }
  void setCharNum(int i) { charNum_ = i; }

  //---

  //! skip space/non-space (for space separated words)
  bool skipSpace();
  bool skipNonSpace();

  //! skip to end
  bool skipToEnd();

  //! read space/non-space text (for space separated words)
  bool readSpace(QString &text);
  bool readNonSpace(QString &text);

  //! skip one or more characters
  bool skipChar(uint num=1);

  //! read number, real, string
  bool readInteger(int *integer);
  bool readInteger(uint *integer);

  bool readBaseInteger(uint base, int *integer);
  bool readBaseInteger(uint base, uint *integer);

  bool readReal(double *real);

  bool readString(QString &str, bool stripQuotes=false);

  //! read to specified character (return if found)
  bool readToChar(char c, QString &text);

  //! check and read identifier (underscore/alpha + underscore/alpnum)
  bool isIdentifier();
  bool readIdentifier(QString &identifier);

  //! check for space/non-space
  bool isSpace();
  bool isNonSpace();

  //! check for character types
  bool isAlpha();
  bool isAlnum();
  bool isDigit();
  bool isBaseChar(uint base);

  //! check for character/string
  bool isOneOf(const QString &str);
  bool isChar(char c);
  bool isNextChar(char c);
  bool isString(const QString &str);

  //! check eol (non-stream) or eof (stream)
  bool eol();
  bool eof();

  //! read character(s)
  QString readChars(int n);

  char readChar();
  bool readChar(char *c);
  bool readChar(uchar *c);

  //! check character (not consumed)
  char lookChar();
  bool lookChar(uchar *c);

  //! check next character (not consumed)
  char lookNextChar();
  bool lookNextChar(uchar *c);

  //! read a line into buffer (non-stream)
  void loadLine();

  //! unread string
  void unread(const QString &str);

  //! print buffer
  void dumpBuffer(std::ostream &os=std::cout);

  //! get buffer
  QString getBuffer();

  //! rewind to start
  bool rewind();

 private:
  //! check eol/eof based on stream type
  bool eof1();

  bool readOneChar(uchar &c);

 private:
  typedef std::deque<char> CharBuffer;

  QFile*     file_;                //!< file
  CharBuffer buffer_;              //!< buffer
  bool       stream_  { false };   //!< is stream based (not line based)
  bool       remove_  { true };    //!< is file created by class (deleted by destructor)
  QString    fileName_;            //!< file name (if known)
  int        lineNum_ { 1 };       //!< current line number
  int        charNum_ { 0 };       //!< current char number
};

#endif
