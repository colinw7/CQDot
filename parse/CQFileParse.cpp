#include <CQFileParse.h>
#include <QFile>
#include <cassert>

CQFileParse::
CQFileParse(const QString &fileName) :
 fileName_(fileName)
{
  file_ = new QFile(fileName_);

  file_->open(QIODevice::ReadOnly);
}

CQFileParse::
~CQFileParse()
{
  if (remove_)
    delete file_;
}

bool
CQFileParse::
skipSpace()
{
  if (isSpace()) {
    while (isSpace())
      readChar();

    return true;
  }
  else
    return false;
}

bool
CQFileParse::
skipNonSpace()
{
  if (isNonSpace()) {
    while (isNonSpace())
      readChar();

    return true;
  }
  else
    return false;
}

bool
CQFileParse::
skipToEnd()
{
  if (! eof1()) {
    while (skipChar()) { }

    return true;
  }
  else
    return false;
}

bool
CQFileParse::
readSpace(QString &text)
{
  text = "";

  if (eof1() || isNonSpace())
    return false;

  while (isSpace())
    text += readChar();

  return true;
}

bool
CQFileParse::
readNonSpace(QString &text)
{
  text = "";

  if (eof1() || isSpace())
    return false;

  while (isNonSpace())
    text += readChar();

  return true;
}

bool
CQFileParse::
readInteger(int *integer)
{
  QString str;

  if (isChar('+') || isChar('-'))
    str += readChar();

  if (eof1() || ! isDigit()) {
    unread(str);
    return false;
  }

  str += readChar();

  while (isDigit())
    str += readChar();

  if (integer != nullptr) {
    bool ok;
    *integer = str.toInt(&ok);
  }

  return true;
}

bool
CQFileParse::
readInteger(uint *integer)
{
  QString str;

  if (eof1() || ! isDigit()) {
    unread(str);
    return false;
  }

  str += readChar();

  while (isDigit())
    str += readChar();

  if (integer != nullptr) {
    bool ok;
    *integer = str.toInt(&ok);
  }

  return true;
}

bool
CQFileParse::
readBaseInteger(uint base, int *integer)
{
  QString str;

  if (isChar('+') || isChar('-'))
    str += readChar();

  if (eof1() || ! isBaseChar(base)) {
    unread(str);
    return false;
  }

  str += readChar();

  while (isBaseChar(base))
    str += readChar();

  if (integer != nullptr) {
     bool ok;
    *integer = str.toInt(&ok, base);
    //*integer = CStrUtil::toBaseInteger(str, base);
  }

  return true;
}

bool
CQFileParse::
readBaseInteger(uint base, uint *integer)
{
  QString str;

  if (eof1() || ! isBaseChar(base)) {
    unread(str);
    return false;
  }

  str += readChar();

  while (isBaseChar(base))
    str += readChar();

  if (integer != nullptr) {
    bool ok;
    *integer = str.toInt(&ok, base);
    //*integer = CStrUtil::toBaseInteger(str, base);
  }

  return true;
}

bool
CQFileParse::
readReal(double *real)
{
  QString str;

  //------

  if (isChar('+') || isChar('-'))
    str += readChar();

  //------

  while (isDigit())
    str += readChar();

  //------

  if (isChar('.')) {
    str += readChar();

    while (isDigit())
      str += readChar();
  }

  //------

  if (isChar('e') || isChar('E')) {
    str += readChar();

    if (isChar('+') || isChar('-'))
      str += readChar();

    if (eof1() || ! isDigit()) {
      unread(str);
      return false;
    }

    while (isDigit())
      str += readChar();
  }

  //------

  if (str.size() == 0)
    return false;

  if (real != nullptr) {
    bool ok;
    *real = str.toDouble(&ok);
    //*real = CStrUtil::toReal(str);
  }

  //------

  return true;
}

bool
CQFileParse::
readString(QString &str, bool stripQuotes)
{
  str = "";

  if (eof1() || (! isChar('\"') && ! isChar('\'')))
    return false;

  char strChar = readChar();

  if (! stripQuotes)
    str += strChar;

  while (! eof1()) {
    if      (isChar('\\')) {
      str += readChar();

      if (! eof1())
        str += readChar();
    }
    else if (isChar(strChar))
      break;
    else
      str += readChar();
  }

  if (eof1()) {
    unread(str);
    return false;
  }

  strChar = readChar();

  if (! stripQuotes)
    str += strChar;

  return true;
}

bool
CQFileParse::
readToChar(char c, QString &str)
{
  str = "";

  while (! eof1() && ! isChar(c))
    str += readChar();

  if (eof1()) {
    unread(str);
    return false;
  }

  return true;
}

bool
CQFileParse::
isIdentifier()
{
  if (eof1())
    return false;

  if (! isChar('_') && ! isAlpha())
    return false;

  return true;
}

bool
CQFileParse::
readIdentifier(QString &identifier)
{
  if (isChar('_') || isAlpha()) {
    identifier = readChar();

    while (isChar('_') || isAlnum())
      identifier += readChar();

    return true;
  }
  else
    return false;
}

bool
CQFileParse::
isSpace()
{
  return (! eof1() && isspace(lookChar()));
}

bool
CQFileParse::
isNonSpace()
{
  return (! eof1() && ! isspace(lookChar()));
}

bool
CQFileParse::
isAlpha()
{
  return (! eof1() && isalpha(lookChar()));
}

bool
CQFileParse::
isAlnum()
{
  return (! eof1() && isalnum(lookChar()));
}

bool
CQFileParse::
isDigit()
{
  return (! eof1() && isdigit(lookChar()));
}

bool
CQFileParse::
isOneOf(const QString &str)
{
  auto isCharStr = [&](char c) { return (str.indexOf(c) != -1); };

  return (! eof1() && isCharStr(lookChar()));
}

bool
CQFileParse::
isBaseChar(uint base)
{
  if (eof1())
    return false;

  static std::string s_base_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  if (base < 2 || base > s_base_chars.length())
    return false;

  int c1 = lookChar();

  if (std::islower(c1))
    c1 = std::toupper(c1);

  auto pos = s_base_chars.find(static_cast<char>(c1));

  if (pos == std::string::npos || pos >= base)
    return false;

  return true;
}

bool
CQFileParse::
isChar(char c)
{
  return (! eof1() && lookChar() == c);
}

bool
CQFileParse::
isNextChar(char c)
{
  return (! eof1() && lookNextChar() == c);
}

bool
CQFileParse::
isString(const QString &str)
{
  int pos = 0;
  int len = str.size();

  QString str1;

  while (! eof1() && pos < len && isChar(str[pos].toLatin1())) {
    str1 += readChar();

    ++pos;
  }

  if (pos != len) {
    if (pos > 0)
      unread(str1);

    return false;
  }

  unread(str1);

  return true;
}

bool
CQFileParse::
eof1()
{
  if (stream_)
    return eof();

  return eol() || eof();
}

bool
CQFileParse::
eol()
{
  return buffer_.empty();
}

bool
CQFileParse::
eof()
{
  return file_->atEnd();
}

bool
CQFileParse::
skipChar(uint num)
{
  for (uint i = 0; i < num; ++i) {
    uchar c;

    if (! readChar(&c))
      return false;
  }

  return true;
}

QString
CQFileParse::
readChars(int n)
{
  QString str;

  for (int i = 0; i < n; ++i) {
    char c;

    if (! readChar(&c))
      break;

    str += c;
  }

  return str;
}

char
CQFileParse::
readChar()
{
  uchar c;

  (void) readChar(&c);

  return c;
}

bool
CQFileParse::
readChar(char *c)
{
  uchar c1;

  if (! readChar(&c1))
    return false;

  *c = c1;

  return true;
}

bool
CQFileParse::
readChar(uchar *c)
{
  if (c != nullptr)
    *c = '\0';

  uchar c1;

  if (! buffer_.empty()) {
    c1 = buffer_[0];

    buffer_.pop_front();
  }
  else {
    if (eof1())
      return false;

    if (! readOneChar(c1))
      return false;
  }

  if (c != nullptr)
    *c = c1;

  if (c1 == '\n') {
    ++lineNum_;

    charNum_ = 0;
  }
  else
    ++charNum_;

  return true;
}

char
CQFileParse::
lookChar()
{
  uchar c;

  lookChar(&c);

  return c;
}

char
CQFileParse::
lookNextChar()
{
  uchar c;

  lookNextChar(&c);

  return c;
}

bool
CQFileParse::
lookChar(uchar *c)
{
  if (c)
    *c = '\0';

  if (! buffer_.empty()) {
    if (c)
      *c = buffer_[0];
  }
  else {
    if (eof1())
      return false;

    uchar c1;

    if (! readOneChar(c1))
      return false;

    if (c)
      *c = c1;

    buffer_.push_back(c1);
  }

  return true;
}

bool
CQFileParse::
lookNextChar(uchar *c)
{
  if (c != nullptr)
    *c = '\0';

  if      (buffer_.size() > 1) {
    if (c != nullptr)
      *c = buffer_[1];
  }
  else {
    if (eof1())
      return false;

    if (! buffer_.empty()) {
      uchar c1;

      if (! readOneChar(c1))
        return false;

      if (c != nullptr)
        *c = c1;

      buffer_.push_back(c1);
    }
    else {
      char c1[2];

      auto num_read = file_->read(c1, 2);

      if (num_read == 2) {
        if (c != nullptr)
          *c = c1[1];

        buffer_.push_back(c1[0]);
        buffer_.push_back(c1[1]);
      }
      else {
        if (num_read == 1)
           buffer_.push_back(c1[0]);

        return false;
      }
    }
  }

  return true;
}

void
CQFileParse::
loadLine()
{
  uchar c;

  buffer_.clear();

  while (! eof()) {
    if (! readOneChar(c))
      break;

    if (c == '\n')
      break;

    if (c != '\0')
      buffer_.push_back(c);
  }
}

void
CQFileParse::
unread(const QString &str)
{
  QString::size_type len = str.size();

  QString::size_type j = len - 1;

  for (QString::size_type i = 0; i < len; ++i, --j)
    buffer_.push_front(str[j].toLatin1());
}

bool
CQFileParse::
readOneChar(uchar &c)
{
  auto num_read = file_->read(reinterpret_cast<char *>(&c), 1);

  return (num_read == 1);
}

void
CQFileParse::
dumpBuffer(std::ostream &os)
{
  os << ">>" << getBuffer().toStdString() << "<<" << std::endl;
}

QString
CQFileParse::
getBuffer()
{
  QString str;

  CharBuffer::iterator p1 = buffer_.begin();
  CharBuffer::iterator p2 = buffer_.end  ();

  for ( ; p1 != p2; ++p1)
    str += *p1;

  return str;
}

bool
CQFileParse::
rewind()
{
  if (! file_->reset())
    return false;

  buffer_.clear();

  lineNum_ = 1;
  charNum_ = 0;

  return true;
}
