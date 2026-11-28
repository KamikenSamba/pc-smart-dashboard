#include "telemetry_receiver.h"

#include <ArduinoJson.h>

namespace {

class FrameReader {
 public:
  FrameReader(const char* data, size_t length) : next_(data), end_(data + length) {}

  int read() {
    return next_ == end_ ? -1 : static_cast<unsigned char>(*next_++);
  }

  size_t readBytes(char* buffer, size_t length) {
    size_t count = 0;
    while (count < length && next_ != end_) {
      buffer[count++] = *next_++;
    }
    return count;
  }

  // ArduinoJson stops at the closing brace. Require a single object per line.
  bool hasOnlyTrailingWhitespace() const {
    for (const char* byte = next_; byte != end_; ++byte) {
      if (*byte != ' ' && *byte != '\t' && *byte != '\r') {
        return false;
      }
    }
    return true;
  }

 private:
  const char* next_;
  const char* end_;
};

bool readUsage(JsonVariantConst field, float& usage) {
  // is<double>() accepts integers and decimals. Missing keys and null fail it.
  if (!field.is<double>()) {
    return false;
  }
  const double value = field.as<double>();
  if (!(value >= 0.0 && value <= 100.0)) {
    return false;
  }
  usage = static_cast<float>(value);
  return true;
}

bool parseSample(const char* data, size_t length, UsageSample& sample) {
  FrameReader input(data, length);
  JsonDocument doc;
  if (deserializeJson(doc, input) || !input.hasOnlyTrailingWhitespace()) {
    return false;
  }
  const JsonObjectConst object = doc.as<JsonObjectConst>();
  return readUsage(object["cpu"], sample.cpu) && readUsage(object["ram"], sample.ram);
}

}  // namespace

bool TelemetryReceiver::consume(char byte, uint32_t now) {
  if (byte != '\n') {
    if (!discarding_) {
      if (frameSize_ == sizeof(frame_) || byte == '\0') {
        discarding_ = true;
        rejectFrame();
      } else {
        frame_[frameSize_++] = byte;
      }
    }
    return false;
  }

  const size_t length = frameSize_;
  frameSize_ = 0;
  if (discarding_) {
    discarding_ = false;
    return false;
  }

  UsageSample sample;
  if (!parseSample(frame_, length, sample)) {
    rejectFrame();
    return false;
  }

  latest_ = sample;
  lastValidAt_ = now;
  status_ = LinkStatus::Connected;
  return true;
}

void TelemetryReceiver::updateTimeout(uint32_t now) {
  // Unsigned subtraction also works across the millis() rollover.
  if (now - lastValidAt_ >= kTimeoutMs) {
    status_ = LinkStatus::NoSignal;
  }
}

void TelemetryReceiver::rejectFrame() {
  // Once timed out, only a valid frame can restore the link.
  if (status_ != LinkStatus::NoSignal) {
    status_ = LinkStatus::DataError;
  }
}
