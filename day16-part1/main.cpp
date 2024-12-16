#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
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

enum class Dir { north, east, south, west };

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

class Maze {
public:
    static Maze parse(const std::string& data)
    {
        std::vector<bool> walls;
        std::optional<int> width;
        std::optional<Vector2i> start_pos;
        std::optional<Vector2i> end_pos;
        int y = 0;
        for (int i = 0; i < data.size(); ++i) {
            if (data[i] == '\n') {
                if (!width.has_value()) {
                    width = i;
                }
                ++y;
                continue;
            }
            switch (data[i]) {
            case 'S':
                assert(!start_pos.has_value());
                start_pos = { i % (width.value() + 1), y };
                walls.push_back(false);
                break;
            case 'E':
                assert(!end_pos.has_value());
                end_pos = { i % (width.value() + 1), y };
                walls.push_back(false);
                break;
            case '.':
                walls.push_back(false);
                break;
            case '#':
                walls.push_back(true);
                break;
            default:
                assert(false); // Invalid char
            }
        }
        return { std::move(walls), { width.value(), y }, start_pos.value(), end_pos.value() };
    }

    [[nodiscard]] uint64_t solve_min_points() const
    {
        return dijkstra_min_score(m_start_pos, m_end_pos);
    }

private:
    Maze(std::vector<bool>&& walls, const Vector2i& size, const Vector2i& start_pos, const Vector2i& end_pos)
        : m_walls { std::move(walls) }
        , m_size { size }
        , m_start_pos { start_pos }
        , m_end_pos { end_pos }
    {
    }

    [[nodiscard]] size_t index(const Vector2i& pos) const
    {
        return pos.y * m_size.x + pos.x;
    }

    struct DijkstraState {
        Vector2i pos;
        bool explored;
        uint64_t min_score;
        std::optional<Dir> dir;
    };

    [[maybe_unused]] void print_dijkstra(const std::vector<DijkstraState>& grid) const
    {
        for (int y = 0; y < m_size.y; ++y) {
            for (int x = 0; x < m_size.x; ++x) {
                if (const size_t i = index({ x, y }); m_walls[i]) {
                    std::cout << "#";
                }
                else if (grid[i].dir.has_value()) {
                    std::cout << dir_str(grid[i].dir.value());
                }
                else {
                    std::cout << ".";
                }
            }
            std::cout << std::endl;
        }
    }

    bool dijkstra_impl(std::vector<DijkstraState>& grid, std::vector<DijkstraState*>& queue) const
    {
        if (queue.empty()) {
            return false;
        }
        std::ranges::sort(queue, [](const DijkstraState* a, const DijkstraState* b) {
            return b->min_score < a->min_score;
        });
        // ReSharper disable once CppUseStructuredBinding
        DijkstraState& next_state = *queue[queue.size() - 1];
        for (constexpr std::array dirs { Dir::north, Dir::east, Dir::south, Dir::west }; const Dir dir : dirs) {
            const Vector2i neighbor_pos = next_state.pos + dir_offset(dir);
            const size_t neighbor_index = index(neighbor_pos);
            // ReSharper disable once CppUseStructuredBinding
            DijkstraState& neighbor_state = grid[neighbor_index];
            if (m_walls[neighbor_index] || neighbor_state.explored) {
                continue;
            }
            if (const uint64_t neighbor_score = next_state.min_score + (dir == next_state.dir ? 1 : 1001);
                neighbor_score < neighbor_state.min_score) {
                neighbor_state.min_score = neighbor_score;
                neighbor_state.dir = dir;
            }
        }
        next_state.explored = true;
        queue.pop_back();
        return true;
    }

    [[nodiscard]] uint64_t dijkstra_min_score(const Vector2i& start, const Vector2i& end) const
    {
        std::vector<DijkstraState> grid;
        grid.reserve(m_size.x * m_size.y);
        for (int y = 0; y < m_size.y; ++y) {
            for (int x = 0; x < m_size.x; ++x) {
                grid.push_back(
                    { .pos = { x, y },
                      .explored = false,
                      .min_score = std::numeric_limits<uint64_t>::max(),
                      .dir = std::nullopt });
            }
        }
        grid[index(start)] = { .pos = start, .explored = false, .min_score = 0, .dir = Dir::east };
        std::vector<DijkstraState*> queue;
        for (size_t i = 0; i < m_size.x * m_size.y; ++i) {
            if (!m_walls[i]) {
                queue.push_back(&grid[i]);
            }
        }
        while (dijkstra_impl(grid, queue)) { }
        return grid[index(end)].min_score;
    }

    std::vector<bool> m_walls;
    Vector2i m_size;
    Vector2i m_start_pos;
    Vector2i m_end_pos;
};

static uint64_t solve(const std::string& data)
{
    const Maze maze = Maze::parse(data);
    return maze.solve_min_points();
}

int main()
{
    const std::string data = read_data("./day16-part1/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100;
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