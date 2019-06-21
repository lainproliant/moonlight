#include "moonlight/automata.h"
#include "moonlight/test.h"
#include <iostream>
#include <csignal>

using namespace moonlight;
using namespace moonlight::test;

int main() {
   return TestSuite("moonlight automata tests")
   .test("stack machine test", [&]() {
      typedef automata::State<std::vector<int>> State;

      class CountingState : public State {
      public:
         using State::State;

         void init(int n, int x = 0) {
            this->n = n;
            this->x = x;
         }

         virtual void run() override {
            context.push_back(x += n);
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

      assert_equal(context.size(), (size_t) 28);
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

         virtual void run() override {
            context.x ++;

            if (is_current() && context.x >= 10) {
               push<YState>();
            }
         }
      };

      class YState : public State {
      public:
         using State::State;

         virtual void run() override {
            parent();
            context.y ++;

            if (context.y >= 10) {
               terminate();
            }
         }
      };

      Context context;
      auto machine = State::Machine::init<XState>(context);
      machine.run_until_complete();
      std::cout << "x = " << context.x << std::endl;
      std::cout << "y = " << context.y << std::endl;
      assert_equal(context.x, 20);
      assert_equal(context.y, 10);
   })
   .test("lambda state machine", [] {
      struct Context {
         int x = 0; int y = 0;
      };

      auto machine = automata::Lambda<Context>::builder()
      .init("init")
      .context(Context())
      .state("init", [&] (auto& m) {
         std::cout << "init state" << std::endl;
         std::cout << "trace: " << str::join(m.stack_trace(), ",") << std::endl;
         m.context.x++;

         if (m.current()->name() == "init") {
            assert_true(lists_equal(m.stack_trace(), {"init"}));
            std::cout << "init state is current" << std::endl;
            m.push("a");
         }
      })
      .state("a", [] (auto& m, auto c) {
         std::cout << "a state" << std::endl;
         std::cout << "trace: " << str::join(m.stack_trace(), ",") << std::endl;
         m.parent();
         m.context.y++;

         if (m.current() == c) {
            assert_true(lists_equal(m.stack_trace(), {"a", "init"}));
            std::cout << "a state is current" << std::endl;
            m.transition("b");
         }
      })
      .state("b", [] (auto& m) {
         std::cout << "b state" << std::endl;
         std::cout << "trace: " << str::join(m.stack_trace(), ",") << std::endl;
         assert_true(lists_equal(m.stack_trace(), {"b", "init"}));
         m.parent();
         m.context.x++;
         m.terminate();
      })
      .build();

      machine.run_until_complete();
      machine.push("init");
      machine.run_until_complete();

      try {
         machine.push("a");
         machine.run_until_complete();

      } catch (const automata::Error& e) {
         // This is supposed to happen, because "a" tries to call
         // parent(), but in this case there is no parent to "a".
         std::cout << "Caught expected automata::Error." << std::endl;
      } catch (...) {
         fail();
      }

      std::cout << "machine.context.x = " << machine.context.x << std::endl;
      std::cout << "machine.context.y = " << machine.context.y << std::endl;
      assert_equal(machine.context.x, (int) 8);
      assert_equal(machine.context.y, (int) 2);
   })
   .test("mixed lambdas and OOP states", []() {
      struct Context {
         int x = 0, y = 0;
      };

      class MixedState : public automata::State<Context> {
      public:
         using State::State;

         void run() {
            context.y ++;
            machine.terminate();
         }
      };
      
      auto machine = automata::Lambda<Context>::builder()
      .context(Context())
      .build();

      machine.def_state("init", [](auto& m) {
         m.context.x ++;
         m.template push_state<MixedState>();
      });
      
      machine.push("init");
      machine.run_until_complete();

      assert_equal(machine.context.x, (int) 1);
      assert_equal(machine.context.y, (int) 1);
   })
   .test("State init and cleanup functions called appropriately.", []() {
      struct Context {
         int init_called = 0;
         int run_called = 0;
         int cleanup_called = 0;
      };

      typedef automata::State<Context> State;

      class SampleState : public State {
      public:
         using State::State;

         ~SampleState() {
            context.cleanup_called ++;
         }

         void init() {
            context.init_called ++;
         }

         void run() override {
            if (context.run_called == 0) {
               context.run_called ++;
               push<SampleState>();

            } else {
               context.run_called ++;
               pop();
            }
         }
      };
      
      Context context;
      auto machine = State::Machine::init<SampleState>(context);
      machine.run_until_complete();

      std::cout << "context.init_called = " << context.init_called << std::endl;
      std::cout << "context.run_called = " << context.run_called << std::endl;
      std::cout << "context.cleanup_called = " << context.cleanup_called << std::endl;

      assert_equal(context.init_called, 2);
      assert_equal(context.run_called, 3);
      assert_equal(context.cleanup_called, 2);
   })
   .run();
}
