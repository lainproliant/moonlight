/*
 * moonlight/automata.h: Templates for finite-state and other automata.
 *
 * Author: Lain Supe (lainproliant)
 * Date: Tuesday, Jun 12 2018
 */
#pragma once

#include "moonlight/core.h"

#include <memory>
#include <vector>
#include <optional>
#include <future>
#include <mutex>
#include <shared_mutex>

namespace moonlight {
namespace automata {

template<class C> class State;

//-------------------------------------------------------------------
class Error : public core::Exception {
   using Exception::Exception;
};

//-------------------------------------------------------------------
template<class S>
class StackMachine {
public:
   friend class State<typename S::Context>;

   typedef std::shared_ptr<S> StatePointer;
   
   template<class T, class... TD>
   void init(typename T::Context& context_, TD... params) {
      auto initial_state = make<T>(*this, context_);
      push(initial_state);
      initial_state->init(std::forward<TD>(params)...); 
   }
   
   void run_until_complete() {
      while (auto state = current()) {
         state->run();
      }
   }

   std::future<void> run() {
      return std::async(std::launch::async, [&] {
         this->run_until_complete();
      });
   }

   void terminate() {
      std::unique_lock<std::shared_mutex> lock(mutex);
      stack.clear();
   }

protected:
   void push(StatePointer state) {
      std::unique_lock<std::shared_mutex> lock(mutex);
      stack.push_back(state);
   }
   
   void transition(StatePointer state) {
      std::unique_lock<std::shared_mutex> lock(mutex);
      pop();
      push(state);
   }
   
   void reset(StatePointer state) {
      std::unique_lock<std::shared_mutex> lock(mutex);
      terminate();
      push(state);
   }
   
   void pop() {
      std::unique_lock<std::shared_mutex> lock(mutex);
      if (stack.size() > 0) {
         stack.pop_back();
      }
   }

   StatePointer current() const {
      std::shared_lock<std::shared_mutex> lock(mutex);
      return stack.size() > 0 ? stack.back() : nullptr;
   }

   StatePointer previous(int n = 1) {
      std::shared_lock<std::shared_mutex> lock(mutex);
      auto result = splice::at(stack, -1 * n);
      return result ? *result : nullptr;
   }

   StatePointer parent(StatePointer child) {
      std::shared_lock<std::shared_mutex> lock(mutex);
      for (int n = 1; n < stack.size(); n++) {
         if (previous(n) == child) {
            return previous(n + 1);
         }
      }
      
      return nullptr;
   }

private:
   std::vector<StatePointer> stack;
   mutable std::shared_mutex mutex;
};

//-------------------------------------------------------------------
template<class C>
class State : public std::enable_shared_from_this<State<C>> {
public:
   friend class StackMachine<State<C>>;

   ALIAS_TYPEDEFS(State<C>)
   typedef C Context;
   typedef StackMachine<State<C>> Machine;

   State(Machine& machine_, C& context_)
   : machine_(machine_), context_(context_) { }

   virtual void run() = 0;

protected:
   void init() { }

   template<class T, class... TD>
   void push(TD&&... params) {
      auto state = make<T>(machine_, context_);
      machine_.push(state);
      state->init(std::forward<TD>(params)...);
   }
   
   template<class T, class... TD>
   void transition(TD&&... params) {
      auto state = make<T>(machine_, context_);
      machine_.transition(state);
      state->init(std::forward<TD>(params)...);
   }

   template<class T, class... TD>
   void reset(TD&&... params) {
      auto state = make<T>(machine_, context_);
      machine_.reset(state);
      state->init(std::forward<TD>(params)...);
   }
   
   void pop() {
      machine_.pop();
   }

   void terminate() {
      machine_.terminate();
   }
   
   C& context() {
      return context_;
   }

   Machine& machine() {
      return machine_;
   }
   
   bool is_current() {
      return current() ? current().get() == this : false;
   }

   pointer current() {
      return machine_.current();
   }

   pointer parent() {
      return machine_.parent(this->shared_from_this());
   }

private:
   C& context_;
   Machine& machine_;
};

//-------------------------------------------------------------------
template<class C>
class LambdaState : public State<C> {
public:
   typedef std::function<void(typename LambdaState::Machine&,
                              typename LambdaState::Context&)> LambdaImpl;
   typedef std::function<LambdaImpl ()> Factory;

   LambdaState(typename LambdaState::Machine& machine,
               typename LambdaState::Context& context,
               LambdaImpl impl) :
   State<C>(machine, context), impl_(impl) { }

   void run() override {
      impl_(this->machine(), this->context());
   }

private:
   LambdaImpl impl_;
};

//-------------------------------------------------------------------
template<class C>
class FactoryContext {
public:
   void declare(const std::string& state_name,
                      LambdaState<C>& state) {
      declare(state_name, [&]() { return state; });
   }

   void declare(const std::string& state_name,
                      typename LambdaState<C>::Factory& factory) {
      factory_map_.insert(state_name, factory);
   }

   void create(const std::string& state_name) {
      auto iter = factory_map_.find(state_name);
      if (iter != factory_map_.end()) {
         return *iter;
      } else {
         throw Error("Undefined state name: " + state_name);
      }
   }

   void transition(typename LambdaState<C>::Machine& machine,
                   const std::string& name) {
      machine.transition(make<LambdaState>(machine, *this, create(name)));
   }

   void reset(typename LambdaState<C>::Machine& machine,
              const std::string& name) {
      machine.reset(make<LambdaState>(machine, *this, create(name)));
   }

private:
   std::map<std::string, typename LambdaState<C>::Factory> factory_map_;
};

}
}

