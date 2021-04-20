#ifndef MAIN_HH_
#define MAIN_HH_

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../lib/cpp-httplib/httplib.h"

#include "server/server.hh"
#include "server/search.hh"
#include "client/client.cc"

#include <iostream>
#include <utility>

#include <boost/lexical_cast.hpp>

constexpr int PORT_DEFAULT = 9080;

#endif
