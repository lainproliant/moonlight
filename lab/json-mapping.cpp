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
class Address {
public:
    int number;
    std::string street;
    std::string city;
    std::string state;
    int zip;

    json::Mapper<Address> __json__() {
        return json::Mapper(this)
            .field("number", number)
            .field("street", street)
            .field("city", city)
            .field("state", state)
            .field("zip", zip);
    }

    friend std::ostream& operator<<(std::ostream& out, const Address& addr) {
        return out << addr.number << " "
                   << addr.street << ", "
                   << addr.city << " "
                   << addr.state << " "
                   << addr.zip;
    }
};

//-------------------------------------------------------------------
class Person {
public:
    std::string name = "";
    Address address;

    Person& set_address(const Address& addr) {
        address = addr;
        return *this;
    }

    const Address& get_address() const {
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
    std::string json = "{\"name\":\"lain\", \"address\":{\"number\":2235, \"street\":\"Schley Blvd\", \"city\":\"Bremerton\", \"state\":\"WA\", \"zip\":98310}}";
    auto json_obj = json::read<json::Object>(json);
    auto obj = json::map<Person>(json_obj);
    std::cout << obj.name << std::endl;
    std::cout << obj.address << std::endl;
    std::cout << json::to_string(json::map(obj)) << std::endl;
    return 0;
}
