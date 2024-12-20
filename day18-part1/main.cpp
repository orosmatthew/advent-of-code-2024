#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <ranges>
#include <span>
#include <sstream>
#include <utility>
#include <variant>
#include <vector>

static std::string read_data(const std::filesystem::path& path)
{
    const std::fstream file { path };
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static bool is_digit(const char c)
{
    return c >= '0' && c <= '9';
}

static int64_t parse_int(const std::string& string, int& pos)
{
    int64_t result = 0;
    do {
        result = result * 10 + (string[pos] - '0');
        ++pos;
    } while (is_digit(string[pos]) && pos < string.length());
    return result;
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

enum class Dir { north, east, south, west };
constexpr std::array dirs { Dir::north, Dir::east, Dir::south, Dir::west };

[[maybe_unused]] std::string dir_str(const Dir dir)
{
    switch (dir) {
    case Dir::north:
        return "^";
    case Dir::east:
        return ">";
    case Dir::south:
        return "v";
    case Dir::west:
        return "<";
    }
    std::unreachable();
}

static Vector2i dir_offset(const Dir dir)
{
    switch (dir) {
    case Dir::north:
        return { 0, -1 };
    case Dir::east:
        return { 1, 0 };
    case Dir::south:
        return { 0, 1 };
    case Dir::west:
        return { -1, 0 };
    }
    std::unreachable();
}

class Map {
public:
    static Map parse(const std::string& data, const Vector2i& map_size, const int64_t bytes_fallen)
    {
        std::vector<bool> walls;
        walls.resize(map_size.x * map_size.y, false);
        int pos = 0;
        int bytes_count = 0;
        while (bytes_count < bytes_fallen) {
            const int64_t x = parse_int(data, pos);
            ++pos; // ","
            const int64_t y = parse_int(data, pos);
            ++pos; // "\n"
            const size_t index = y * map_size.x + x;
            walls[index] = true;
            ++bytes_count;
        }
        return { std::move(walls), map_size };
    }

    uint64_t steps_to_exit()
    {
    }

private:
    Map(std::vector<bool> walls, const Vector2i& map_size)
        : m_walls { std::move(walls) }
        , m_size { map_size }
    {
    }

    [[nodiscard]] size_t index(const Vector2i& pos) const
    {
        return pos.y * m_size.x + pos.x;
    }

    [[nodiscard]] bool in_bounds(const Vector2i& pos) const
    {
        return pos.x >= 0 && pos.x < m_size.x && pos.y >= 0 && pos.y < m_size.y;
    }

    struct DijkstraState {
        Vector2i pos;
        bool explored;
        uint64_t score;
        DijkstraState* prev_state;
    };

    struct DijkstraStateCmp {
        bool operator()(const DijkstraState* a, const DijkstraState* b) const noexcept
        {
            return a->score > b->score;
        }
    };

    using DijkstraQueue = std::priority_queue<DijkstraState*, std::vector<DijkstraState*>, DijkstraStateCmp>;

    bool dijkstra_impl(std::vector<DijkstraState>& grid, DijkstraQueue& queue) const
    {
        if (queue.empty()) {
            return false;
        }
        DijkstraState* current_state = queue.top();
        queue.pop();
        for (const Dir dir : dirs) {
            const Vector2i neighbor_pos = current_state->pos + dir_offset(dir);
            if (!in_bounds(neighbor_pos)) {
                continue;
            }
            const size_t neighbor_index = index(neighbor_pos);
            if (m_walls[neighbor_index]) {
                continue;
            }
            DijkstraState& neighbor_state = grid[neighbor_index];
            if (const uint64_t neighbor_score = current_state->score + 1; neighbor_score < neighbor_state.score) {
                neighbor_state.score = neighbor_score;
                neighbor_state.prev_state = current_state;
            }
            if (!neighbor_state.explored) {
                queue.push(&neighbor_state);
            }
        }
        current_state->explored = true;
        return true;
    }

    std::vector<bool> m_walls;
    Vector2i m_size;
};

// ReSharper disable once CppDFAConstantParameter
static uint64_t solve(const std::string& data, const Vector2i& map_size, const int64_t bytes_fallen)
{
    const Map map = Map::parse(data, map_size, bytes_fallen);
    return 1;
}

int main()
{
    const std::string data = read_data("./day18-part1/sample.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100000;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile auto result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%llu\n", solve(data, { 7, 7 }, 12));
#endif
}