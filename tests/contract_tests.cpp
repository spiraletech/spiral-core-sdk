#include <spiral/core/contract.hpp>
#include <cassert>
#include <vector>
int main(){
 using namespace spiral::core;
 const Message m{1,"saelis.research.assessment",{0,255,0,42}};
 auto encoded=MessageCodec::encode(m);assert(encoded);
 auto good=MessageCodec::decode(*encoded);assert(good);
 assert(good.message->topic==m.topic&&good.message->payload==m.payload);
 assert(!MessageCodec::encode({2,"topic",{}}));
 assert(!MessageCodec::encode({1,"bad topic",{}}));
 assert(!MessageCodec::encode({1,"topic",std::vector<std::uint8_t>(1024*1024+1)}));
 auto bad=*encoded;bad[0]='X';assert(MessageCodec::decode(bad).error==DecodeError::bad_magic);
 bad=*encoded;bad[4]=2;assert(MessageCodec::decode(bad).error==DecodeError::unsupported_version);
 bad=*encoded;bad[12]=0;assert(MessageCodec::decode(bad).error==DecodeError::invalid_topic);
 bad=*encoded;bad[bad.size()-1]^=1;assert(MessageCodec::decode(bad).error==DecodeError::checksum_mismatch);
 bad=*encoded;bad.pop_back();assert(MessageCodec::decode(bad).error==DecodeError::truncated);
 bad=*encoded;bad.push_back(0);assert(MessageCodec::decode(bad).error==DecodeError::trailing_data);
 bad=*encoded;bad[8]=255;bad[9]=255;bad[10]=255;bad[11]=255;assert(MessageCodec::decode(bad).error==DecodeError::oversized);
}
