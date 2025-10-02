#pragma once

#include <fstream>

#include "baseexception.h"

class FileWriter {
public:
  class OpenException : public BaseException {
  public:
    OpenException(const std::string &path)
        : BaseException("Could not open file for writing at " + path) {}
  };

  FileWriter(const std::string &path) {
    hdl.rdbuf()->pubsetbuf(0, 0);
    hdl.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!hdl.is_open()) {
      throw OpenException(path);
    }
  }

  void onData(const char *data, std::size_t size) {
    assert(size > 0);
    hdl.write(data, size);
  }

  template <typename MemType> MemType onBestow(MemType mem) {
    // Don't need the memory any more (already consumed by write), so can immediately give back to
    // producer for re-use.
    return std::forward<MemType>(mem);
  }

  template <typename AckType> void onFlush(AckType ack) {
    hdl.flush();
    ack();
  }

  ~FileWriter() { hdl.close(); }

private:
  std::ofstream hdl;
};
