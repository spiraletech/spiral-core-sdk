#pragma once
#include <spiral/core/contract.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace spiral::core {
// Stream framing for the existing SPRL v1 wire contract. Not real-time safe.
// A malformed frame poisons the decoder until reset(); no implicit resync.
enum class FrameError { none, malformed, poisoned };
struct FrameBatch {
 std::vector<Message> messages;
 FrameError error{FrameError::none};
 [[nodiscard]] explicit operator bool() const noexcept { return error==FrameError::none; }
};
class MessageFramer {
 static constexpr std::size_t header_size=12;
 static constexpr std::size_t max_topic=128;
 static constexpr std::size_t max_payload=1024*1024;
 std::vector<std::uint8_t> pending_;
 std::size_t target_{header_size};
 bool poisoned_{false};
 static std::uint16_t u16(const std::vector<std::uint8_t>& s,std::size_t i) noexcept {
  return std::uint16_t(s[i]) | (std::uint16_t(s[i+1])<<8);
 }
 static std::uint32_t u32(const std::vector<std::uint8_t>& s,std::size_t i) noexcept {
  return std::uint32_t(s[i]) | (std::uint32_t(s[i+1])<<8) |
         (std::uint32_t(s[i+2])<<16) | (std::uint32_t(s[i+3])<<24);
 }
 FrameBatch reject() {
  poisoned_=true;
  pending_.clear();
  target_=header_size;
  return {{},FrameError::malformed};
 }
public:
 void reset() noexcept { pending_.clear(); target_=header_size; poisoned_=false; }
 [[nodiscard]] bool poisoned() const noexcept { return poisoned_; }
 [[nodiscard]] std::size_t buffered_bytes() const noexcept { return pending_.size(); }
 // An entire feed is accepted or rejected; malformed trailing frames discard
 // earlier decoded messages from the same feed, preventing partial dispatch.
 [[nodiscard]] FrameBatch feed(std::span<const std::uint8_t> bytes) {
  if(poisoned_)return {{},FrameError::poisoned};
  std::vector<Message> complete;
  while(!bytes.empty()) {
   const auto take=std::min(target_-pending_.size(),bytes.size());
   pending_.insert(pending_.end(),bytes.begin(),bytes.begin()+take);
   bytes=bytes.subspan(take);
   if(pending_.size()!=target_)continue;
   if(target_==header_size) {
    if(pending_[0]!='S'||pending_[1]!='P'||pending_[2]!='R'||pending_[3]!='L'||u16(pending_,4)!=1)
     return reject();
    const auto topic=u16(pending_,6);
    const auto payload=u32(pending_,8);
    if(topic==0||topic>max_topic||payload>max_payload)return reject();
    target_=16+topic+payload;
    continue;
   }
   auto decoded=MessageCodec::decode(pending_);
   if(!decoded)return reject();
   complete.push_back(std::move(*decoded.message));
   pending_.clear();
   target_=header_size;
  }
  return {std::move(complete),FrameError::none};
 }
};
}
