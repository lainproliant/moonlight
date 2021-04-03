#include "moonlight/json.h"
#include "moonlight/test.h"

#include <cstdio>

using namespace std;
using namespace moonlight;
using namespace moonlight::test;

int main() {
   return TestSuite("moonlight json tests")
      .die_on_signal(SIGSEGV)
      .test("json::Wrapper-001: Loading settings from a file", [&]()->bool {
         json::Wrapper settings = json::Wrapper::load_from_file("data/test001.json");
         json::Wrapper graphics_settings = settings.get_object("graphics");
         assert_true(graphics_settings.get<int>("width") == 1920);
         assert_true(graphics_settings.get<int>("height") == 1080);
         return true;
      })
      .test("json::Wrapper-002: Using default values for settings", [&]()->bool {
         json::Wrapper settings;
         json::Wrapper graphics_settings = settings.get_object("graphics");
         int width = graphics_settings.get<int>("width", 1920);
         int height = graphics_settings.get<int>("height", 1080);
         bool fullscreen = graphics_settings.get<bool>("fullscreen", false);

         assert_true(width == 1920);
         assert_true(height == 1080);
         assert_true(fullscreen == false);
         assert_true(graphics_settings.get<int>("width") == 1920);
         assert_true(graphics_settings.get<int>("height") == 1080);

         settings.set_object("graphics", graphics_settings);
         graphics_settings = settings.get_object("graphics");
         assert_true(graphics_settings.get<int>("width") == 1920);
         assert_true(graphics_settings.get<int>("height") == 1080);
         assert_true(graphics_settings.get<bool>("fullscreen") == false);

         return true;
      })
      .test("json::Wrapper-003: Write a new settings value based on defaults", [&]()->bool {
         json::Wrapper settings;
         json::Wrapper graphics_settings = settings.get_object("graphics");

         int width = graphics_settings.get<int>("width", 1920);
         int height = graphics_settings.get<int>("height", 1080);

         assert_true(width == 1920);
         assert_true(height == 1080);
         assert_true(graphics_settings.get<int>("width") == 1920);
         assert_true(graphics_settings.get<int>("height") == 1080);

         settings.set_object("graphics", graphics_settings);
         settings.save_to_file("json::Wrapper-003.json.output");
         settings = json::Wrapper::load_from_file("json::Wrapper-003.json.output");
         remove("json::Wrapper-003.json.output");

         graphics_settings = settings.get_object("graphics");
         assert_true(graphics_settings.get<int>("width") == 1920);
         assert_true(graphics_settings.get<int>("height") == 1080);

         return true;
      })
      .test("json::Wrapper-004: Load arrays from settings keys", [&]()->bool {
         json::Wrapper settings = json::Wrapper::load_from_file("data/test004.json");

         vector<int> integers = settings.get_array<int>("numbers");
         vector<string> strings = settings.get_array<string>("strings");

         assert_true(lists_equal(integers, {1, 2, 3, 4, 5}));
         assert_true(lists_equal(strings, {"alpha", "bravo", "charlie", "delta", "eagle"}));
         assert_false(lists_equal(integers, {1, 2, 3, 4}));
         assert_false(lists_equal(strings, {"alpha", "bravo", "charlie", "delta"}));

         return true;
      })
      .test("json::Wrapper-005: Load and save arrays with defaults", [&]()->bool {
         json::Wrapper settings;

         vector<int> integers = settings.get_array<int>("numbers", {1, 2, 3, 4, 5});
         assert_true(lists_equal(integers, {1, 2, 3, 4, 5}));
         settings.save_to_file("json::Wrapper-005.json.output");
         settings = json::Wrapper::load_from_file("json::Wrapper-005.json.output");
         remove("json::Wrapper-005.json.output");
         integers = settings.get_array<int>("numbers");
         assert_true(lists_equal(integers, {1, 2, 3, 4, 5}));

         return true;
      })
      .test("json::Wrapper-006: Heterogenous lists throw json::WrapperException", [&]()->bool {
         json::Wrapper settings = json::Wrapper::load_from_file("data/test006.json");
         try {
            vector<int> integers = settings.get_array<int>("numbers");

         } catch (const json::WrapperException& e) {
            cerr << "Received expected json::WrapperException: "
                 << e.get_message()
                 << endl;
            return true;
         }

         return false;
      })
      .test("json::Wrapper-007: Creation of settings objects with float values", [&]()->bool {
         json::Wrapper settings;
         settings.set<float>("pi", 3.14159);
         cout << settings.to_string() << endl;
         return true;
      })
      .run();
}
