/*
 * json_mapping.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Monday September 13, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include <string>
#include "moonlight/json.h"

//-------------------------------------------------------------------
namespace json = moonlight::json;

//-------------------------------------------------------------------
struct Person {
    std::string name = "";
    std::string address = "";

    Person& set_address(const std::string& addr) {
        address = addr;
        return *this;
    }

    const std::string& get_address() const {
        return address;
    }

    json::Mapper<Person> __json__() {
        return json::Mapper(this)
            .field("name", name)
            .property("address", &Person::get_address, &Person::set_address);
    }
};

//-------------------------------------------------------------------
int main() {
    std::string json = "{\"name\":\"lain\", \"address\":\"localhost\"}";
    auto json_obj = json::read<json::Object>(json);
    auto obj = json::map<Person>(json_obj);
    std::cout << obj.name << std::endl;
    std::cout << obj.address << std::endl;
    std::cout << json::to_string(json::map(obj)) << std::endl;
    return 0;
}
