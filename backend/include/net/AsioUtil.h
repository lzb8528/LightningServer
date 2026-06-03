#pragma once

#ifdef ASIO_STANDALONE
#include <asio.hpp>
namespace lightning::net {
using io_context = asio::io_context;
namespace ip = asio::ip;
using tcp = ip::tcp;
using error_code = asio::error_code;
namespace error = asio::error;
using steady_timer = asio::steady_timer;
using asio::buffer;
using asio::async_write;
} // namespace lightning::net
#else
#include <boost/asio.hpp>
namespace lightning::net {
using io_context = boost::asio::io_context;
namespace ip = boost::asio::ip;
using tcp = ip::tcp;
using error_code = boost::system::error_code;
namespace error = boost::asio::error;
using steady_timer = boost::asio::steady_timer;
using boost::asio::buffer;
using boost::asio::async_write;
} // namespace lightning::net
#endif
