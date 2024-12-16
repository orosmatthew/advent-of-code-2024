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

enum Dir { dir_north = 1, dir_east = 2, dir_south = 4, dir_west = 8 };

[[maybe_unused]] std::string dir_str(const Dir dir)
{
    switch (dir) {
    case dir_north:
        return "^";
    case dir_east:
        return ">";
    case dir_south:
        return "v";
    case dir_west:
        return "<";
    }
    std::unreachable();
}

static Vector2i dir_offset(const Dir dir)
{
    switch (dir) {
    case dir_north:
        return { 0, -1 };
    case dir_east:
        return { 1, 0 };
    case dir_south:
        return { 0, 1 };
    case dir_west:
        return { -1, 0 };
    }
    std::unreachable();
}

static Dir opposite_dir(const Dir dir)
{
    switch (dir) {
    case dir_north:
        return dir_south;
    case dir_east:
        return dir_west;
    case dir_south:
        return dir_north;
    case dir_west:
        return dir_east;
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
        const std::vector<DijkstraState> grid = dijkstra_final_state();
        return best_paths_grid_count(grid);
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
        uint8_t dirs;
    };

    [[maybe_unused]] void print_dijkstra(const std::vector<DijkstraState>& grid) const
    {
        for (int y = 0; y < m_size.y; ++y) {
            for (int x = 0; x < m_size.x; ++x) {
                if (const size_t i = index({ x, y }); m_walls[i]) {
                    std::cout << "#";
                }
                else if (grid[i].dirs) {
                    std::cout << static_cast<int>(grid[i].dirs);
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
        DijkstraState* next_state = nullptr;
        uint64_t min_score = std::numeric_limits<uint64_t>::max();
        size_t next_queue_index = 0;
        for (size_t i = 0; i < queue.size(); ++i) {
            if (!queue[i]->explored && queue[i]->min_score <= min_score) {
                next_state = queue[i];
                min_score = queue[i]->min_score;
                next_queue_index = i;
            }
        }
        if (next_state == nullptr) {
            return false;
        }
        for (constexpr std::array dirs { dir_north, dir_east, dir_south, dir_west }; const Dir dir : dirs) {
            const Vector2i neighbor_pos = next_state->pos + dir_offset(dir);
            const size_t neighbor_index = index(neighbor_pos);
            // ReSharper disable once CppUseStructuredBinding
            DijkstraState& neighbor_state = grid[neighbor_index];
            if (m_walls[neighbor_index] || neighbor_state.explored) {
                continue;
            }
            if (const uint64_t neighbor_score = next_state->min_score + (next_state->dirs & dir ? 1 : 1001);
                neighbor_score <= neighbor_state.min_score) {
                if (neighbor_score == neighbor_state.min_score) {
                    neighbor_state.dirs |= dir;
                }
                neighbor_state.dirs = dir;
                neighbor_state.min_score = neighbor_score;
            }
        }
        next_state->explored = true;
        queue.erase(queue.begin() + static_cast<int64_t>(next_queue_index));
        return true;
    }

    [[nodiscard]] std::vector<DijkstraState> dijkstra_final_state() const
    {
        std::vector<DijkstraState> grid;
        grid.reserve(m_size.x * m_size.y);
        for (int y = 0; y < m_size.y; ++y) {
            for (int x = 0; x < m_size.x; ++x) {
                grid.push_back(
                    { .pos = { x, y },
                      .explored = false,
                      .min_score = std::numeric_limits<uint64_t>::max(),
                      .dirs = 0 });
            }
        }
        grid[index(m_start_pos)] = { .pos = m_start_pos, .explored = false, .min_score = 0, .dirs = dir_east };
        std::vector<DijkstraState*> queue;
        for (size_t i = 0; i < m_size.x * m_size.y; ++i) {
            if (!m_walls[i]) {
                queue.push_back(&grid[i]);
            }
        }
        while (dijkstra_impl(grid, queue)) { }
        return grid;
    }

    [[nodiscard]] uint64_t best_paths_grid_count(const std::vector<DijkstraState>& grid) const
    {
        print_dijkstra(grid);
        std::vector<bool> visited;
        visited.resize(m_size.x * m_size.y, false);
        std::vector<Vector2i> queue;
        queue.push_back(m_end_pos);
        uint64_t count = 0;
        std::vector<Vector2i> best_positions;
        while (!queue.empty()) {
            ++count;
            const Vector2i pos = queue[queue.size() - 1];
            best_positions.push_back(pos);
            visited[index(pos)] = true;
            queue.pop_back();
            constexpr std::array dirs { dir_north, dir_east, dir_south, dir_west };
            for (int i = 0; i < 4; ++i) {
                const Vector2i neighbor_pos = pos + dir_offset(dirs[i]);
                const size_t neighbor_index = index(neighbor_pos);
                if (m_walls[neighbor_index] || visited[neighbor_index]) {
                    continue;
                }
                if (grid[neighbor_index].dirs & opposite_dir(dirs[i])) {
                    queue.push_back(neighbor_pos);
                }
            }
        }
        ++count;
        for (int y = 0; y < m_size.y; ++y) {
            for (int x = 0; x < m_size.x; ++x) {
                if (const size_t i = index({ x, y }); m_walls[i]) {
                    std::cout << "#";
                }
                else if (std::ranges::find(best_positions, Vector2i { x, y }) != best_positions.end()) {
                    std::cout << "O";
                }
                else {
                    std::cout << ".";
                }
            }
            std::cout << std::endl;
        }
        return count;
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
    const std::string data = read_data("./day16-part2/sample1.txt");

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