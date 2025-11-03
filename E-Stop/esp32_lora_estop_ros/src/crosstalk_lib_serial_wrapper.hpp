// The MIT License (MIT)
//
// Copyright (c) 2025 Stefan Fabian
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef CROSSTALK_LIB_SERIAL_WRAPPER_HPP
#define CROSSTALK_LIB_SERIAL_WRAPPER_HPP

#ifndef CROSSTALK_SERIAL_ABSTRACTION_HPP
  #error "Include crosstalk.hpp or crosstalk/serial_abstraction.hpp before including lib_serial_wrapper.hpp"
#endif // CROSSTALK_SERIAL_ABSTRACTION_HPP

#include <libserial/SerialPort.h>

namespace crosstalk
{
class LibSerialWrapper : public crosstalk::SerialAbstraction
{
public:
  explicit LibSerialWrapper( LibSerial::SerialPort &serial ) : serial_( serial ) { }

  int available() const override { return serial_.GetNumberOfBytesAvailable(); }

  int read( uint8_t *data, size_t length ) override
  {
    buffer_.clear();
    buffer_.reserve( length );
    serial_.Read( buffer_, length );
    std::memcpy( data, buffer_.data(), std::min( length, buffer_.size() ) );
    return static_cast<int>( buffer_.size() );
  }

  void write( const uint8_t *data, size_t length ) override {
    buffer_.clear();
    buffer_.assign(data, data + length);
    serial_.Write( buffer_ );
  }

private:
  LibSerial::SerialPort &serial_;
  std::vector<uint8_t> buffer_; // Buffer to hold read data
};
} // namespace crosstalk

#endif // CROSSTALK_LIB_SERIAL_WRAPPER_HPP
