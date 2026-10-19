#pragma once
#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <typeindex>
#include <utility>
#include <vector>
namespace spiral::core {
class EventBus {
 using Callback = std::function<void(const void*)>;
 struct Slot { std::size_t id; std::shared_ptr<Callback> callback; };
 std::mutex mutex_;
 std::map<std::type_index, std::vector<Slot>> slots_;
 std::size_t next_id_{1};
 public:
 template<class Event, class Handler> std::size_t subscribe(Handler&& handler) {
  // User-defined handler construction can throw or reenter the bus.
  // Prepare it before acquiring the subscription mutex.
  Callback callback{[fn=std::forward<Handler>(handler)](const void* p) mutable {fn(*static_cast<const Event*>(p));}};
  // Stable ownership: snapshotting must never copy a user-defined handler
  // while the bus mutex is held, or reset mutable handler state.
  Slot pending{0,std::make_shared<Callback>(std::move(callback))};
  std::lock_guard lock(mutex_);
  const auto id=next_id_;
  pending.id=id;
  slots_[typeid(Event)].push_back(std::move(pending));
  // A failed map/vector allocation must not consume a subscription ID.
  ++next_id_;
  return id;
 }
 template<class Event> void unsubscribe(std::size_t id) {
  // Destruction of a user callback may reenter this bus. Retain its last
  // owner until after the subscription mutex has been released.
  std::shared_ptr<Callback> retired;
  {
   std::lock_guard lock(mutex_);
   auto it=slots_.find(typeid(Event));
   if(it==slots_.end()) return;
   auto& vec=it->second;
   auto slot=std::find_if(vec.begin(),vec.end(),[id](const Slot& s){return s.id==id;});
   if(slot==vec.end()) return;
   retired=std::move(slot->callback);
   vec.erase(slot);
  }
 }
 template<class Event> void publish(const Event& event) {
  std::vector<Slot> snapshot;
  { std::lock_guard lock(mutex_); auto it=slots_.find(typeid(Event)); if(it!=slots_.end()) snapshot=it->second; }
  for(const auto& slot:snapshot) (*slot.callback)(&event);
 }
};
}
