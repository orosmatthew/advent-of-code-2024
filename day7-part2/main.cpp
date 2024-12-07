#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

static std::string read_data(const std::filesystem::path& path)
{
    const std::fstream file { path };
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

enum class Operator { add, mul, concat };

static std::optional<Operator> next_operator(const Operator op)
{
    switch (op) {
    case Operator::add:
        return Operator::mul;
    case Operator::mul:
        return Operator::concat;
    default:
        return std::nullopt;
    }
}

static bool next_operators(std::vector<Operator>& ops)
{
    for (int i = 0; i < ops.size(); ++i) {
        if (std::optional<Operator> next = next_operator(ops[i]); next.has_value()) {
            ops[i] = next.value();
            break;
        }
        ops[i] = Operator::add;
        if (i == ops.size() - 1) {
            return false;
        }
    }
    return true;
}

static bool is_digit(const char c)
{
    return c >= '0' && c <= '9';
}

static std::optional<int64_t> parse_int_opt(const std::string& string, int& pos)
{
    if (!is_digit(string[pos])) {
        return std::nullopt;
    }
    int64_t result = 0;
    do {
        result = result * 10 + (string[pos] - '0');
        ++pos;
    } while (is_digit(string[pos]) && pos < string.length());
    return result;
}

struct Equation {
    int64_t result {};
    std::vector<int64_t> numbers;
};

static void parse_equation(const std::string& data, int& pos, Equation& equation)
{
    equation.numbers.clear();
    equation.result = parse_int_opt(data, pos).value();
    pos += 2; // :
    while (true) {
        equation.numbers.push_back(parse_int_opt(data, pos).value());
        if (data[pos] == '\n') {
            break;
        }
        ++pos; // " "
    }
}

constexpr std::array pow10 {
    1LL,
    10LL,
    100LL,
    1000LL,
    10000LL,
    100000LL,
    1000000LL,
    10000000LL,
    100000000LL,
    1000000000LL,
    10000000000LL,
    100000000000LL,
    1000000000000LL,
    10000000000000LL,
    100000000000000LL,
    1000000000000000LL,
    10000000000000000LL,
    100000000000000000LL,
    1000000000000000000LL,
};

static int digits_count(const int64_t num)
{
    int count = 1;
    for (int i = 1; i < pow10.size(); ++i) {
        if (num >= pow10[i]) {
            ++count;
        }
        else {
            break;
        }
    }
    return count;
}

static bool evaluate_equals(const std::vector<int64_t>& numbers, const std::vector<Operator>& ops, const int64_t value)
{
    assert(!numbers.empty());
    assert(ops.size() == numbers.size() - 1);
    int64_t result = value;
    for (int i = static_cast<int>(numbers.size()) - 1; i > 0; --i) {
        const int64_t num = numbers[i];
        switch (ops[i - 1]) {
        case Operator::add:
            result -= num;
            break;
        case Operator::mul:
            if (result % num != 0) {
                return false;
            }
            result /= num;
            break;
        case Operator::concat:
            const int64_t divisor = pow10[digits_count(num)];
            if (result % divisor != num) {
                return false;
            }
            result /= divisor;
            break;
        }
    }
    return result == numbers[0];
}

static bool validate_equation(const Equation& equation)
{
    static std::vector<Operator> ops;
    ops.clear();
    ops.resize(equation.numbers.size() - 1, Operator::add);
    do {
        if (evaluate_equals(equation.numbers, ops, equation.result)) {
            return true;
        }
    } while (next_operators(ops));
    return false;
}

static int64_t solve(const std::string& data)
{
    Equation equation;
    int pos = 0;
    int64_t result = 0;
    while (pos < data.size()) {
        parse_equation(data, pos, equation);
        ++pos; // \n
        if (validate_equation(equation)) {
            result += equation.result;
        }
    }
    return result;
}

int main()
{
    const std::string data = read_data("./day7-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile int64_t result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%lld\n", solve(data));
#endif
}