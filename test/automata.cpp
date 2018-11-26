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
            context().x ++;

            if (is_current() && context().x >= 10) {
               push<YState>();
            }
         }
      };

      class YState : public State {
      public:
         using State::State;

         virtual void run() override {
            parent();
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
         m.context().x++;

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
         m.context().y++;

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

      } catch (const automata::Error& e) {
         // This is supposed to happen, because "a" tries to call
         // parent(), but in this case there is no parent to "a".
         std::cout << "Caught expected automata::Error." << std::endl;
      } catch (...) {
         fail();
      }

      std::cout << "machine.context().x = " << machine.context().x << std::endl;
      std::cout << "machine.context().y = " << machine.context().y << std::endl;
      assert_equal(machine.context().x, (int) 8);
      assert_equal(machine.context().y, (int) 2);
   })
   .run();
}
