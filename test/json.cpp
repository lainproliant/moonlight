#include "moonlight/json.h"
#include "moonlight/test.h"

#include <cstdio>

using namespace std;
using namespace moonlight;
using namespace moonlight::test;

int main() {
    return TestSuite("moonlight json tests")
    .die_on_signal(SIGSEGV)
    .test("json::Object-001: Loading settings from a file", [&]()->bool {
        json::Object settings = json::read_file<json::Object>("data/test001.json");
        json::Object graphics_settings = settings.get<json::Object>("graphics");
        assert_true(graphics_settings.get<int>("width") == 1920);
        assert_true(graphics_settings.get<int>("height") == 1080);
        return true;
    })
    .test("json::Object-002: Using default values for settings", [&]()->bool {
        json::Object settings;
        json::Object graphics_settings = settings.get_or_set<json::Object>("graphics", json::Object());
        int width = graphics_settings.get_or_set<int>("width", 1920);
        int height = graphics_settings.get_or_set<int>("height", 1080);
        bool fullscreen = graphics_settings.get_or_set<bool>("fullscreen", false);

        assert_true(width == 1920);
        assert_true(height == 1080);
        assert_true(fullscreen == false);
        assert_true(graphics_settings.get<int>("width") == 1920);
        assert_true(graphics_settings.get<int>("height") == 1080);

        settings.set<json::Object>("graphics", graphics_settings);
        graphics_settings = settings.get<json::Object>("graphics");
        assert_true(graphics_settings.get<int>("width") == 1920);
        assert_true(graphics_settings.get<int>("height") == 1080);
        assert_true(graphics_settings.get<bool>("fullscreen") == false);

        return true;
    })
    .test("json::Object-003: Write a new settings value based on defaults", [&]()->bool {
        json::Object settings;
        json::Object graphics_settings = settings.get<json::Object>("graphics", json::Object());

        int width = graphics_settings.get_or_set<int>("width", 1920);
        int height = graphics_settings.get_or_set<int>("height", 1080);

        assert_true(width == 1920);
        assert_true(height == 1080);
        assert_true(graphics_settings.get<int>("width") == 1920);
        assert_true(graphics_settings.get<int>("height") == 1080);

        settings.set<json::Object>("graphics", graphics_settings);
        json::write_file("json::Object-003.json.output", settings);
        settings = json::read_file<json::Object>("json::Object-003.json.output");
        remove("json::Object-003.json.output");

        graphics_settings = settings.get<json::Object>("graphics");
        assert_true(graphics_settings.get<int>("width") == 1920);
        assert_true(graphics_settings.get<int>("height") == 1080);

        return true;
    })
    .test("json::Object-004: Load arrays from settings keys", [&]()->bool {
        json::Object settings = json::read_file<json::Object>("data/test004.json");

        vector<int> integers = settings.get<json::Array>("numbers").extract<int>();
        vector<string> strings = settings.get<json::Array>("strings").extract<std::string>();

        assert_true(lists_equal(integers, {1, 2, 3, 4, 5}));
        assert_true(lists_equal(strings, {"alpha", "bravo", "charlie", "delta", "eagle"}));
        assert_false(lists_equal(integers, {1, 2, 3, 4}));
        assert_false(lists_equal(strings, {"alpha", "bravo", "charlie", "delta"}));

        return true;
    })
    .test("json::Object-005: Load and save arrays with defaults", [&]()->bool {
        json::Object settings;

        vector<int> integers = settings.get_or_set<json::Array>("numbers", {1, 2, 3, 4, 5}).extract<int>();
        assert_true(lists_equal(integers, {1, 2, 3, 4, 5}));
        json::write_file("json::Object-005.json.output", settings);
        settings = json::read_file<json::Object>("json::Object-005.json.output");
        remove("json::Object-005.json.output");
        integers = settings.get<json::Array>("numbers").extract<int>();
        assert_true(lists_equal(integers, {1, 2, 3, 4, 5}));

        return true;
    })
    .test("json::Object-006: Heterogenous lists throw json::Error", [&]()->bool {
        json::Object settings = json::read_file<json::Object>("data/test006.json");
        try {
            vector<int> integers = settings.get<json::Array>("numbers").extract<int>();

        } catch (const json::Error& e) {
            cout << "Received expected json::Error: "
            << e.get_message()
            << endl;
            return true;
        }

        return false;
    })
    .test("json::Object-007: Creation of settings objects with float values", [&]()->bool {
        json::Object settings;
        settings.set<float>("pi", 3.14159);
        cout << json::to_string(settings) << endl;
        return true;
    })
    .test("json::Object-008: Extract a map from an object.", [&]()->bool {
        json::Object settings;
        settings.set<int>("a", 1);
        settings.set<int>("b", 2);
        settings.set<int>("c", 3);

        auto map = settings.extract<int>();

        assert_equal(map["a"], 1);
        assert_equal(map["b"], 2);
        assert_equal(map["c"], 3);

        settings.set<std::string>("s", "meow");

        try {
            map = settings.extract<int>();

        } catch (const json::Error& e) {
            cout << "Received expected json::Error: "
            << e.get_message()
            << endl;
            return true;
        }

        return false;
    })
    .run();
}
