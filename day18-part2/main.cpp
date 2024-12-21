#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <set>
#include <sstream>
#include <unordered_set>
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

template <>
struct std::hash<Vector2i> {
    size_t operator()(const Vector2i& vector) const noexcept
    {
        const size_t hash1 = std::hash<int64_t>()(vector.x);
        const size_t hash2 = std::hash<int64_t>()(vector.y);
        return hash1 ^ hash2 << 1;
    }
};

enum class Dir { north, east, south, west };
constexpr std::array dirs { Dir::north, Dir::east, Dir::south, Dir::west };

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
    static Map parse(const std::string& data, const Vector2i& map_size)
    {
        std::vector<Vector2i> walls;
        int pos = 0;
        while (pos < data.size()) {
            const int64_t x = parse_int(data, pos);
            ++pos; // ","
            const int64_t y = parse_int(data, pos);
            ++pos; // "\n"
            walls.push_back({ x, y });
        }
        return { std::move(walls), map_size };
    }

    [[nodiscard]] std::optional<Vector2i> coordinates_to_block_path()
    {
        std::optional<std::unordered_set<Vector2i>> prev_path_positions;
        for (const Vector2i wall_pos : m_wall_positions) {
            m_walls[index(wall_pos)] = true;
            if (prev_path_positions.has_value() && !prev_path_positions.value().contains(wall_pos)) {
                continue;
            }
            prev_path_positions = dijkstra_path_positions();
            if (!prev_path_positions.has_value()) {
                return wall_pos;
            }
        }
        return std::nullopt;
    }

private:
    Map(std::vector<Vector2i> wall_positions, const Vector2i& map_size)
        : m_wall_positions { std::move(wall_positions) }
        , m_size { map_size }
    {
        m_walls.resize(m_size.x * m_size.y, false);
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
            if (a->score != b->score) {
                return a->score < b->score;
            }
            return a < b;
        }
    };

    // using DijkstraQueue = std::priority_queue<DijkstraState*, std::vector<DijkstraState*>, DijkstraStateCmp>;
    using DijkstraQueue = std::set<DijkstraState*, DijkstraStateCmp>;

    void dijkstra_step(std::vector<DijkstraState>& grid, DijkstraQueue& queue) const
    {
        if (queue.empty()) {
            return;
        }
        DijkstraState* current_state = *queue.begin();
        queue.erase(queue.begin());
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
            if (!neighbor_state.explored && !m_walls[neighbor_index]) {
                queue.insert(&neighbor_state);
            }
        }
        current_state->explored = true;
    }

    void run_dijkstra()
    {
        m_d_grid.clear();
        m_d_grid.reserve(m_size.x * m_size.y);
        for (int y = 0; y < m_size.y; ++y) {
            for (int x = 0; x < m_size.x; ++x) {
                m_d_grid.push_back(
                    { .pos = { x, y },
                      .explored = false,
                      .score = std::numeric_limits<uint64_t>::max(),
                      .prev_state = nullptr });
            }
        }
        DijkstraState& start_state = m_d_grid[index({ 0, 0 })];
        start_state.score = 0;
        DijkstraQueue queue;
        queue.insert(&start_state);
        while (!queue.empty()) {
            dijkstra_step(m_d_grid, queue);
        }
    }

    [[nodiscard]] std::optional<std::unordered_set<Vector2i>> dijkstra_path_positions()
    {
        run_dijkstra();
        const DijkstraState* state = &m_d_grid[index({ m_size.x - 1, m_size.y - 1 })];
        if (state->prev_state == nullptr) {
            return std::nullopt;
        }
        std::unordered_set<Vector2i> positions;
        while (state != nullptr) {
            positions.insert(state->pos);
            state = state->prev_state;
        }
        return positions;
    }

    std::vector<bool> m_walls;
    std::vector<Vector2i> m_wall_positions;
    Vector2i m_size;
    std::vector<DijkstraState> m_d_grid;
};

// ReSharper disable once CppDFAConstantParameter
static std::string solve(const std::string& data, const Vector2i& map_size)
{
    Map map = Map::parse(data, map_size);
    const auto [x, y] = map.coordinates_to_block_path().value();
    std::stringstream ss;
    ss << x << "," << y;
    return ss.str();
}

int main()
{
    const std::string data = read_data("./day18-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile auto result = solve(data, { 71, 71 });
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%s\n", solve(data, { 71, 71 }).c_str());
#endif
}