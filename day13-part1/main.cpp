#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>
#include <utility>
#include <vector>

static std::string read_data(const std::filesystem::path& path)
{
    const std::fstream file { path };
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

struct Vector2i {
    int x;
    int y;

    Vector2i& operator+=(const Vector2i& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2i operator+(const Vector2i& other) const
    {
        return { x + other.x, y + other.y };
    }

    Vector2i& operator-=(const Vector2i& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2i operator-(const Vector2i& other) const
    {
        return { x - other.x, y - other.y };
    }

    bool operator==(const Vector2i& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Vector2i& other) const
    {
        return x != other.x || y != other.y;
    }

    bool operator<(const Vector2i& other) const
    {
        if (x == other.x) {
            return y < other.y;
        }
        return x < other.x;
    }
};

static bool is_digit(const char c)
{
    return c >= '0' && c <= '9';
}

static int parse_int(const std::string& string, int& pos)
{
    int result = 0;
    do {
        result = result * 10 + (string[pos] - '0');
        ++pos;
    } while (is_digit(string[pos]) && pos < string.length());
    return result;
}

class ClawMachine {
public:
    static ClawMachine parse(const std::string& data, int& pos)
    {
        pos += 12; // "Button A: X+"
        Vector2i button_a {};
        button_a.x = parse_int(data, pos);
        pos += 4; // ", Y+"
        button_a.y = parse_int(data, pos);
        pos += 13; // "\nButton B: X+"
        Vector2i button_b {};
        button_b.x = parse_int(data, pos);
        pos += 4; // ", Y+"
        button_b.y = parse_int(data, pos);
        pos += 10; // "\nPrize: X="
        Vector2i prize {};
        prize.x = parse_int(data, pos);
        pos += 4; // ", Y="
        prize.y = parse_int(data, pos);
        return { button_a, button_b, prize };
    }

    [[nodiscard]] std::optional<uint64_t> tokens_to_win() const
    {
        const std::optional<Presses> presses = presses_to_win();
        if (!presses.has_value()) {
            return std::nullopt;
        }
        return presses.value().button_a * 3 + presses.value().button_b;
    }

private:
    ClawMachine(const Vector2i& button_a, const Vector2i& button_b, const Vector2i& prize)
        : m_button_a { button_a }
        , m_button_b { button_b }
        , m_prize { prize }
    {
    }

    struct Presses {
        int button_a;
        int button_b;
    };

    [[nodiscard]] std::optional<Presses> presses_to_win() const
    {
        // equation derivations can be found in day13-part1/math.md
        Presses presses {};
        const int a_num = m_prize.y * m_button_b.x - m_prize.x * m_button_b.y;
        const int a_denom = m_button_a.y * m_button_b.x - m_button_a.x * m_button_b.y;
        if (a_num % a_denom != 0) {
            return std::nullopt;
        }
        presses.button_a = a_num / a_denom;
        const int b_num = -m_button_a.x * presses.button_a + m_prize.x;
        const int b_denom = m_button_b.x;
        if (b_num % b_denom != 0) {
            return std::nullopt;
        }
        presses.button_b = b_num / b_denom;
        return presses;
    }

    Vector2i m_button_a;
    Vector2i m_button_b;
    Vector2i m_prize;
};

static uint64_t solve(const std::string& data)
{
    uint64_t total_tokens = 0;
    for (int pos = 0; pos < data.size();) {
        const ClawMachine machine = ClawMachine::parse(data, pos);
        if (std::optional<uint64_t> tokens = machine.tokens_to_win(); tokens.has_value()) {
            total_tokens += tokens.value();
        }
        pos += 2; // "\n\n"
    }
    return total_tokens;
}

int main()
{
    const std::string data = read_data("./day13-part1/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100000;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile uint64_t result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%llu\n", solve(data));
#endif
}