#pragma once

#ifdef ASIO_STANDALONE
#include <asio.hpp>
namespace lightning::net {
using io_context = asio::io_context;
namespace ip = asio::ip;
using tcp = ip::tcp;
using error_code = asio::error_code;
using steady_timer = asio::steady_timer;
using asio::buffer;
using asio::async_write;
template<typename T>
using executor_work_guard = asio::executor_work_guard<T>;
template<typename T>
inline executor_work_guard<T> make_work_guard(T& ex) {
    return asio::make_work_guard(ex);
}
} // namespace lightning::net
#else
#include <boost/asio.hpp>
namespace lightning::net {
using io_context = boost::asio::io_context;
namespace ip = boost::asio::ip;
using tcp = ip::tcp;
using error_code = boost::system::error_code;
using steady_timer = boost::asio::steady_timer;
using boost::asio::buffer;
using boost::asio::async_write;
template<typename T>
using executor_work_guard = boost::asio::executor_work_guard<T>;
template<typename T>
inline executor_work_guard<T> make_work_guard(T& ex) {
    return boost::asio::make_work_guard(ex);
}
} // namespace lightning::net
#endif
