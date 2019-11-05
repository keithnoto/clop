#ifndef CLOP_DAU_H
#define CLOP_DAU_H

#include <exception>
#include <iostream>
#include <sstream>
#include <cstring>

namespace noto {

// exception for I/O methods
class dau_t : public std::exception {

  public:

    std::ostringstream oss;

    dau_t() { }
    dau_t(const dau_t &peer) { oss << peer.what(); }
    dau_t(const std::string &msg) { oss << msg; }

    template<typename P> dau_t& operator<<(const P &msg) {
        oss << msg;
        oss.flush();
        return *this;
    }

    const char* what() const throw() {
        char *msg = new char[oss.str().size() + 1];   // leak
        strcpy(msg, oss.str().c_str());
        return msg;
		// return oss.str().c_str();  // not sure why returning oss.str().c_str() isn't good enough, but the error message is missing and this fixes it (???)
    }

    ~dau_t() throw() { }

};

typedef dau_t DAU;

} // namespace

#endif
