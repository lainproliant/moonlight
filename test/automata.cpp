#include <iostream>
#include <csignal>
#include "moonlight/automata.h"
#include "moonlight/test.h"

using namespace moonlight;
using namespace moonlight::test;

int main() {
    return TestSuite("moonlight automata tests")
    .test("stack machine test", [&]() {
        typedef automata::State<std::vector<int>> State;

        class CountingState : public State {
         public:
             explicit CountingState(int n, int x = 0) {
                 this->n = n;
                 this->x = x;
             }

         private:
             void run() override {
                 context().push_back(x += n);
                 if (x >= 100000000) {
                     terminate();

                 } else if (x >= (2*n)) {
                     transition<CountingState>(n * 2, x);
                 }
             }

         private:
             int n;
             int x;
        };

        std::vector<int> context;
        auto machine = State::Machine::init<CountingState>(context, 1);
        auto future = machine.run();
        future.wait();

        for (int x : context) {
            std::cout << x << " ";
        }
        std::cout << std::endl;

        ASSERT_EQUAL(context.size(), (size_t) 28);
    })
    .test("stack machine parent state test", [&]() {
        struct Context {
            int x = 0, y = 0;
        };

        typedef automata::State<Context> State;
        class YState;

        class XState : public State {
         public:
             using State::State;

         private:
             void run() override {
                 context().x ++;

                 if (is_current() && context().x >= 10) {
                     push<YState>();
                 }
             }
        };

        class YState : public State {
         public:
             using State::State;

             void run() override {
                 call_parent();
                 context().y ++;

                 if (context().y >= 10) {
                     terminate();
                 }
             }
        };

        Context context;
        auto machine = State::Machine::init<XState>(context);
        machine.run_until_complete();
        std::cout << "x = " << context.x << std::endl;
        std::cout << "y = " << context.y << std::endl;
        ASSERT_EQUAL(context.x, 20);
        ASSERT_EQUAL(context.y, 10);
    })
    .test("lambda state machine", [] {
        struct Context {
            int x = 0; int y = 0;
        };

        Context context;

        auto machine = automata::Lambda<Context>::builder(context)
        .init("init")
        .state("init", [&] (auto& m) {
            std::cout << "init state" << std::endl;
            std::cout << "trace: " << str::join(m.stack_trace(), ",") << std::endl;
            std::cout << "m.context().x: " << m.context().x << std::endl;
            std::cout << "m.context().y: " << m.context().y << std::endl;
            m.context().x++;

            if (m.current()->name() == "init") {
                ASSERT_EQUAL(m.stack_trace(), {"init"});
                std::cout << "init state is current" << std::endl;
                m.push("a");
            }
        })
        .state("a", [] (auto& m, auto c) {
            std::cout << "a state" << std::endl;
            std::cout << "trace: " << str::join(m.stack_trace(), ",") << std::endl;
            std::cout << "m.context().x: " << m.context().x << std::endl;
            std::cout << "m.context().y: " << m.context().y << std::endl;
            m.call_parent();
            m.context().y++;

            if (m.current() == c) {
                ASSERT_EQUAL(m.stack_trace(), {"a", "init"});
                std::cout << "a state is current" << std::endl;
                m.transition("b");
            }
        })
        .state("b", [] (auto& m) {
            std::cout << "b state" << std::endl;
            std::cout << "trace: " << str::join(m.stack_trace(), ",") << std::endl;
            std::cout << "m.context().x: " << m.context().x << std::endl;
            std::cout << "m.context().y: " << m.context().y << std::endl;
            ASSERT_EQUAL(m.stack_trace(), {"b", "init"});
            m.call_parent();
            m.context().x++;
            m.terminate();
        })
        .build();

        machine.run_until_complete();
        machine.push("init");
        machine.run_until_complete();

        try {
            machine.push("a");
            machine.run_until_complete();
            FAIL("Expected RuntimeError was not thrown.");

        } catch (const core::RuntimeError& e) {
            // This is supposed to happen, because "a" tries to call
            // call_parent(), but in this case there is no parent to "a".
            std::cout << "Caught expected RuntimeError." << std::endl;

        } catch (...) {
            FAIL("An unexpected error was thrown.");
        }

        std::cout << "machine.context().x = " << machine.context().x << std::endl;
        std::cout << "machine.context().y = " << machine.context().y << std::endl;
        ASSERT_EQUAL(machine.context().x, (int) 8);
        ASSERT_EQUAL(machine.context().y, (int) 2);
    })
    .test("mixed lambdas and OOP states", []() {
        struct Context {
            int x = 0, y = 0;
        };

        class MixedState : public automata::State<Context> {
         public:
             using State::State;

         private:
             void run() {
                 context().y ++;
                 machine().terminate();
             }
        };

        Context context;
        auto machine = automata::Lambda<Context>::builder(context)
        .build();

        machine.def_state("init", [](auto& m) {
            m.context().x ++;
            m.template push_state<MixedState>();
        });

        machine.push("init");
        machine.run_until_complete();

        ASSERT_EQUAL(machine.context().x, (int) 1);
        ASSERT_EQUAL(machine.context().y, (int) 1);
    })
    .test("State init and cleanup functions called appropriately.", []() {
        struct Context {
            int run_called = 0;
            int cleanup_called = 0;
        };

        typedef automata::State<Context> State;

        class SampleState : public State {
         public:
             using State::State;

             SampleState() {}

             ~SampleState() {
                 context().cleanup_called ++;
             }

         private:
             void run() override {
                 if (context().run_called == 0) {
                     context().run_called ++;
                     push<SampleState>();

                 } else {
                     context().run_called ++;
                     pop();
                 }
             }
        };

        Context context;
        auto machine = State::Machine::init<SampleState>(context);
        machine.run_until_complete();

        std::cout << "context.run_called = " << context.run_called << std::endl;
        std::cout << "context.cleanup_called = " << context.cleanup_called << std::endl;

        ASSERT_EQUAL(context.run_called, 3);
        ASSERT_EQUAL(context.cleanup_called, 2);
    })
    .test("Ability to refer to parent states as objects.", []() {
        struct Context {
            std::vector<std::string> names;
        };

        class State : public automata::State<Context> {
         public:
             explicit State(const std::string& name) : _name(name) { }

             const std::string& name() const {
                 return _name;
             }

             virtual void push_name() {
                 context().names.push_back(name());
             }

             virtual void delegate() { }

         private:
             std::string _name;
        };

        class StateB;
        class StateC;
        class StateA : public State {
         public:
             StateA() : State("A") { }
             void run() {
                 push<StateB>();
             }
             void delegate() {
                 push_name();
             }
        };
        class StateB : public State {
         public:
             StateB() : State("B") { }
             void run() {
                 push<StateC>();
             }
             void delegate() {
                 push_name();
                 std::static_pointer_cast<State>(parent())->delegate();
             }
        };
        class StateC : public State {
         public:
             StateC() : State("C") { }

             void run() {
                 push_name();
                 std::static_pointer_cast<State>(parent())->delegate();
                 terminate();
             }
        };

        Context context;
        auto machine = State::Machine::init<StateA>(context);
        machine.run_until_complete();

        ASSERT_EQUAL(context.names, {"C", "B", "A"});
    })
    .test("Enum based key for Lambda states.", []() {
        enum AlphaState {
            A,
            B,
            C
        };

        int ctx = 0;

        auto machine = automata::Lambda<int, AlphaState>::builder(ctx)
        .init(AlphaState::A)
        .state(AlphaState::A, [](auto& m) {
            m.context() += 1;
            m.transition(AlphaState::B);
        })
        .state(AlphaState::B, [](auto& m) {
            m.context() += 2;
            m.transition(AlphaState::C);
        })
        .state(AlphaState::C, [](auto& m) {
            m.context() += 3;
            m.pop();
        })
        .build();

        machine.run_until_complete();

        ASSERT_EQUAL(ctx, 6);
    })
    .run();
}
