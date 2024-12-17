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
#include <sstream>
#include <unordered_set>
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

template <>
struct std::hash<Vector2i> {
    size_t operator()(const Vector2i& vector) const noexcept
    {
        const size_t hash1 = std::hash<int64_t>()(vector.x);
        const size_t hash2 = std::hash<int64_t>()(vector.y);
        return hash1 ^ hash2 << 1;
    }
};

enum class Dir { north = 0, east = 1, south = 2, west = 3 };
constexpr std::array dirs { Dir::north, Dir::east, Dir::south, Dir::west };

static int dir_index(const Dir dir)
{
    return static_cast<int>(dir);
}

[[maybe_unused]] static std::string dir_str(const Dir dir)
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

static std::array<Dir, 2> rotate_dirs(const Dir dir)
{
    switch (dir) {
    case Dir::north:
        return { Dir::west, Dir::east };
    case Dir::east:
        return { Dir::north, Dir::south };
    case Dir::south:
        return { Dir::east, Dir::west };
    case Dir::west:
        return { Dir::south, Dir::north };
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

    [[nodiscard]] uint64_t best_tiles_count() const
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

    [[nodiscard]] size_t d_index(const Vector2i& pos, const Dir dir) const
    {
        return dir_index(dir) * m_size.x * m_size.y + pos.y * m_size.x + pos.x;
    }

    struct DijkstraState {
        Vector2i pos;
        Dir dir;
        bool explored;
        uint64_t min_score;
        std::array<DijkstraState*, 6> prev_states;
    };

    struct DijkstraStateCmp {
        bool operator()(const DijkstraState* a, const DijkstraState* b) const
        {
            return a->min_score > b->min_score;
        }
    };

    using DijkstraQueue = std::priority_queue<DijkstraState*, std::vector<DijkstraState*>, DijkstraStateCmp>;

    bool dijkstra_impl(std::vector<DijkstraState>& grid, DijkstraQueue& queue) const
    {
        if (queue.empty()) {
            return false;
        }
        DijkstraState* next_state = queue.top();
        queue.pop();
        auto update_score
            = [next_state](DijkstraState& neighbor_state, const uint64_t neighbor_score, const int prev_state_index) {
                  if (neighbor_score < neighbor_state.min_score) {
                      neighbor_state.min_score = neighbor_score;
                      neighbor_state.prev_states = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
                  }
                  if (neighbor_score <= neighbor_state.min_score) {
                      neighbor_state.prev_states[prev_state_index] = next_state;
                  }
              };
        const std::array<Dir, 2> r_dirs = rotate_dirs(next_state->dir);
        for (int i = 0; i < 2; ++i) {
            DijkstraState& neighbor_state = grid[d_index(next_state->pos, r_dirs[i])];
            const uint64_t neighbor_score = next_state->min_score + 1000;
            update_score(neighbor_state, neighbor_score, i + 4);
            if (!neighbor_state.explored) {
                queue.push(&neighbor_state);
            }
        }
        const Vector2i neighbor_pos = next_state->pos + dir_offset(next_state->dir);
        DijkstraState& neighbor_state = grid[d_index(neighbor_pos, next_state->dir)];
        const uint64_t neighbor_score = next_state->min_score + 1;
        if (!m_walls[index(neighbor_pos)]) {
            update_score(neighbor_state, neighbor_score, dir_index(next_state->dir));
            if (!neighbor_state.explored) {
                queue.push(&neighbor_state);
            }
        }
        next_state->explored = true;
        return true;
    }

    [[nodiscard]] std::vector<DijkstraState> dijkstra_final_state() const
    {
        std::vector<DijkstraState> grid;
        grid.reserve(m_size.x * m_size.y * 4);
        for (const Dir dir : dirs) {
            for (int y = 0; y < m_size.y; ++y) {
                for (int x = 0; x < m_size.x; ++x) {
                    grid.push_back(
                        { .pos = { x, y },
                          .dir = dir,
                          .explored = false,
                          .min_score = std::numeric_limits<uint64_t>::max(),
                          .prev_states = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } });
                }
            }
        }
        DijkstraState& start_state = grid[d_index(m_start_pos, Dir::east)];
        start_state.min_score = 0;
        DijkstraQueue queue;
        queue.push(&start_state);
        while (dijkstra_impl(grid, queue)) { }
        return grid;
    }

    [[nodiscard]] uint64_t best_paths_grid_count(const std::vector<DijkstraState>& grid) const
    {
        uint64_t end_min_score = std::numeric_limits<uint64_t>::max();
        for (const Dir dir : dirs) {
            if (const uint64_t score = grid[d_index(m_end_pos, dir)].min_score; score < end_min_score) {
                end_min_score = score;
            }
        }
        std::vector<const DijkstraState*> queue;
        for (const Dir dir : dirs) {
            if (const DijkstraState& state = grid[d_index(m_end_pos, dir)]; state.min_score == end_min_score) {
                for (const DijkstraState* prev : state.prev_states) {
                    if (prev != nullptr) {
                        queue.push_back(prev);
                    }
                }
            }
        }
        std::unordered_set best_positions { m_end_pos, m_start_pos };
        while (!queue.empty()) {
            const DijkstraState& state = *queue[queue.size() - 1];
            best_positions.insert(state.pos);
            queue.pop_back();
            for (const DijkstraState* prev : state.prev_states) {
                if (prev != nullptr) {
                    queue.push_back(prev);
                }
            }
        }
        return best_positions.size();
    }

    std::vector<bool> m_walls;
    Vector2i m_size;
    Vector2i m_start_pos;
    Vector2i m_end_pos;
};

static uint64_t solve(const std::string& data)
{
    const Maze maze = Maze::parse(data);
    return maze.best_tiles_count();
}

int main()
{
    const std::string data = read_data("./day16-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 10;
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