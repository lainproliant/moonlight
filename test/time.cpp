#include "moonlight/posix.h"
#include "moonlight/test.h"
#include <iostream>
#include <unistd.h>

using namespace moonlight;
using namespace moonlight::test;

double run_timer_sim(int fps, const unsigned int frames) {
   auto timer = time::posix::create_timer(1000 / fps);
   timer.start();

   double totalErrorTime = 0.0;
   unsigned long terr;

   while (timer.frames() < frames) {
      if (timer.update(&terr)) {
         totalErrorTime += terr;

      } else {
         usleep(timer.wait_time() + 5 + rand() % 5);
      }
   }

   std::cout << std::endl;

   return totalErrorTime;
}

int main() {
   srand(::time(0));

   return TestSuite("Time tests")
   .test("Testing average error rate at various FPS", []() {
      for (int fps = 15; fps <= 120; fps *= 2) {
         std::cout << "Testing " << fps << "fps:" << std::endl;
         double totalErrorTime = run_timer_sim(fps, 10);
         double avgErrorPerFrame = totalErrorTime / 100;
         assert_true(avgErrorPerFrame < 1.0);
      }
   })
   .test("Sample physics and graphics timers", []() {
      const int SECONDS = 3;
      const int GRAPHICS_FPS = 30;
      const int PHYSICS_FPS = 100;

      auto graphics_timer = time::posix::create_timer(1000 / GRAPHICS_FPS);
      auto physics_timer = time::posix::create_timer(1000 / PHYSICS_FPS, true); // accumulate

      graphics_timer.start();
      physics_timer.start();

      auto graphics_fps = time::posix::create_frame_calculator(graphics_timer);
      auto physics_fps = time::posix::create_frame_calculator(physics_timer);

      auto startTime = time::posix::get_ticks();
      double totalRenderTime = 0.0;
      double totalRenderErrorTime = 0.0;
      double totalPhysicsErrorTime = 0.0;

      while (time::posix::get_ticks() - startTime < (1000 * SECONDS)) {
         unsigned long terr;

         graphics_fps.update();
         physics_fps.update();

         if (graphics_timer.update(&terr)) {
            uint32_t renderStart = time::posix::get_ticks();
            usleep(5 + rand() % 5);
            uint32_t renderEnd = time::posix::get_ticks();
            totalRenderTime += renderEnd - renderStart;
            totalRenderErrorTime += terr;
         }

         while (physics_timer.update(&terr)) {
            totalPhysicsErrorTime += terr;
            usleep(rand() % 5);
         }

         usleep(graphics_timer.wait_time());
      }

      double avgRenderErrorTime = totalRenderErrorTime / graphics_timer.frames();
      double avgPhysicsErrorTime = totalPhysicsErrorTime / physics_timer.frames();
      double avgRenderTime = totalRenderTime / graphics_timer.frames();
      double correctedAvgPhysicsErrorTime = avgPhysicsErrorTime - avgRenderTime;

      std::cout << std::endl;
      std::cout << "Total graphics frames: "
         << graphics_timer.frames() << std::endl
         << "Total physics frames: "
         << physics_timer.frames() << std::endl
         << "Physics FPS: "
         << physics_fps.get_fps() << std::endl
         << "Avg render time: "
         << totalRenderTime / graphics_timer.frames() << std::endl
         << "Render FPS: "
         << graphics_fps.get_fps() << std::endl
         << "Avg physics error time - avg render time: "
         << correctedAvgPhysicsErrorTime << std::endl
         << "Avg render error time: "
         << avgRenderErrorTime << std::endl;

      assert_true(avgRenderErrorTime < 1.0);
      assert_true(correctedAvgPhysicsErrorTime < 1.0);
   })
   .run();
}
