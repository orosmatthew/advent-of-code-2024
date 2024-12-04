#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
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
};

class WordSearch {
public:
    enum class Dir { north, northeast, east, southeast, south, southwest, west, northwest };

    static WordSearch parse(const std::string& string)
    {
        std::optional<int> width;
        std::vector<char> board;
        int pos = 0;
        while (pos < string.length()) {
            while (string[pos] != '\n') {
                board.push_back(string[pos]);
                ++pos;
            }
            if (!width.has_value()) {
                width = pos;
            }
            ++pos;
        }
        return WordSearch { width.value(), std::move(board) };
    }

    [[nodiscard]] bool word_at(const std::string& word, const Vector2i& pos, const Dir dir) const
    {
        const Vector2i offset = word_dir_offset(dir);
        Vector2i current = pos;
        for (int i = 0; i < word.length(); ++i) {
            if (std::optional<char> c = at(current); !c.has_value() || word[i] != c.value()) {
                return false;
            }
            current += offset;
        }
        return true;
    }

    [[nodiscard]] bool in_bounds(const Vector2i& pos) const
    {
        return pos.x >= 0 && pos.x < m_size.x && pos.y >= 0 && pos.y < m_size.y;
    }

    [[nodiscard]] std::optional<char> at(const Vector2i& pos) const
    {
        if (!in_bounds(pos)) {
            return std::nullopt;
        }
        return m_board[index(pos)];
    }

    [[nodiscard]] Vector2i size() const
    {
        return m_size;
    }

private:
    WordSearch(const int width, std::vector<char> board)
        : m_board { std::move(board) }
        , m_size { width, static_cast<int>(m_board.size()) / width }
    {
    }

    [[nodiscard]] int index(const Vector2i& pos) const
    {
        return pos.y * m_size.x + pos.x;
    }

    static Vector2i word_dir_offset(const Dir dir)
    {
        switch (dir) {
        case Dir::north:
            return { 0, -1 };
        case Dir::northeast:
            return { 1, -1 };
        case Dir::east:
            return { 1, 0 };
        case Dir::southeast:
            return { 1, 1 };
        case Dir::south:
            return { 0, 1 };
        case Dir::southwest:
            return { -1, 1 };
        case Dir::west:
            return { -1, 0 };
        case Dir::northwest:
            return { -1, -1 };
        }
        std::unreachable();
    }

    std::vector<char> m_board;
    Vector2i m_size;
};

static int solve(const std::string& data)
{
    const WordSearch search = WordSearch::parse(data);
    const auto [width, height] = search.size();
    int count = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const bool first = search.word_at("MAS", { x, y }, WordSearch::Dir::southeast)
                || search.word_at("SAM", { x, y }, WordSearch::Dir::southeast);
            if (!first) {
                continue;
            }
            const bool second = search.word_at("MAS", { x, y + 2 }, WordSearch::Dir::northeast)
                || search.word_at("SAM", { x, y + 2 }, WordSearch::Dir::northeast);
            if (second) {
                ++count;
            }
        }
    }
    return count;
}

int main()
{
    const std::string data = read_data("./day4-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100000;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile int result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%d\n", solve(data));
#endif
}