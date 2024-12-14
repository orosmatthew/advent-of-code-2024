#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

    [[maybe_unused]] void print(const int seconds) const
    {
        std::vector<bool> occupied;
        calculate_occupied(seconds, occupied);
        std::cout << "\n";
        std::cout << "===== " << seconds << " seconds =====\n";
        for (int y = 0; y < m_size.y; ++y) {
            for (int x = 0; x < m_size.x; ++x) {
                if (const size_t index = y * m_size.x + x; occupied[index]) {
                    std::cout << "#";
                }
                else {
                    std::cout << ".";
                }
            }
            std::cout << "\n";
        }
    }

    [[maybe_unused]] void output_images(const int max_seconds) const
    {
        if (!std::filesystem::exists("day14-part2/images")) {
            std::filesystem::create_directory("day14-part2/images");
        }
        std::vector<bool> occupied;
        std::vector<uint32_t> image_data;
        for (int i = 0; i < max_seconds; ++i) {
            image_data.clear();
            image_data.resize(m_size.x * m_size.y, 0xFF000000);
            calculate_occupied(i, occupied);
            for (int j = 0; j < m_size.x * m_size.y; ++j) {
                if (occupied[j]) {
                    image_data[j] = 0xFF0000FF;
                }
            }
            constexpr int channels = 4;
            std::stringstream ss;
            ss << "day14-part2/images/" << i << ".png";
            stbi_write_png(
                ss.str().c_str(),
                static_cast<int>(m_size.x),
                static_cast<int>(m_size.y),
                channels,
                image_data.data(),
                static_cast<int>(m_size.x) * channels);
        }
    }

    [[nodiscard]] int find_tree() const
    {
        int seconds = 0;
        std::vector<bool> occupied;
        while (true) {
            calculate_occupied(seconds, occupied);
            for (int y = 0; y < m_size.y - sc_tree_size.y; ++y) {
                for (int x = 0; x < m_size.x - sc_tree_size.x; ++x) {
                    for (int ty = 0; ty < sc_tree_size.y; ++ty) {
                        for (int tx = 0; tx < sc_tree_size.x; ++tx) {
                            const size_t map_index = (y + ty) * m_size.x + (x + tx);
                            if (const size_t tree_index = ty * sc_tree_size.x + tx;
                                occupied[map_index] != static_cast<bool>(sc_tree[tree_index])) {
                                goto not_found;
                            }
                        }
                    }
                    return seconds;
                not_found:;
                }
            }
            ++seconds;
        }
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

    void calculate_occupied(const int seconds, std::vector<bool>& occupied) const
    {
        occupied.clear();
        occupied.resize(m_size.x * m_size.y, false);
        for (const auto& [pos, vel] : m_robots) {
            Vector2i new_pos = pos + vel * seconds;
            new_pos %= m_size;
            new_pos.x = new_pos.x < 0 ? m_size.x + new_pos.x : new_pos.x;
            new_pos.y = new_pos.y < 0 ? m_size.y + new_pos.y : new_pos.y;
            const size_t index = new_pos.y * m_size.x + new_pos.x;
            occupied[index] = true;
        }
    }

    static constexpr Vector2i sc_tree_size { 31, 33 };
    // clang-format off
    static constexpr std::array<char, sc_tree_size.x * sc_tree_size.y> sc_tree = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1,
	1,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,
	1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
    };
    // clang-format on

    std::vector<Robot> m_robots;
    Vector2i m_size;
};

static uint64_t solve(const std::string& data, const Vector2i& map_size)
{
    const Map map = Map::parse(data, map_size);
    // map.print(7037);
    // map.output_images(10000);
    return map.find_tree();
}

int main()
{
    const std::string data = read_data("./day14-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100;
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