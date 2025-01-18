/*
 * sqlite3.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday January 17, 2025
 */

#include "moonlight/sql/sqlite3.h"

#include <iostream>
#include <csignal>
#include "moonlight/test.h"

using namespace moonlight;
using namespace moonlight::test;

void create_test_tables(sql::Client::Pointer sql) {
    sql->exec(
        "create table scoreboard ("
        "   id integer primary key autoincrement,"
        "   name text not null,"
        "   score real not null default 0.0,"
        "   notes text"
        ");"
    );
}

void print_scoreboard(sql::Client::Pointer sql) {
    std::cout << "----------" << std::endl;
    sql->query("select * from scoreboard order by score desc").for_each([](auto row) {
        std::cout << row->at("id").as_int() << " "
                  << row->at(1).as_str() << " -> "
                  << row->at("score").as_float() << std::endl;
    });
}

void assert_scoreboard(sql::Client::Pointer sql, const std::vector<std::pair<std::string, float>>& values) {
    auto results = sql->query("select name, score from scoreboard order by name").transform<std::pair<std::string, float>>([](auto row) {
        return std::pair<std::string, float>(row->at("name").as_str(), row->at("score").as_float());
    }).collect();
    ASSERT_EQUAL(results, values);
}

int main() {
    return TestSuite("moonlight sqlite3 tests")
    .test("test basic table creation and querying", []() {
        auto sql = sqlite::Client::open("");
        create_test_tables(sql);
        sql->exec("insert into scoreboard (name) values (?)", "Lain");
        sql->exec("insert into scoreboard (name) values (?)", "Jenna");
        sql->exec("insert into scoreboard (name) values (?)", "Holly");
        print_scoreboard(sql);
        assert_scoreboard(sql, {{"Holly", 0.0}, {"Jenna", 0.0}, {"Lain", 0.0}});
        sql->exec("update scoreboard set score = ? where name = ?", 7.5, "Jenna");
        sql->exec("update scoreboard set score = ? where name = ?", 1.0, "Lain");
        sql->exec("update scoreboard set score = ? where name = ?", 10.0, "Holly");
        print_scoreboard(sql);
        assert_scoreboard(sql, {{"Holly", 10.0}, {"Jenna", 7.5}, {"Lain", 1.0}});

        sql->exec("delete from scoreboard where name = ?", "Lain");
        print_scoreboard(sql);
        assert_scoreboard(sql, {{"Holly", 10.0}, {"Jenna", 7.5}});
    })
    .run();
}
