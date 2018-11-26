/*
 * moonlight/automata.h: Templates for finite-state automata.
 *
 * Author: Lain Supe (lainproliant)
 * Date: Tuesday, Jun 12 2018
 */
#pragma once

#include "moonlight/core.h"

#include <vector>
#include <optional>
#include <future>

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

   template<class T, class... TD>
   static StateMachine<S> init(typename S::Context& context, TD... params) {
      StateMachine<S> machine;
      auto state = make<T>(machine, context);
      machine.push(state);
      state->init(std::forward<TD>(params)...);
      return machine;
   }

   static StateMachine<S> init_empty() {
      return StateMachine<S>();
   }

   void run_until_complete() {
      if (! current()) {
         throw Error("StateMachine has no initial state.");
      }

      StatePointer state;
      while ((state = current())) {
         state->run();
      }
   }

   std::future<void> run() {
      return std::async(std::launch::async, [&] {
         this->run_until_complete();
      });
   }

   void terminate() {
      stack.clear();
   }

   void push(StatePointer state) {
      stack.push_back(state);
   }

   void transition(StatePointer state) {
      pop();
      push(state);
   }

   void reset(StatePointer state) {
      terminate();
      push(state);
   }

   void pop() {
      if (stack.size() > 0) {
         stack.pop_back();
      }
   }

   StatePointer current() const {
      return current_state();
   }

   StatePointer current_state() const {
      return stack.size() > 0 ? stack.back() : nullptr;
   }

   void parent() {
      if (! snapshot) {
         core::Finalizer finally([&]() {
            snapshot = {};
         });

         snapshot = stack;
         _parent_impl();

      } else {
         _parent_impl();
      }
   }


protected:
   StateMachine() { }

   void _parent_impl() {
      if (snapshot->size() <= 1) {
         throw Error("There are no more states on the stack.");
      }

      snapshot->pop_back();
      snapshot->back()->run();
   }

private:
   std::vector<StatePointer> stack;
   std::optional<std::vector<StatePointer>> snapshot = {};
};

//-------------------------------------------------------------------
template<class C>
class State : public std::enable_shared_from_this<State<C>> {
public:
   typedef C Context;
   typedef StateMachine<State<C>> Machine;
   typedef std::shared_ptr<State<C>> Pointer;

   State(StateMachine<State<C>>& machine, C& context)
   : machine_(machine), context_(context) { }
   virtual ~State() { }

   virtual void run() = 0;
   void init() { }

   void terminate() {
      machine().terminate();
   }

protected:
   StateMachine<State<C>>& machine() {
      return machine_;
   }

   C& context() {
      return context_;
   }

   template<class T, class... TD>
   void push(TD... params) {
      auto state = derive<T>();
      machine().push(state);
      state->init(std::forward<TD>(params)...);
   }

   template<class T, class... TD>
   void transition(TD... params) {
      auto state = derive<T>();
      machine().transition(state);
      state->init(std::forward<TD>(params)...);
   }

   template<class T, class... TD>
   void reset(TD... params) {
      auto state = derive<T>();
      machine().reset(state);
      state->init(std::forward<TD>(params)...);
   }

   template<class T, class... TD>
   std::shared_ptr<T> derive(TD... params) {
      return make<T>(machine_, context_, std::forward<TD>(params)...);
   }

   void pop() {
      machine().pop();
   }

   bool is_current() {
      return current() ? current().get() == this : false;
   }

   Pointer current() {
      return machine_.current();
   }

   void parent() {
      return machine_.parent();
   }

private:
   StateMachine<State<C>>& machine_;
   C& context_;
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

      void push(const K& name) {
         push(state(name));
      }

      void transition(const K& name) {
         push(state(name));
      }

      void reset(const K& name) {
         push(state(name));
      }

      Pointer state(const K& name) {
         auto iter = state_map.find(name);
         if (iter != state_map.end()) {
            return iter->second;
         } else {
            throw Error(str::cat("Unknown state: ", name));
         }
      }

      C& context() {
         return context_;
      }

      Pointer current() {
         return std::static_pointer_cast<Lambda<C, K>>(this->current_state());
      }

      Machine& def_state(const K& name,
                         const typename Lambda<C, K>::Impl1& impl) {
         state_map.insert({name, make<Lambda<C, K>>(*this, context(), name, impl)});
         return *this;
      }

      Machine& def_state(const K& name,
                         const typename Lambda<C, K>::Impl2& impl) {
         state_map.insert({name, make<Lambda<C, K>>(*this, context(), name, impl)});
         return *this;
      }

   protected:
      Machine(const C& context)
      : context_(context) { }

   private:
      C context_;
      std::map<K, typename Lambda<C, K>::Pointer> state_map;
   };

   class Builder {
   public:
      Machine build() {
         if (! context_) {
            throw Error("Context must be provided via Builder::context()");
         }

         if (! init_state_) {
            throw Error("Initial state must be provided via Builder::init()");
         }

         Machine machine = Machine(*context_);
         for (auto iter : state_map_1) {
            machine.def_state(iter.first, iter.second);
         }
         for (auto iter : state_map_2) {
            machine.def_state(iter.first, iter.second);
         }

         machine.push(machine.state(*init_state_));
         return machine;
      }

      Builder& context(const C& context) {
         context_ = context;
         return *this;
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
      std::optional<C> context_;
      std::optional<K> init_state_;
      std::map<K, typename Lambda<C, K>::Impl1> state_map_1;
      std::map<K, typename Lambda<C, K>::Impl2> state_map_2;
   };

   Lambda(Machine& machine, C& context,
               K name, const Impl1& impl)
   : State<C>(machine, context), machine_(machine), name_(name), impl1(impl) { }

   Lambda(Machine& machine, C& context,
               K name, const Impl2& impl)
   : State<C>(machine, context), machine_(machine), name_(name), impl2(impl) { }

   static Builder builder() {
      return Builder();
   }

   void run() override {
      if (impl1) {
         impl1.value()(machine_);
      } else {
         impl2.value()(machine_, std::static_pointer_cast<Lambda<C, K>>(
                 shared_from_this()));
      }
   }

   const K& name() const {
      return name_;
   }

private:
   Machine& machine_;
   std::optional<const Impl1> impl1;
   std::optional<const Impl2> impl2;
   K name_;
};

}
}

