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
    int64_t x;
    int64_t y;

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

    Vector2i operator*(const int64_t value) const
    {
        return { x * value, y * value };
    }

    Vector2i operator/(const int64_t value) const
    {
        return { x / value, y / value };
    }

    Vector2i& operator%=(const Vector2i& other)
    {
        x %= other.x;
        y %= other.y;
        return *this;
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

static int64_t parse_int(const std::string& string, int& pos)
{
    int64_t result = 0;
    const bool negative = string[pos] == '-';
    if (negative) {
        ++pos;
    }
    do {
        result = result * 10 + (string[pos] - '0');
        ++pos;
    } while (is_digit(string[pos]) && pos < string.length());
    return negative ? -result : result;
}

class Map {
public:
    static Map parse(const std::string& data, const Vector2i& map_size)
    {
        std::vector<Robot> robots;
        int pos = 0;
        while (pos < data.size()) {
            Robot robot {};
            pos += 2; // "p="
            robot.pos.x = parse_int(data, pos);
            ++pos; // ","
            robot.pos.y = parse_int(data, pos);
            pos += 3; // " v="
            robot.vel.x = parse_int(data, pos);
            ++pos; // ","
            robot.vel.y = parse_int(data, pos);
            ++pos; // "\n"
            robots.push_back(robot);
        }
        return { std::move(robots), map_size };
    }

    [[nodiscard]] uint64_t safety_factor() const
    {
        std::array<uint64_t, 4> quadrant_counts { 0, 0, 0, 0 };
        for (const auto& [pos, vel] : m_robots) {
            Vector2i new_pos = pos + vel * 100;
            new_pos %= m_size;
            new_pos.x = new_pos.x < 0 ? m_size.x + new_pos.x : new_pos.x;
            new_pos.y = new_pos.y < 0 ? m_size.y + new_pos.y : new_pos.y;
            if (std::optional<int> quadrant = pos_quadrant(new_pos); quadrant.has_value()) {
                ++quadrant_counts[quadrant.value()];
            }
        }
        return quadrant_counts[0] * quadrant_counts[1] * quadrant_counts[2] * quadrant_counts[3];
    }

private:
    struct Robot {
        Vector2i pos;
        Vector2i vel;
    };

    Map(std::vector<Robot>&& robots, const Vector2i& size)
        : m_robots { std::move(robots) }
        , m_size { size }
    {
    }

    [[nodiscard]] std::optional<int> pos_quadrant(const Vector2i& pos) const
    {
        if (const auto [hx, hy] = m_size / 2; pos.y < hy) {
            if (pos.x < hx) {
                return 0;
            }
            if (pos.x > hx) {
                return 1;
            }
        }
        else if (pos.y > hy) {
            if (pos.x < hx) {
                return 2;
            }
            if (pos.x > hx) {
                return 3;
            }
        }
        return std::nullopt;
    }

    std::vector<Robot> m_robots;
    Vector2i m_size;
};

static uint64_t solve(const std::string& data, const Vector2i& map_size)
{
    const Map map = Map::parse(data, map_size);
    return map.safety_factor();
}

int main()
{
    const std::string data = read_data("./day14-part1/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100000;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile uint64_t result = solve(data, { 101, 103 });
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%llu\n", solve(data, { 101, 103 }));
#endif
}