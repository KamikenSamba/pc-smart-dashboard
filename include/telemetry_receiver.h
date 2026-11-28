#pragma once

#include <cstddef>
#include <cstdint>

enum class LinkStatus {
  Waiting,
  Connected,
  DataError,
  NoSignal,
};

struct UsageSample {
  float cpu = 0;
  float ram = 0;
};

class TelemetryReceiver {
 public:
  static constexpr uint32_t kTimeoutMs = 5000;

  explicit TelemetryReceiver(uint32_t startedAt) : lastValidAt_(startedAt) {}

  // Returns true when a complete, valid sample has been accepted.
  bool consume(char byte, uint32_t now);
  void updateTimeout(uint32_t now);
  LinkStatus status() const { return status_; }
  const UsageSample& latest() const { return latest_; }

 private:
  static constexpr size_t kMaxFrameBytes = 256;

  void rejectFrame();

  uint32_t lastValidAt_;
  LinkStatus status_ = LinkStatus::Waiting;
  UsageSample latest_;
  char frame_[kMaxFrameBytes];
  size_t frameSize_ = 0;
  bool discarding_ = false;
};
