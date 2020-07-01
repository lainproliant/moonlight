/*
 * moonlight/automata.h: Templates for finite-state automata.
 *
 * Author: Lain Supe (lainproliant)
 * Date: Tuesday, Jun 12 2018
 */
#ifndef __MOONLIGHT_AUTOMATA_H
#define __MOONLIGHT_AUTOMATA_H

#include "moonlight/exceptions.h"

#include <thread>
#include <future>
#include <map>

namespace moonlight {
namespace automata {

template<class C, class K> class Lambda;

//-------------------------------------------------------------------
class Error : public core::Exception {
   using Exception::Exception;
};

//-------------------------------------------------------------------
template<class S>
class StateMachine {
public:
   typedef std::shared_ptr<S> StatePointer;

   enum class TraceEvent {
      PUSH,
      POP,
      TRANSITION,
      RESET,
      TERMINATE
   };

   typedef std::function<void(TraceEvent event,
                              typename S::Context& context,
                              const std::string& event_name, const std::vector<StatePointer>& stack,
                              StatePointer prev_state, StatePointer new_state)> Tracer;

   void add_tracer(Tracer tracer) {
      tracers.push_back(tracer);
   }

   template<class T, class... TD>
   static StateMachine<S> init(typename S::Context& context, TD... params) {
      StateMachine<S> machine(context);
      auto state = std::make_shared<T>(std::forward<TD>(params)...);
      machine.push(state);
      return machine;
   }

   static StateMachine<S> init_empty() {
      return StateMachine<S>();
   }

   bool update() {
      if (current()) {
         current()->run();
         return true;
      } else {
         return false;
      }
   }

   void run_until_complete() {
      if (! current()) {
         throw Error("StateMachine has no initial state.");
      }

      while (current()) {
         current()->run();
      }
   }

   std::future<void> run() {
      return std::async(std::launch::async, [&] {
         this->run_until_complete();
      });
   }

   void terminate() {
#ifdef MOONLIGHT_AUTOMATA_DEBUG
      _trace(TraceEvent::TERMINATE);
#endif
      stack.clear();
   }

   void push(StatePointer state) {
#ifdef MOONLIGHT_AUTOMATA_DEBUG
      _trace(TraceEvent::PUSH, state);
#endif
      state->inject(this, &context());
      stack.push_back(state);
   }

   void transition(StatePointer state) {
#ifdef MOONLIGHT_AUTOMATA_DEBUG
      _trace(TraceEvent::TRANSITION, state);
#endif
      state->inject(this, &context());
      stack.pop_back();
      stack.push_back(state);
   }

   void reset(StatePointer state) {
#ifdef MOONLIGHT_AUTOMATA_DEBUG
      _trace(TraceEvent::RESET, state);
#endif
      state->inject(this, &context());
      stack.clear();
      stack.push_back(state);
   }

   void pop() {
#ifdef MOONLIGHT_AUTOMATA_DEBUG
      _trace(TraceEvent::POP, stack.size() >= 2 ? stack[stack.size()-2] : nullptr);
#endif
      try {
         stack.pop_back();
      } catch (...) {
         throw Error("The stack is empty.");
      }
   }

   StatePointer current() const {
      return current_state();
   }

   StatePointer previous() const {
      return stack.size() > 1 ? stack[stack.size()-2] : nullptr;
   }

   StatePointer current_state() const {
      return stack.size() > 0 ? stack.back() : nullptr;
   }

   StatePointer parent(StatePointer pivot) const {
      auto iter = stack.rbegin();
      for (; iter != stack.rend() && *iter != pivot; iter++);
      if (iter == stack.rend() || std::next(iter) == stack.rend()) {
         return nullptr;
      } else {
         return *std::next(iter);
      }
   }

   void call_parent() {
      if (! snapshot) {
         core::Finalizer finally([&]() {
            snapshot = {};
         });

         snapshot = stack;
         _call_parent_impl();

      } else {
         _call_parent_impl();
      }
   }

   std::vector<StatePointer> stack_trace() const {
      return stack;
   }

   typename S::Context& context() {
      return context_;
   }

   const typename S::Context& context() const {
      return context_;
   }

protected:
   StateMachine(typename S::Context& context) :
   context_(context) { }

#ifdef MOONLIGHT_AUTOMATA_DEBUG
   static const std::string& _trace_event_name(TraceEvent event) {
      static std::map<TraceEvent, std::string> names = {
         {TraceEvent::PUSH, "PUSH"},
         {TraceEvent::POP, "POP"},
         {TraceEvent::TRANSITION, "TRANSITION"},
         {TraceEvent::RESET, "RESET"},
         {TraceEvent::TERMINATE, "TERMINATE"}
      };

      return names[event];
   }

   void _trace(TraceEvent event, StatePointer new_state = nullptr) {
      const std::string& event_name = _trace_event_name(event);
      for (auto tracer : tracers) {
         switch(event) {
         case TraceEvent::PUSH:
         case TraceEvent::TRANSITION:
         case TraceEvent::RESET:
            tracer(event, context_, event_name, stack, current(), new_state);
            break;
         case TraceEvent::POP:
            tracer(event, context_, event_name, stack, current(), previous());
            break;
         case TraceEvent::TERMINATE:
            tracer(event, context_, event_name, stack, current(), nullptr);
            break;
         }
      }
   }
#endif

   void _call_parent_impl() {
      if (snapshot->size() <= 1) {
         throw Error("There are no more states on the stack.");
      }

      snapshot->pop_back();
      snapshot->back()->run();
   }

private:
   typename S::Context& context_;
   std::vector<StatePointer> stack;
   std::vector<Tracer> tracers;
   std::optional<std::vector<StatePointer>> snapshot = {};
};

//-------------------------------------------------------------------
template<class C>
class State : public std::enable_shared_from_this<State<C>> {
public:
   typedef C Context;
   typedef StateMachine<State<C>> Machine;
   typedef std::shared_ptr<State<C>> Pointer;

   virtual ~State() { }

   virtual void run() = 0;
   void inject(StateMachine<State<C>>* machine, C* context) {
      this->machine_ = machine;
      this->context_ = context;
   }

   void terminate() {
      machine().terminate();
   }

   C& context() {
      return *context_;
   }

   const C& context() const {
      return *context_;
   }

   StateMachine<State<C>>& machine() {
      return *machine_;
   }

   const StateMachine<State<C>>& machine() const {
      return *machine_;
   }

   virtual const char* tracer_name() const {
      return "???";
   }

protected:
   C* context_;
   StateMachine<State<C>>* machine_;

   template<class T, class... TD>
   void push(TD... params) {
      auto state = std::make_shared<T>(std::forward<TD>(params)...);
      machine().push(state);
   }

   template<class T, class... TD>
   void transition(TD... params) {
      auto state = std::make_shared<T>(std::forward<TD>(params)...);
      machine().transition(state);
   }

   template<class T, class... TD>
   void reset(TD... params) {
      auto state = std::make_shared<T>(std::forward<TD>(params)...);
      machine().reset(state);
   }

   void pop() {
      machine().pop();
   }

   bool is_current() {
      return current() ? current().get() == this : false;
   }

   Pointer current() {
      return machine().current();
   }

   Pointer parent() {
      return machine().parent(this->shared_from_this());
   }

   void call_parent() {
      return machine().call_parent();
   }
};

//-------------------------------------------------------------------
template<class C, class K = std::string>
class Lambda : public State<C> {
public:
   class Builder;
   class Machine;
   typedef std::shared_ptr<Lambda<C, K>> Pointer;
   typedef std::function<void(Lambda<C, K>::Machine&)> Impl1;
   typedef std::function<void(Lambda<C, K>::Machine&, Pointer)> Impl2;

   using State<C>::shared_from_this;

   class Machine : public StateMachine<State<C>> {
   public:
      friend class Builder;
      using StateMachine<State<C>>::push;
      using StateMachine<State<C>>::transition;
      using StateMachine<State<C>>::reset;

      Machine(C& context) :
      StateMachine<State<C>>(context) { }

      template<class T, class... TD>
      void push_state(TD... params) {
         auto state = std::make_shared<T>(std::forward<TD>(params)...);
         push(state);
      }

      template<class T, class... TD>
      void transition_state(TD... params) {
         auto state = std::make_shared<T>(std::forward<TD>(params)...);
         transition(state);
      }

      template<class T, class... TD>
      void reset_state(TD... params) {
         auto state = std::make_shared<T>(std::forward<TD>(params)...);
         reset(state);
      }

      void push(const K& name) {
         push(state(name));
      }

      void transition(const K& name) {
         transition(state(name));
      }

      void reset(const K& name) {
         reset(state(name));
      }

      Pointer state(const K& name) {
         auto iter = state_map.find(name);
         if (iter != state_map.end()) {
            return iter->second;
         } else {
            throw Error(str::cat("Unknown state: ", name));
         }
      }

      Pointer current() {
         return std::static_pointer_cast<Lambda<C, K>>(this->current_state());
      }

      Machine& def_state(const K& name,
                         const typename Lambda<C, K>::Impl1& impl) {
         auto state = std::make_shared<Lambda<C, K>>(name, impl);
         state_map.insert({name, state});
         return *this;
      }

      Machine& def_state(const K& name,
                         const typename Lambda<C, K>::Impl2& impl) {
         auto state = std::make_shared<Lambda<C, K>>(name, impl);
         state_map.insert({name, state});
         return *this;
      }

      std::vector<K> stack_trace() {
         auto stacktrace = this->StateMachine<State<C>>::stack_trace();
         std::vector<K> trace;
         for (auto iter = stacktrace.rbegin();
              iter != stacktrace.rend();
              iter++) {
            trace.push_back(std::static_pointer_cast<Lambda<C, K>>(*iter)->name());
         }

         return trace;
      }

   private:
      std::map<K, typename Lambda<C, K>::Pointer> state_map;
   };

   class Builder {
   public:
      friend class Lambda<C, K>;

      Builder(C& context)
      : context(context) { }

      Machine build() {
         Machine machine = Machine(context);
         for (auto iter : state_map_1) {
            machine.def_state(iter.first, iter.second);
         }
         for (auto iter : state_map_2) {
            machine.def_state(iter.first, iter.second);
         }

         if (init_state_) {
            machine.push(machine.state(*init_state_));
         }

         return machine;
      }

      Builder& init(const K& name) {
         init_state_ = name;
         return *this;
      }

      Builder& state(const K& name,
                     const typename Lambda<C, K>::Impl1& impl) {
         state_map_1.insert({name, impl});
         return *this;
      }

      Builder& state(const K& name,
                     const typename Lambda<C, K>::Impl2& impl) {
         state_map_2.insert({name, impl});
         return *this;
      }

   private:
      C& context;
      std::optional<K> init_state_;
      std::map<K, typename Lambda<C, K>::Impl1> state_map_1;
      std::map<K, typename Lambda<C, K>::Impl2> state_map_2;
   };

   Lambda(K name, const Impl1& impl)
   : name_(name), impl1(impl) { }

   Lambda(K name, const Impl2& impl)
   : name_(name), impl2(impl) { }

   static Builder builder(C& context) {
      return Builder(context);
   }

   void run() override {
      if (this->machine_ == nullptr) {
         throw Error("Machine not injected into LambdaState. "
                     "This is a bug in the state machine code.");
      }
      if (impl1) {
         impl1.value()(*static_cast<Machine*>(this->machine_));
      } else {
         impl2.value()(*static_cast<Machine*>(this->machine_),
                       std::static_pointer_cast<Lambda<C, K>>(shared_from_this()));
      }
   }

   const K& name() const {
      return name_;
   }

private:
   std::optional<const Impl1> impl1;
   std::optional<const Impl2> impl2;
   K name_;
};

}
}

#endif /* !__MOONLIGHT_AUTOMATA_H */
