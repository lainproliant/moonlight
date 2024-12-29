/*
 * ## moonlight/automata.h: Templates for finite-state automata. ----
 *
 * Author: Lain Supe (lainproliant)
 * Date: Tuesday, Jun 12 2018
 *
 * ## Usage: Class States -------------------------------------------
 * The main way to use `automata.h` is to define your state machine in terms of
 * classes, each class containing the implementation of a discrete state in your
 * state machine, and a `Context` type which is attached to your state machine
 * where your machine's side effects will be reflected.
 *
 * As an exmaple, if your state machine is for a video game, your `Context` type
 * might contain window handles, compiled shaders, textures, and other assets
 * that may be used in different states.  Data that is specific to a particular
 * state in the state machine should be part of that particular state object,
 * rather than the shared `Context`.
 *
 * First, define a `State` typedef in terms of your `Context` type.  This will
 * serve as the abstract base class for all of the states in your state machine.
 *
 * ```
 * typedef automata::State<Context> State;
 * ```
 *
 * Next, define your state classes.  Each state needs to possess the following
 * properties:
 *
 * - Is a `public` subclass of `State`.
 * - Contains an explicit constructor with any parameters needed to the state
 *   when it is instantiated, if necessary.
 * - Overrides the pure virtual function `void State::run()`, which will contain
 *   the implementation of the state's logic/behavior.  The state machine while
 *   running will run this method continuously
 *
 * This state machine model uses a stack internally, meaning you can push and
 * pop states, preserving previous states while transitioning with a push and
 * returning to those previous states later with a pop.  The current state is
 * always the state at the top of the stack.
 *
 * Within your `run()` method, the following methods are available for you to
 * interact with the state machine:
 *
 * - `context()`: Fetches a reference to the machine's `Context` instance.
 * - `machine()`: Fetches a reference to the machine itself.
 * - `push<T>(...)`: Push a new state `T` onto the machine's stack.
 * - `pop<T>()`: Pops the current state off of the state machine's stack.  If
 *   this was the only state on the stack, the machine is now terminated.
 * - `transition<T>(...)`: Replaces the current state with a new state `T`.
 * - `reset<T>(...)`: Removes all other states from the machine's stack,
 *   replacing them with a new `T` state.
 * - `is_current()`: Returns whether this state is the current state on top of
 *   the state machine.  This is always true, unless a parent state's `run()` is
 *   invoked via `call_parent()` or otherwise.
 * - `terminate()`: Removes all states, including this state, terminating the
 *   state machine.
 * - `current()`: Get a pointer to the current state, which should always be
 *   equal to `this` unless a parent state's `run()` is invoked via
 *   `call_parent()` or otherwise.
 * - `parent()`: Get a pointer to the previous state in the state machine's
 *   stack, or `nullptr` if there is no parent state.
 * - `call_parent()`: Invokes the `run()` method of the parent state.
 *   Throws a `core::RuntimeError` if there is no parent state.
 *
 * Now that we know how to define our states, let's kick things off by creating
 * our state machine with the `init<T>(context, ...)` method.  This method sets
 * up an initial state, associates a reference to the context object with our
 * machine, and passes any needed parameters to your initial state.  Assuming
 * you've defined an initial state called `InitialState`:
 *
 * ```
 * Context contex;
 * auto machine = State::Machine::init<InitialState>(context, 1);
 * ```
 *
 * Now that you've created the machine, you have a few options as to how it
 * should run:
 *
 * - Call the `run_until_complete()` method, which runs the state machine in the
 *   current thread and blocks until completion.
 * - Call the `run()` method, which starts the state machine in a new thread and
 *   returns an `std::future<void>` object which can be used later to wait for
 *   the state machine to finish.
 * - Intermittently call `update()`, which runs a single cycle of the state
 *   machine loop.  This method returns `true` if the machine has more cycles to
 *   run, or `false` if the machine has finished.
 *
 * ## Usage: Lambda States -------------------------------------------
 * `automata.h` also supports running a state machine where each state is
 * defined by a closure (i.e. "lambda") associated with a constant value.  This
 * constant value can be of any comparable type, e.g. it can be an integer
 * constant, an `std::string` (the default), or an `enum` type.
 *
 * Lambda state machine execution mostly works like the above, except
 * interaction with the machine is through a reference passed to the closure.
 * In order to prevent unintended side-effects, reference or pointer captures in
 * the closure are not recommended.
 *
 * The closure may accept one or two parameters:
 *
 * - `[](auto& m) {...}`: In this form, the parameter `m` is a reference to the
 *   lambda state machine.
 * - `[](auto& m, auto c) {...}`: In this form, the parameter `m` is a reference
 *   to the lambda state machine, and the parameter `c` is a pointer to the
 *   current state object.
 *
 * The lambda state machine provides the following methods:
 *
 * - `push(name)`: Push the named state to the top of the state machine stack.
 * - `pop()`: Pop the current state off of the state machine stack.
 * - `transition(name)`: Replaces the current state with the named state.
 * - `reset(name)`: Removes all other states from the machine's stack,
 *   replacing them with the named state.
 * - `current()`: Get a pointer to the current state.  Compare with `c` to
 *   determine if the currently running state is the machine's current state.
 *
 * Note there is no method on `m` to get the parent state.  To invoke the parent
 * state, define your closure in two-parameter form with `c` and call
 * `c->parent()->run()`.
 */
#ifndef __MOONLIGHT_AUTOMATA_H
#define __MOONLIGHT_AUTOMATA_H

#include <future>
#include <optional>
#include <map>
#include <string>
#include <utility>
#include <memory>
#include <vector>

#include "moonlight/exceptions.h"

namespace moonlight {
namespace automata {

template<class C, class K> class Lambda;

// ------------------------------------------------------------------
template<class S>
class StateMachine {
 public:
     typedef std::shared_ptr<S> StatePointer;

     enum class TraceEvent {
         PUSH,
         POP,
         TRANSITION,
         RESET,
         TERMINATE,
         FATAL_EXCEPTION
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

     static StateMachine<S> init_empty(typename S::Context& context) {
         return StateMachine<S>(context);
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
             THROW(core::UsageError, "StateMachine has no initial state.");
         }

         try {
             while (current()) {
                 current()->run();
             }

         } catch (...) {
#ifdef MOONLIGHT_AUTOMATA_DEBUG
             _trace(TraceEvent::TERMINATE);
             _trace(TraceEvent::FATAL_EXCEPTION);
#endif
             throw;
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
         while (current()) {
             _pop(false);
         }
     }

     void push(StatePointer state) {
#ifdef MOONLIGHT_AUTOMATA_DEBUG
         _trace(TraceEvent::PUSH, state);
#endif
         _push(state, true);
     }

     void transition(StatePointer state) {
#ifdef MOONLIGHT_AUTOMATA_DEBUG
         _trace(TraceEvent::TRANSITION, state);
#endif
         _pop(false);
         _push(state, false);
     }

     void reset(StatePointer state) {
#ifdef MOONLIGHT_AUTOMATA_DEBUG
         _trace(TraceEvent::RESET, state);
#endif
         while (current()) {
             _pop(false);
         }
         _push(state, false);
     }

     void pop() {
#ifdef MOONLIGHT_AUTOMATA_DEBUG
         _trace(TraceEvent::POP, stack.size() >= 2 ? stack[stack.size()-2] : nullptr);
#endif
         _pop(true);
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
         for (; iter != stack.rend() && *iter != pivot; iter++) { }
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
             {TraceEvent::TERMINATE, "TERMINATE"},
             {TraceEvent::FATAL_EXCEPTION, "FATAL_EXCEPTION"}
         };

         return names[event];
     }

     void _trace(TraceEvent event, StatePointer new_state = nullptr) {
         const std::string& event_name = _trace_event_name(event);
         for (auto tracer : tracers) {
             switch (event) {
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
             THROW(core::RuntimeError, "There are no more states on the stack.");
         }

         snapshot->pop_back();
         snapshot->back()->run();
     }

     void _push(StatePointer state, bool standby) {
         if (current() && standby) {
             current()->standby();
         }
         state->inject(this, &context());
         stack.push_back(state);
         state->init();
     }

     void _pop(bool resume) {
         try {
             current()->exit();
             stack.pop_back();

         } catch (...) {
             THROW(core::RuntimeError, "The stack is empty.");
         }

         if (current() && resume) {
             current()->resume();
         }
     }

 private:
     typename S::Context& context_;
     std::vector<StatePointer> stack;
     std::vector<Tracer> tracers;
     std::optional<std::vector<StatePointer>> snapshot = {};
};

// ------------------------------------------------------------------
template<class C>
class State : public std::enable_shared_from_this<State<C>> {
 public:
     typedef C Context;
     typedef StateMachine<State<C>> Machine;
     typedef std::shared_ptr<State<C>> Pointer;

     virtual ~State() { }

     virtual void run() = 0;

     virtual void init() { }
     virtual void standby() { }
     virtual void resume() { }
     virtual void exit() { }

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

// ------------------------------------------------------------------
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
                  THROW(core::UsageError, str::cat("Undefined state: ", name));
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
             THROW(core::FrameworkError, "Machine not injected into LambdaState.");
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
     K name_;
     std::optional<const Impl1> impl1;
     std::optional<const Impl2> impl2;
};

}  // namespace automata
}  // namespace moonlight

#endif /* !__MOONLIGHT_AUTOMATA_H */
