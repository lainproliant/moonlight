#include <cstdio>
#include <filesystem>
#include "moonlight/json.h"
#include "moonlight/test.h"
#include "moonlight/date.h"

using namespace std;
using namespace moonlight;
using namespace moonlight::test;
using namespace moonlight::date;

const Duration PERF_TEST_DURATION = Duration::of_seconds(5);
const std::filesystem::path LARGE_JSON = "./test/data/large-file.json";

int main() {
    return TestSuite("moonlight json tests")
    .test("Loading settings from a file", []() {
        json::Object settings = json::read_file<json::Object>("test/data/test001.json");
        json::Object graphics_settings = settings.get<json::Object>("graphics");
        ASSERT(graphics_settings.get<int>("width") == 1920);
        ASSERT(graphics_settings.get<int>("height") == 1080);
    })
    .test("Using default values for settings", []() {
        json::Object settings;
        json::Object graphics_settings = settings.get_or_set<json::Object>("graphics", json::Object());
        int width = graphics_settings.get_or_set<int>("width", 1920);
        int height = graphics_settings.get_or_set<int>("height", 1080);
        bool fullscreen = graphics_settings.get_or_set<bool>("fullscreen", false);

        ASSERT(width == 1920);
        ASSERT(height == 1080);
        ASSERT(fullscreen == false);
        ASSERT(graphics_settings.get<int>("width") == 1920);
        ASSERT(graphics_settings.get<int>("height") == 1080);

        settings.set<json::Object>("graphics", graphics_settings);
        graphics_settings = settings.get<json::Object>("graphics");
        ASSERT(graphics_settings.get<int>("width") == 1920);
        ASSERT(graphics_settings.get<int>("height") == 1080);
        ASSERT(graphics_settings.get<bool>("fullscreen") == false);
    })
    .test("Write a new settings value based on defaults", []() {
        json::Object settings;
        json::Object graphics_settings = settings.get<json::Object>("graphics", json::Object());

        int width = graphics_settings.get_or_set<int>("width", 1920);
        int height = graphics_settings.get_or_set<int>("height", 1080);

        ASSERT(width == 1920);
        ASSERT(height == 1080);
        ASSERT(graphics_settings.get<int>("width") == 1920);
        ASSERT(graphics_settings.get<int>("height") == 1080);

        settings.set<json::Object>("graphics", graphics_settings);
        json::write_file("json::Object-003.json.output", settings);
        settings = json::read_file<json::Object>("json::Object-003.json.output");
        remove("json::Object-003.json.output");

        graphics_settings = settings.get<json::Object>("graphics");
        ASSERT(graphics_settings.get<int>("width") == 1920);
        ASSERT(graphics_settings.get<int>("height") == 1080);
    })
    .test("Load arrays from settings keys", []() {
        json::Object settings = json::read_file<json::Object>("test/data/test004.json");

        vector<int> integers = settings.get<json::Array>("numbers").extract<int>();
        vector<string> strings = settings.get<json::Array>("strings").extract<std::string>();

        ASSERT_EQUAL(integers, {1, 2, 3, 4, 5});
        ASSERT_EQUAL(strings, {"alpha", "bravo", "charlie", "delta", "eagle"});
        ASSERT_NOT_EQUAL(integers, {1, 2, 3, 4});
        ASSERT_NOT_EQUAL(strings, {"alpha", "bravo", "charlie", "delta"});
    })
    .test("Load and save arrays with defaults", []() {
        json::Object settings;

        vector<int> integers = settings.get_or_set<json::Array>("numbers", {1, 2, 3, 4, 5}).extract<int>();
        ASSERT_EQUAL(integers, {1, 2, 3, 4, 5});
        json::write_file("json::Object-005.json.output", settings);
        settings = json::read_file<json::Object>("json::Object-005.json.output");
        remove("json::Object-005.json.output");
        integers = settings.get<json::Array>("numbers").extract<int>();
        ASSERT_EQUAL(integers, {1, 2, 3, 4, 5});
    })
    .test("Heterogenous lists throw json::Error", []() {
        json::Object settings = json::read_file<json::Object>("test/data/test006.json");
        try {
            vector<int> integers = settings.get<json::Array>("numbers").extract<int>();
            FAIL("Expected json::Error was not thrown.");

        } catch (const core::TypeError& e) {
            cout << "Caught expected " << e << endl;
        }
    })
    .test("Creation of settings objects with float values", []() {
        json::Object settings;
        settings.set<float>("pi", 3.14159);
        cout << json::to_string(settings) << endl;
    })
    .test("Extract a map from an object.", []() {
        json::Object settings;
        settings.set<int>("a", 1);
        settings.set<int>("b", 2);
        settings.set<int>("c", 3);

        auto map = settings.extract<int>();
        cout << "map.size() = " << map.size() << std::endl;
        for (auto pair : map) {
            cout << pair.first << " => " << pair.second << std::endl;
        }

        ASSERT_EQUAL(map["a"], 1);
        ASSERT_EQUAL(map["b"], 2);
        ASSERT_EQUAL(map["c"], 3);

        settings.set<std::string>("s", "meow");

        try {
            map = settings.extract<int>();
            FAIL("Expected TypeError was not thrown.");

        } catch (const core::TypeError& e) {
            cout << "Received expected " << e << endl;
        }
    })
    .test("Iterate directly over the contents of a homogenous array.", []() {
        auto array = json::read<json::Array>("[1,2,3,4,5,6,7]");
        auto vector = array.extract<int>();
        std::vector<int> vec_copy;

        for (auto val : array.stream<int>()) {
            vec_copy.push_back(val);
        }

        ASSERT_EQUAL(vector, vec_copy);
    })
    .test("Test large file read performance", []() {
        Datetime start = Datetime::now();
        int count = 0;

        while (Datetime::now() < start + PERF_TEST_DURATION) {
            json::Array large_array = json::read_file<json::Array>(LARGE_JSON);
            count++;
        }

        std::cout << std::endl;
        std::cout << "Loaded large JSON file " << count << " times in " << PERF_TEST_DURATION << std::endl;
    })
    .test("Test large file write performance", []() {
        Datetime start = Datetime::now();
        int count = 0;

        json::Array large_array = json::read_file<json::Array>(LARGE_JSON);

        while (Datetime::now() < start + PERF_TEST_DURATION) {
            file::TemporaryFile tempfile("", ".json");
            std::cout << "Writing to " << tempfile.name() << std::endl;
            json::write(tempfile.output(), large_array);
            count++;
        }

        std::cout << std::endl;
        std::cout << "Wrote a large JSON file " << count << " times in " << PERF_TEST_DURATION << std::endl;
    })
    .test("Object mappings", []() {
        class Address {
         public:
             int number;
             std::string street;
             std::string city;
             std::string state;
             int zip;

             bool operator==(const Address&) const = default;

             json::Mapper<Address> __json__() {
                 return json::Mapper(this)
                 .field("number", number)
                 .field("street", street)
                 .field("city", city)
                 .field("state", state)
                 .field("zip", zip);
             }
        };

        class Person {
         public:
             std::string name = "";
             Address address;
             std::vector<Address> work_addresses;
             std::map<std::string, std::string> alternate_names;

             bool operator==(const Person&) const = default;

             Person& set_address(const Address& addr) {
                 address = addr;
                 return *this;
             }

             const Address& get_address() const {
                 return address;
             }

             const std::map<std::string, std::string>& get_altnames() const {
                 return alternate_names;
             }

             const void set_altnames(const std::map<std::string, std::string>& names) {
                 alternate_names = names;
             }

             json::Mapper<Person> __json__() {
                 return json::Mapper(this)
                 .field("name", name)
                 .field("work_addresses", work_addresses)
                 .property("other_names", &Person::get_altnames, &Person::set_altnames)
                 .property("address", &Person::get_address, &Person::set_address);
             }
        };

        auto person = json::read_file<Person>("test/data/test-json-mapping.json");
        ASSERT_EQUAL(person.name, "Lain Musgrove");
        ASSERT_EQUAL(person.address.number, 2235);
        ASSERT_EQUAL(person.address.street, "Schley Blvd");
        ASSERT_EQUAL(person.address.city, "Bremerton");
        ASSERT_EQUAL(person.address.state, "WA");
        ASSERT_EQUAL(person.address.zip, 98310);

        std::ostringstream sb;
        json::write(sb, person);
        std::string json_str = sb.str();

        std::istringstream si(json_str);

        auto new_person = json::read<Person>(si);
        std::cout << json::map(person) << std::endl;
        std::cout << json::map(new_person) << std::endl;

        ASSERT_EQUAL(person, new_person);
    })
    .run();
}
