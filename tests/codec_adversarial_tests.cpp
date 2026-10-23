#include <spiral/core/contract.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace {
using spiral::core::DecodeError;
using spiral::core::Message;
using spiral::core::MessageCodec;

std::uint64_t state = 0x7b6a45d2e31c908fULL;
std::uint64_t next_random() {
  // Fixed-seed PRNG: this is a reproducible regression corpus, not crypto.
  state ^= state >> 12;
  state ^= state << 25;
  state ^= state >> 27;
  return state * 2685821657736338717ULL;
}

bool fail(const char* label, std::size_t iteration) {
  std::cerr << "B44 codec adversarial regression: " << label
            << " (case " << iteration << ")\n";
  return false;
}

bool no_message(const spiral::core::DecodeResult& decoded,
                DecodeError expected, const char* label, std::size_t iteration) {
  if (decoded.message || decoded.error != expected || decoded) {
    return fail(label, iteration);
  }
  return true;
}

bool exercise_valid(const Message& source, std::size_t iteration) {
  const auto wire = MessageCodec::encode(source);
  if (!wire) return fail("valid input refused by encoder", iteration);
  const auto decoded = MessageCodec::decode(*wire);
  if (!decoded || decoded.error != DecodeError::none ||
      decoded.message->version != source.version ||
      decoded.message->topic != source.topic ||
      decoded.message->payload != source.payload) {
    return fail("valid wire roundtrip", iteration);
  }
  // Check transport boundaries and the checksum without relying on asserts.
  auto changed = *wire;
  changed.pop_back();
  if (!no_message(MessageCodec::decode(changed), DecodeError::truncated,
                  "truncated checksum accepted", iteration)) return false;
  changed = *wire;
  changed.push_back(0);
  if (!no_message(MessageCodec::decode(changed), DecodeError::trailing_data,
                  "trailing data accepted", iteration)) return false;
  changed = *wire;
  changed.back() ^= 0x80;
  if (!no_message(MessageCodec::decode(changed), DecodeError::checksum_mismatch,
                  "modified checksum accepted", iteration)) return false;
  changed = *wire;
  // A valid topic byte changed to another valid character must still fail
  // the checksum, rather than only being caught by topic validation.
  changed[12] = changed[12] == 'x' ? 'y' : 'x';
  if (!no_message(MessageCodec::decode(changed), DecodeError::checksum_mismatch,
                  "modified valid topic accepted", iteration)) return false;
  if (!source.payload.empty()) {
    changed = *wire;
    const auto offset = 12 + source.topic.size() +
                        (next_random() % source.payload.size());
    changed[offset] ^= 0x01;
    if (!no_message(MessageCodec::decode(changed), DecodeError::checksum_mismatch,
                    "modified binary payload accepted", iteration)) return false;
  }
  return true;
}
} // namespace

int main() {
  constexpr std::array<std::size_t, 12> sizes =
      {0, 1, 2, 15, 16, 127, 128, 255, 256, 257, 1024, 4096};
  constexpr char alphabet[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-";
  std::size_t valid_cases = 0;
  for (std::size_t size : sizes) {
    for (std::size_t sample = 0; sample < 24; ++sample) {
      Message message;
      const auto topic_size = 1 + (next_random() % 128);
      message.topic.reserve(topic_size);
      for (std::size_t i = 0; i < topic_size; ++i)
        message.topic.push_back(alphabet[next_random() % (sizeof(alphabet) - 1)]);
      message.payload.resize(size);
      for (auto& byte : message.payload)
        byte = static_cast<std::uint8_t>(next_random());
      if (!exercise_valid(message, valid_cases)) return 1;
      ++valid_cases;
    }
  }

  // Fuzz arbitrary hostile wire spans with a stable seed and bounded sizes.
  // If a frame decodes, its canonical re-encoding must be byte-identical.
  // Invalid frames must never expose a partially populated Message.
  constexpr std::size_t hostile_cases = 25000;
  for (std::size_t i = 0; i < hostile_cases; ++i) {
    const auto length = static_cast<std::size_t>(next_random() % 513);
    std::vector<std::uint8_t> hostile(length);
    for (auto& byte : hostile) byte = static_cast<std::uint8_t>(next_random());
    const auto decoded = MessageCodec::decode(hostile);
    if (decoded) {
      if (decoded.error != DecodeError::none) { fail("success with error", i); return 2; }
      const auto recoded = MessageCodec::encode(*decoded.message);
      if (!recoded || *recoded != hostile) { fail("noncanonical accepted frame", i); return 2; }
    } else if (decoded.message || decoded.error == DecodeError::none) {
      fail("invalid frame leaked message or success status", i);
      return 2;
    }
  }
  std::cout << "B44: " << valid_cases << " legal binary frames with 4-5 mutations each; "
            << hostile_cases << " fixed-seed hostile frames verified\n";
  return 0;
}
