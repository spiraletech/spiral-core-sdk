#include <spiral/core/contract.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <span>
#include <string>
#include <vector>

namespace {
std::atomic<std::size_t> allocations{0};
thread_local bool count_allocations = false;
}

void* operator new(std::size_t n) {
    if (count_allocations) allocations.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
    if (count_allocations) allocations.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
using spiral::core::DecodeError;
using spiral::core::Message;
using spiral::core::MessageCodec;
using Bytes = std::vector<std::uint8_t>;

std::uint32_t independent_checksum(std::span<const std::uint8_t> data) {
    std::uint32_t h = 2166136261u;
    for (const auto b : data) { h ^= b; h *= 16777619u; }
    return h;
}
void update_checksum(Bytes& frame) {
    const auto hash = independent_checksum(
        std::span<const std::uint8_t>(frame.data(), frame.size() - 4));
    for (unsigned i = 0; i < 4; ++i)
        frame[frame.size() - 4 + i] = static_cast<std::uint8_t>(hash >> (8 * i));
}

bool check_rejected(const Bytes& frame, DecodeError expected,
                    std::size_t topic_size, std::size_t payload_size,
                    const char* variant) {
    allocations.store(0, std::memory_order_relaxed);
    count_allocations = true;
    const auto decoded = MessageCodec::decode(frame);
    count_allocations = false;
    const auto count = allocations.load(std::memory_order_relaxed);
    if (decoded || decoded.message || decoded.error != expected || count != 0) {
        std::cerr << "B46 rejected frame " << variant << ": topic=" << topic_size
                  << " payload=" << payload_size << " error="
                  << static_cast<int>(decoded.error) << " allocations=" << count << '\n';
        return false;
    }
    return true;
}
}

int main() {
    constexpr std::array<std::size_t, 6> topic_lengths{1, 15, 16, 31, 64, 128};
    constexpr std::array<std::size_t, 5> payload_lengths{0, 1, 128, 4096, 1048576};
    std::size_t verified = 0;
    for (const auto topic_length : topic_lengths) {
        for (const auto payload_length : payload_lengths) {
            Message source;
            source.topic = std::string(topic_length, 'a');
            source.payload.resize(payload_length);
            for (std::size_t i = 0; i < payload_length; ++i)
                source.payload[i] = static_cast<std::uint8_t>(i * 73u + 11u);
            const auto encoded = MessageCodec::encode(source);
            if (!encoded) return 2;
            const auto accepted = MessageCodec::decode(*encoded);
            if (!accepted || accepted.error != DecodeError::none ||
                accepted.message->topic != source.topic ||
                accepted.message->payload != source.payload) return 3;

            Bytes wrong_checksum = *encoded;
            wrong_checksum.back() ^= 0x80;
            if (!check_rejected(wrong_checksum, DecodeError::checksum_mismatch,
                                topic_length, payload_length, "checksum")) return 1;
            ++verified;

            Bytes invalid_topic = *encoded;
            invalid_topic[12] = '/';
            update_checksum(invalid_topic);  // independently valid checksum
            if (!check_rejected(invalid_topic, DecodeError::invalid_topic,
                                topic_length, payload_length, "topic")) return 1;
            ++verified;
        }
    }
    std::cout << "B46: " << verified
              << " rejected valid-length frames without ordinary heap allocations; "
                 "30 legal frames round-tripped\n";
    return 0;
}
