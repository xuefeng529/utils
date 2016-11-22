#ifndef BASE_EXCEPTION_H
#define BASE_EXCEPTION_H

#include <string>
#include <exception>

#include <stdint.h>

namespace base
{

class Exception : public std::exception
{
 public:
  explicit Exception(const char* what);
  explicit Exception(const std::string& what);
  virtual ~Exception() throw();
  virtual const char* what() const throw();
  const char* stackTrace() const throw();

 private:
  void fillStackTrace();

  std::string message_;
  std::string stack_;
};

}

#endif  // BASE_EXCEPTION_H
