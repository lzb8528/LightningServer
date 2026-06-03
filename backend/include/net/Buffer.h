#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

namespace lightning {

class Buffer {
public:
    static const size_t kInitialSize = 1024;
    static const size_t kMaxSize = 64 * 1024 * 1024;

    Buffer() : buffer_(kInitialSize), readIndex_(0), writeIndex_(0) {}

    size_t readableBytes() const { return writeIndex_ - readIndex_; }
    size_t writableBytes() const { return buffer_.size() - writeIndex_; }
    size_t prependableBytes() const { return readIndex_; }

    const char* peek() const { return begin() + readIndex_; }
    char* beginWrite() { return begin() + writeIndex_; }

    void retrieve(size_t len) {
        if (len < readableBytes()) {
            readIndex_ += len;
        } else {
            retrieveAll();
        }
    }

    void retrieveAll() {
        readIndex_ = 0;
        writeIndex_ = 0;
    }

    std::string retrieveAsString(size_t len) {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    void append(const char* data, size_t len) {
        ensureWritableBytes(len);
        std::memcpy(beginWrite(), data, len);
        hasWritten(len);
    }

    void append(const std::string& str) { append(str.data(), str.size()); }

    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
    }

    void hasWritten(size_t len) { writeIndex_ += len; }

private:
    char* begin() { return buffer_.data(); }
    const char* begin() const { return buffer_.data(); }

    void makeSpace(size_t len) {
        if (writableBytes() + prependableBytes() < len) {
            buffer_.resize(writeIndex_ + len);
        } else {
            size_t readable = readableBytes();
            std::copy(begin() + readIndex_, begin() + writeIndex_, begin());
            readIndex_ = 0;
            writeIndex_ = readable;
        }
    }

    std::vector<char> buffer_;
    size_t readIndex_;
    size_t writeIndex_;
};

} // namespace lightning
