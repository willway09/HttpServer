#ifndef _SERVEREXCEPTION_HPP
#define _SERVEREXCEPTION_HPP

#include <stdexcept>

class ServerException : public std::exception {
private:
    std::string description;

public:
  ServerException(const std::string& s) {
    description = s;
  }

  virtual const char* what() const throw() {
    return description.c_str();
  }
};

#endif //_SERVEREXCEPTION_HPP
