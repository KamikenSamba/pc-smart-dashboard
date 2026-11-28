#include "telemetry_receiver.h"

#include <cassert>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view kValid = "{\"cpu\":25.5,\"ram\":46}\n";

unsigned receive(TelemetryReceiver& receiver, std::string_view bytes, uint32_t now) {
  unsigned accepted = 0;
  for (char byte : bytes) {
    if (receiver.consume(byte, now)) {
      ++accepted;
    }
  }
  return accepted;
}

void checkSample(const TelemetryReceiver& receiver, float cpu, float ram) {
  assert(receiver.latest().cpu == cpu);
  assert(receiver.latest().ram == ram);
}

void acceptsIntegersDecimalsBoundariesAndCrLf() {
  TelemetryReceiver receiver(100);
  assert(receive(receiver, kValid, 200) == 1);
  assert(receiver.status() == LinkStatus::Connected);
  checkSample(receiver, 25.5f, 46);

  assert(receive(receiver, " \t{\"ram\":1e2,\"cpu\":0,\"other\":\"x}\"} \r\n", 300) == 1);
  checkSample(receiver, 0, 100);
  assert(receive(receiver, "{\"cpu\":100,\"ram\":0}\n", 400) == 1);
  checkSample(receiver, 100, 0);
}

void rejectsInvalidFramesWithoutUpdatingEitherValue() {
  constexpr std::string_view invalid[] = {
      "\n", "[25,46]\n", "null\n", "{}\n",
      "{\"cpu\":20}\n", "{\"ram\":20}\n", "{\"cpu\":20,\"RAM\":20}\n",
      "{\"cpu\":\"12\",\"ram\":20}\n", "{\"cpu\":12,\"ram\":false}\n",
      "{\"cpu\":null,\"ram\":20}\n", "{\"cpu\":12,\"ram\":[20]}\n",
      "{\"cpu\":{\"value\":12},\"ram\":20}\n",
      "{\"cpu\":-0.5,\"ram\":20}\n", "{\"cpu\":12,\"ram\":101}\n",
      "{\"cpu\":100.000001,\"ram\":20}\n", "{\"cpu\":20,\"ram\":1e999}\n",
      "{\"cpu\":NaN,\"ram\":20}\n", "{\"cpu\":20,\"ram\":Infinity}\n",
      "{\"cpu\":12 \"ram\":20}\n", "{\"cpu\":12,\"ram\":20\n",
      "{\"cpu\":12,\"ram\":20}garbage}\n", "{\"cpu\":12,\"ram\":20}{}\n",
  };
  TelemetryReceiver receiver(0);
  assert(receive(receiver, kValid, 100) == 1);
  for (const auto frame : invalid) {
    assert(receive(receiver, frame, 200) == 0);
    assert(receiver.status() == LinkStatus::DataError);
    checkSample(receiver, 25.5f, 46);
  }
  assert(receive(receiver, "{\"cpu\":7,\"ram\":19}\n", 300) == 1);
  assert(receiver.status() == LinkStatus::Connected);
  checkSample(receiver, 7, 19);
}

void waitsForTheDelimiterAndAcceptsSeveralFrames() {
  TelemetryReceiver receiver(0);
  assert(receive(receiver, "{\"cpu\":25.5,", 100) == 0);
  assert(receive(receiver, "\"ram\":46}", 200) == 0);
  assert(receiver.status() == LinkStatus::Waiting);
  assert(receive(receiver, "\n", 300) == 1);
  checkSample(receiver, 25.5f, 46);
  receiver.updateTimeout(5299);
  assert(receiver.status() == LinkStatus::Connected);
  receiver.updateTimeout(5300);
  assert(receiver.status() == LinkStatus::NoSignal);

  assert(receive(receiver, "{\"cpu\":7,\"ram\":19}\n{\"cpu\":8,\"ram\":20}\n", 5301) == 2);
  assert(receiver.status() == LinkStatus::Connected);
  checkSample(receiver, 8, 20);
}

void dropsOversizedAndBinaryLinesUntilTheNextDelimiter() {
  TelemetryReceiver receiver(0);
  const std::string limit = std::string(256 - (kValid.size() - 1), ' ') + std::string(kValid);
  assert(receive(receiver, limit, 100) == 1);

  assert(receive(receiver, std::string(257, ' '), 200) == 0);
  assert(receiver.status() == LinkStatus::DataError);
  assert(receive(receiver, "{\"cpu\":7,\"ram\":19}\n", 300) == 0);
  checkSample(receiver, 25.5f, 46);

  const std::string binary = "{\"cpu\":7,\"ram\":19}" + std::string(1, '\0') + "\n";
  assert(receive(receiver, binary, 400) == 0);
  checkSample(receiver, 25.5f, 46);
  assert(receive(receiver, "{\"cpu\":7,\"ram\":19}\n", 500) == 1);
  checkSample(receiver, 7, 19);
}

void goesOfflineWhenNoValidDataHasArrivedSinceStartup() {
  TelemetryReceiver receiver(100);
  receiver.updateTimeout(5099);
  assert(receiver.status() == LinkStatus::Waiting);
  receiver.updateTimeout(5100);
  assert(receiver.status() == LinkStatus::NoSignal);
  checkSample(receiver, 0, 0);
  assert(receive(receiver, kValid, 5101) == 1);
  assert(receiver.status() == LinkStatus::Connected);
}

void invalidDataDoesNotRefreshTheValidSampleTimer() {
  TelemetryReceiver receiver(100);
  assert(receive(receiver, kValid, 800) == 1);
  receiver.updateTimeout(5100);
  assert(receiver.status() == LinkStatus::Connected);
  assert(receive(receiver, "{\"cpu\":10}\n", 5200) == 0);
  receiver.updateTimeout(5799);
  assert(receiver.status() == LinkStatus::DataError);
  receiver.updateTimeout(5800);
  assert(receiver.status() == LinkStatus::NoSignal);
  assert(receive(receiver, "{\"cpu\":10}\n", 5801) == 0);
  assert(receiver.status() == LinkStatus::NoSignal);
  checkSample(receiver, 25.5f, 46);
  assert(receive(receiver, "{\"cpu\":55,\"ram\":10}\n", 5802) == 1);
  assert(receiver.status() == LinkStatus::Connected);
  checkSample(receiver, 55, 10);
}

void timeoutStillWorksWhenMillisWraps() {
  constexpr uint32_t max = std::numeric_limits<uint32_t>::max();
  TelemetryReceiver receiver(max - 200);
  assert(receive(receiver, kValid, max - 100) == 1);
  receiver.updateTimeout(4898);
  assert(receiver.status() == LinkStatus::Connected);
  receiver.updateTimeout(4899);
  assert(receiver.status() == LinkStatus::NoSignal);
  assert(receive(receiver, kValid, 4900) == 1);
  assert(receiver.status() == LinkStatus::Connected);
}

}  // namespace

int main() {
  acceptsIntegersDecimalsBoundariesAndCrLf();
  rejectsInvalidFramesWithoutUpdatingEitherValue();
  waitsForTheDelimiterAndAcceptsSeveralFrames();
  dropsOversizedAndBinaryLinesUntilTheNextDelimiter();
  goesOfflineWhenNoValidDataHasArrivedSinceStartup();
  invalidDataDoesNotRefreshTheValidSampleTimer();
  timeoutStillWorksWhenMillisWraps();
  std::puts("Telemetry receiver: 7 test groups passed");
}
