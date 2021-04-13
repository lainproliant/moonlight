/*
 * json_write_example.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Monday April 12, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include <iostream>
#include "moonlight/json.h"

using namespace moonlight;

json::Object person() {
    json::Object obj;
    json::Array hobbies = {"Bicycling", "Gaming", "Programming"};
    hobbies.append(json::Object().set("sport", "billiards").set("num_balls", 15));
    hobbies.extend({"Magic the Gathering", "Swimming"});

    obj
        .set("name", "Lain")
        .set("age", 32)
        .set("eye_color", "blue")
        .set("avogadro's number", 6.02214076e23)
        .set("hobbies", hobbies);

    obj.set("negative 1 times avogadro's number", -1.0 * obj.get<double>("avogadro's number"));
    obj.set("inverse of negative avogadro's number", 1.0 / obj.get<double>("negative 1 times avogadro's number"));

    return obj;
}

int main() {
    json::Object lain = person();
    std::cout << json::to_string(lain) << std::endl;
    return 0;
}
