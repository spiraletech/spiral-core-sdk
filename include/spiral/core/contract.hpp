#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
namespace spiral::core {
struct Message { std::uint16_t version{1}; std::string topic; std::vector<std::uint8_t> payload; };
enum class DecodeError { none, truncated, bad_magic, unsupported_version, invalid_topic, oversized, checksum_mismatch, trailing_data };
struct DecodeResult { std::optional<Message> message; DecodeError error{DecodeError::none}; explicit operator bool() const noexcept {return message.has_value();} };
class MessageCodec {
 static constexpr std::uint32_t max_payload=1024*1024;
 static constexpr std::uint16_t max_topic=128;
 static void put16(std::vector<std::uint8_t>& v,std::uint16_t x){v.push_back(static_cast<std::uint8_t>(x));v.push_back(static_cast<std::uint8_t>(x>>8));}
 static void put32(std::vector<std::uint8_t>& v,std::uint32_t x){for(unsigned i=0;i<4;++i)v.push_back(static_cast<std::uint8_t>(x>>(8*i)));}
 static std::uint32_t hash(std::span<const std::uint8_t> v){std::uint32_t h=2166136261u;for(auto b:v){h^=b;h*=16777619u;}return h;}
 static std::uint16_t read16(std::span<const std::uint8_t> s,std::size_t p){return std::uint16_t(s[p])|(std::uint16_t(s[p+1])<<8);}
 static std::uint32_t read32(std::span<const std::uint8_t> s,std::size_t p){std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(s[p+i])<<(8*i);return v;}
 static bool topic_valid(std::string_view s){
  if(s.empty()||s.size()>max_topic)return false;
  for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'))return false;
  return true;
 }
public:
 static std::optional<std::vector<std::uint8_t>> encode(const Message& m){
  if(m.version!=1||!topic_valid(m.topic)||m.payload.size()>max_payload)return std::nullopt;
  std::vector<std::uint8_t> out;out.reserve(16+m.topic.size()+m.payload.size());
  out.insert(out.end(),{'S','P','R','L'});put16(out,m.version);put16(out,static_cast<std::uint16_t>(m.topic.size()));put32(out,static_cast<std::uint32_t>(m.payload.size()));
  out.insert(out.end(),m.topic.begin(),m.topic.end());out.insert(out.end(),m.payload.begin(),m.payload.end());put32(out,hash(out));return out;
 }
 static DecodeResult decode(std::span<const std::uint8_t> s){
  if(s.size()<16)return {{},DecodeError::truncated};
  if(!(s[0]=='S'&&s[1]=='P'&&s[2]=='R'&&s[3]=='L'))return {{},DecodeError::bad_magic};
  if(read16(s,4)!=1)return {{},DecodeError::unsupported_version};
  const auto n=read16(s,6);const auto m=read32(s,8);
  if(n==0||n>max_topic||m>max_payload)return {{},DecodeError::oversized};
  const auto wanted=std::size_t(16)+n+m;
  if(s.size()<wanted)return {{},DecodeError::truncated};
  if(s.size()>wanted)return {{},DecodeError::trailing_data};
  const std::string topic(s.begin()+12,s.begin()+12+n);
  if(!topic_valid(topic))return {{},DecodeError::invalid_topic};
  if(hash(s.first(wanted-4))!=read32(s,wanted-4))return {{},DecodeError::checksum_mismatch};
  Message out;out.version=1;out.topic=topic;out.payload.assign(s.begin()+12+n,s.begin()+12+n+m);
  return {std::move(out),DecodeError::none};
 }
};
}
