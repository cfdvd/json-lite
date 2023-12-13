#ifndef __JSON_ANALYSIS_HPP
#define __JSON_ANALYSIS_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <unordered_map>
#include <charconv>

#include "fmt/format.h"

#include "boost/variant.hpp"
#include "boost/assert.hpp"
#include "boost/optional.hpp"
#include "boost/xpressive/xpressive.hpp"

namespace JsonAnalysis {

template <typename... T>
struct Overload;

template <typename T>
struct Overload<T>: T {
    Overload(T _t): T(_t) {}
    using T::operator();
};

template <typename T, typename... U>
struct Overload<T, U...>: T, Overload<U...> {
    Overload(T _t, U... _u): T(_t), Overload<U...>(_u...) {}
    using T::operator();
    using Overload<U...>::operator();
};

template <typename... T>
Overload<T...> make_overload(T... t) {
    return Overload<T...>(t...);
}

struct JsonData_s {
    boost::variant<
            boost::blank,
            std::nullptr_t,
            bool,
            int,
            double,
            std::string,
            std::vector<JsonData_s>,
            std::unordered_map<std::string, JsonData_s>
    > inner;
};

class JsonAnalysis {
public:
    explicit JsonAnalysis(const std::string &_json): json(_json) {}
    explicit JsonAnalysis(std::string &&_json): json(std::forward<std::string_view>(_json)) {}
    explicit JsonAnalysis(const std::string_view &_json): json(_json) {}
    explicit JsonAnalysis(std::string_view &&_json): json(std::forward<std::string_view>(_json)) {}

    explicit JsonAnalysis(std::fstream &_file) {
        if(!_file.is_open()) { throw std::logic_error("file can't open"); }
        std::copy(std::istream_iterator<std::string>(_file), std::istream_iterator<std::string>(), std::ostream_iterator<std::string>(JsonAnalysis::JsonAnalysis::ss));
        ss >> json;
    }
    explicit JsonAnalysis(std::fstream &&_file) {
        if(!_file.is_open()) { throw std::logic_error("file can't open"); }
        std::copy(std::istream_iterator<std::string>(_file), std::istream_iterator<std::string>(), std::ostream_iterator<std::string>(JsonAnalysis::JsonAnalysis::ss));
        ss >> json;
    }

    template <typename T>
    boost::optional<T> ConvertToNumber(std::string_view str) {
        T value {};
        auto ret = std::from_chars(str.begin(), str.end(), value);
        if(ret.ec == std::errc() && ret.ptr == str.end()) {
            return value;
        }
        return boost::none;
    }

    std::pair<int, JsonData_s> Parse(std::string_view str) {
        int strSize = str.size();

        if(str[0] == '{') {
            std::unordered_map<std::string, JsonData_s> ret {};
            int i = 1;
            for(; i != strSize;) {
                if(str[i] == '}') { ++i; break; }
                auto [keyEaten, key] = Parse(str.substr(i));
                i += keyEaten;

                if(str[i] == ':') { ++i; }
                auto [valueEaten, value] = Parse(str.substr(i));
                i += valueEaten;

                ret.try_emplace(std::move(boost::get<std::string>(key.inner)), std::move(value));
                if(str[i] == ',') { ++i; }
            }
            return {i, JsonData_s{std::move(ret)}};
        }

        if(str[0] == '[') {
            std::vector<JsonData_s> ret {};
            int i = 1;
            for(; i != strSize;) {
                if(str[i] == ']') { ++i; break; }

                auto [objEaten, obj] = Parse(str.substr(i));
                i += objEaten;
                if(str[i] == ',') { ++i; }

                ret.push_back(std::move(obj));
            }
            return {i, JsonData_s{std::move(ret)}};
        }

        if(str[0] == '\"') {
            enum class CharacterType_e {
                NormalCharacter, EscapeCharacter
            } state = CharacterType_e::NormalCharacter;

            std::string ret {};
            int i = 1;
            for(; i != strSize; ++i) {
                if(state == CharacterType_e::NormalCharacter) {
                    if(str[i] == '\\') {
                        state = CharacterType_e::EscapeCharacter;
                    } else if(str[i] == '\"') {
                        ++i;
                        break;
                    } else {
                        ret += str[i];
                    }
                } else if(state == CharacterType_e::EscapeCharacter) {
                    switch(str[i]) {
                        case 'n': ret += '\n'; break;
                        case 'r': ret += '\r'; break;
                        case '0': ret += '\0'; break;
                        case 't': ret += '\t'; break;
                        case 'v': ret += '\v'; break;
                        case 'f': ret += '\f'; break;
                        case 'b': ret += '\b'; break;
                        case 'a': ret += '\a'; break;
                        default: ret += str[i];
                    }
                    state = CharacterType_e::NormalCharacter;
                }
            }

            return {i, JsonData_s{std::move(ret)}};
        }

        if((str[0] >= '0' && str[0] <= '9') || str[0] == '-' || str[0] == '+') {
            boost::xpressive::cmatch what {};
            if(boost::xpressive::regex_search(str.begin(), str.end(), what, NumberRegex)) {
                std::string matchStr = what.str();
                if(auto num = ConvertToNumber<int>(matchStr); num.has_value()) {
                    return {matchStr.size(), JsonData_s {num.value()}};
                }
                if(auto num = ConvertToNumber<double>(matchStr); num.has_value()) {
                    return {matchStr.size(), JsonData_s {num.value()}};
                }
            }
        }

        if(str[0] == 't') {
            std::string_view tmp {str.data(), str.data() + 4};
            if(tmp == "true") {
                return {4, JsonData_s {true}};
            }
        }

        if(str[0] == 'f') {
            std::string_view tmp {str.data(), str.data() + 5};
            if(tmp == "false") {
                return {5, JsonData_s {false}};
            }
        }

        if(str[0] == 'n') {
            std::string_view tmp {str.data(), str.data() + 4};
            if(tmp == "null") {
                return {4, JsonData_s {nullptr}};
            }
        }

        return {0, JsonData_s{}};
    }

    void StartAnalysisJson() {
        if(this->json.empty()) { throw std::logic_error("empty file"); }
        auto ret = Parse(this->json);
        this->jsonElem = std::move(ret.second);
        this->analysisFinish = true;
    }

    void PrintJsonElem() {
        if(!this->analysisFinish) {
            StartAnalysisJson();
        }

        auto myPrint = make_overload(
                [&](std::nullptr_t data) { boost::ignore_unused_variable_warning(data); std::cout << "\tnull \n"; },
                [&](bool data) { std::cout << fmt::format("\tbool, {}\n", data); },
                [&](int data) { std::cout << fmt::format("\tint, {}\n", data); },
                [&](double data) { std::cout << fmt::format("\tdouble, {}\n", data); },
                [&](std::string data) { std::cout << fmt::format("\tstring, {}\n", data); },
                [&](auto data) { boost::ignore_unused_variable_warning(data); std::cout << "\tunknown type\n"; }
        );

        // 递归的 lambda
        auto visitor = [&](auto &func, const JsonData_s &data) ->void {
            boost::apply_visitor([&](const auto &elem) ->void {
                if constexpr(std::is_same<typename std::decay<decltype(elem)>::type, std::vector<JsonData_s> >::value) {
                    for(const auto &subElem: elem) {
                        func(func, subElem);
                    }
                } else if constexpr (std::is_same<typename std::decay<decltype(elem)>::type, std::unordered_map<std::string, JsonData_s> >::value) {
                    for(const auto &subElem: elem) {
                        auto &[key, value] = subElem;
                        std::cout << fmt::format("key:\n\t{}\nvalue:\n[\n", key);
                        func(func, value);
                        std::cout << "]\n";
                    }
                } else {
                    myPrint(elem);
                }
            }, data.inner);
        };
        visitor(visitor, this->jsonElem);
    }
private:
    std::string json;
    JsonData_s jsonElem;
    bool analysisFinish {false};
    static const boost::xpressive::cregex NumberRegex;
    static std::stringstream ss;
};

const boost::xpressive::cregex JsonAnalysis::NumberRegex = boost::xpressive::cregex::compile(R"(([+-]?\d+)(.\d+)?([eE][+-]\d+)?)");
std::stringstream JsonAnalysis::ss = std::stringstream{};

}

#endif