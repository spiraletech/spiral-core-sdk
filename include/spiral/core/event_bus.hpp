#pragma once
#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <mutex>
#include <typeindex>
#include <utility>
#include <vector>
namespace spiral::core {
class EventBus {
 using Callback = std::function<void(const void*)>;
 struct Slot { std::size_t id; Callback callback; };
 std::mutex mutex_;
 std::map<std::type_index, std::vector<Slot>> slots_;
 std::size_t next_id_{1};
 public:
 template<class Event, class Handler> std::size_t subscribe(Handler&& handler) {
  std::lock_guard lock(mutex_);
  const auto id=next_id_++;
  slots_[typeid(Event)].push_back({id,[fn=std::forward<Handler>(handler)](const void* p){fn(*static_cast<const Event*>(p));}});
  return id;
 }
 template<class Event> void unsubscribe(std::size_t id) {
  std::lock_guard lock(mutex_);
  auto it=slots_.find(typeid(Event));
  if(it==slots_.end()) return;
  auto& vec=it->second;
  std::erase_if(vec,[id](const Slot& s){return s.id==id;});
 }
 template<class Event> void publish(const Event& event) {
  std::vector<Slot> snapshot;
  { std::lock_guard lock(mutex_); auto it=slots_.find(typeid(Event)); if(it!=slots_.end()) snapshot=it->second; }
  for(const auto& slot:snapshot) slot.callback(&event);
 }
};
}
