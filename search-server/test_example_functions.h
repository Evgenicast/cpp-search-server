#pragma once

#include "search_server.h"

#include <vector>
#include <string>
#include <iostream>

using std::string_literals::operator""s;

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))
#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
#define RUN_TEST(func)  RunTestImpl(func, #func)

void TestSearchServer();

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint)
{
    if (t != u)
    {
        std::cerr << std::boolalpha;
        std::cerr << file << "("s << line << "s): "s << func << ": "s;
        std::cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << "s) failed: "s;
        std::cerr << t << " != "s << u << "."s;
        if (!hint.empty())
        {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
    }
}

void AssertImpl(bool value, const std::string& expr_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint);

template <typename Function>
void RunTestImpl(Function func, const std::string& funcName)
{
    func();
    std::cerr << funcName << " OK"s << std::endl;
}
