#include "moonlight/automata.h"
#include "moonlight/test.h"
#include <iostream>

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
               context().push_back(++x);
               if (x >= 100) {
                  terminate();

               } else if (x >= (2*n)) {
                  transition<CountingState>(x * 2);
               }
            }

         private:
            int n;
            int x;
         };

         State::Machine machine;
         std::vector<int> context;
         machine.init<CountingState>(context, 1);
         auto future = machine.run();
         future.wait();

         for (int x : context) {
            std::cout << x << " ";
         }
         std::cout << std::endl;
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
               parent()->run();
               context().y ++;

               if (context().y >= 10) {
                  terminate();
               }
            }
         };

         State::Machine machine;
         Context context;
         machine.init<XState>(context);
         machine.run_until_complete();
         assert_equal(context.x, 20);
         assert_equal(context.y, 10);
      })
      .run();
}
