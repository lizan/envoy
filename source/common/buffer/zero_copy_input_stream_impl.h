#pragma once

#include <cstdint>
#include <string>
#include "google/protobuf/io/zero_copy_stream.h"

#include "common/buffer/buffer_impl.h"

namespace Envoy {

namespace Buffer{

class ZeroCopyInputStreamImpl : public virtual google::protobuf::io::ZeroCopyInputStream {
 public:
  ZeroCopyInputStreamImpl(Buffer::Instance& buffer);
  ZeroCopyInputStreamImpl() {}
  virtual ~ZeroCopyInputStreamImpl() {}

  // Add a buffer to input stream, will consume all buffer from parameter
  // if the stream is not finished
  void move(Buffer::Instance& instance);

  // Mark the buffer is finished
  void finish() { finished_ = true; }

  // google::protobuf::io::ZeroCopyInputStream
  virtual bool Next(const void** data, int* size) override;
  virtual void BackUp(int count) override;
  virtual bool Skip(int count) override; // Not implemented
  virtual google::protobuf::int64 ByteCount() const override { return byte_count_; }

 protected:
  Buffer::OwnedImpl buffer_;
  uint64_t position_{0};

 private:
  bool finished_{false};
  int64_t byte_count_{0};
};

} // namespace Buffer
} // namespace Envoy

