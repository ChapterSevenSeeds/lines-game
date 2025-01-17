#include <iostream>
#include <ranges>
#include <unordered_set>
#include <algorithm>
#include "hash_combine.hpp"

constexpr auto ROWS = 5;
constexpr auto COLUMNS = 5;

template <typename T, size_t Size>
class OrderedTuple
{
    std::array<T, Size> items{};

    void reset()
    {
        std::sort(items.begin(), items.end());
    }

public:
    OrderedTuple() = default;

    std::array<T, Size> get_items() const
    {
        return items;
    }
    T size() const
    {
        return Size;
    }

    T get(size_t index) const
    {
        return items[index];
    }

    void set(size_t index, T &item)
    {
        items[index] = item;
        reset();
    }
};

template <typename T, size_t Size>
struct std::hash<OrderedTuple<T, Size>>
{
    std::size_t operator()(const OrderedTuple<T, Size> &s) const noexcept
    {
        size_t seed = 0;
        for (const auto &item : s.get_items())
        {
            hash_combine(seed, s);
        }
        return seed;
    }
};

struct Point
{
    uint32_t row;
    uint32_t column;

    Point(uint32_t row, uint32_t column) : row(row), column(column) {}
    Point() {}

    bool operator==(const Point &other) const = default;
    bool operator<(const Point &other) const
    {
        return row < other.row || (row == other.row && column < other.column);
    }
    bool operator<=(const Point &other) const
    {
        return *this < other || *this == other;
    }
    bool operator>(const Point &other) const
    {
        return !(*this <= other);
    }
    bool operator>=(const Point &other) const
    {
        return !(*this < other);
    }
    Point &operator=(const Point &other) = default;
};

template <>
struct std::hash<Point>
{
    std::size_t operator()(const Point &s) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, s.row, s.column);
        return seed;
    }
};

bool can_form_line(Point point1, Point point2)
{
    auto column_diff = labs(point1.column - point2.column);
    auto row_diff = labs(point1.row - point2.row);
    return (column_diff == 1 && row_diff == 0) || (column_diff == 0 && row_diff == 1);
}

struct Line
{
    OrderedTuple<Point, 2> points{};

    Line(Point point1, Point point2)
    {
        if (!can_form_line(point1, point2))
        {
            throw std::runtime_error("Nope");
        }
        points.set(0, point1);
        points.set(1, point2);
    }

    Line() {}

    bool operator==(const Line &other) const
    {
        return points.get(0) == other.points.get(0) && points.get(1) == other.points.get(1);
    }

    bool operator<(const Line &other) const
    {
        return points.get(0) < other.points.get(0) || (points.get(0) == other.points.get(0) && points.get(1) < other.points.get(1));
    }
    bool operator<=(const Line &other) const
    {
        return *this < other || *this == other;
    }
    bool operator>(const Line &other) const
    {
        return !(*this <= other);
    }
    bool operator>=(const Line &other) const
    {
        return !(*this < other);
    }
};

template <>
struct std::hash<Line>
{
    std::size_t operator()(const Line &s) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, s.points);
        return seed;
    }
};

struct Square
{
    // Top, left, right, bottom
    OrderedTuple<Line, 4> lines{};

    Square(Line line1, Line line2, Line line3, Line line4)
    {
        lines.set(0, line1);
        lines.set(1, line2);
        lines.set(2, line3);
        lines.set(3, line4);
    }

    bool operator==(const Square &other) const
    {
        for (auto i : std::views::iota(1, 4))
        {
            if (lines.get(i) == other.lines.get(i))
                continue;

            return false;
        }

        return true;
    }

    static bool possible(Line line1, Line line2, Line line3, Line line4)
    {
        OrderedTuple<Line, 4> lines;
        lines.set(0, line1);
        lines.set(1, line2);
        lines.set(2, line3);
        lines.set(3, line4);

        auto top = lines.get(0);
        auto left = lines.get(1);
        auto right = lines.get(2);
        auto bottom = lines.get(3);

        return top.points.get(0) == left.points.get(0) && top.points.get(1) == right.points.get(0) && left.points.get(1) == bottom.points.get(0) && right.points.get(1) == bottom.points.get(1);
    }
};

int main()
{
    std::unordered_set<Line> all_lines{};
    for (const auto row1 : std::views::iota(1, ROWS))
    {
        for (const auto column1 : std::views::iota(1, COLUMNS))
        {
            const Point point1(row1, column1);

            for (const auto row2 : std::views::iota(1, ROWS))
            {
                for (const auto column2 : std::views::iota(1, COLUMNS))
                {
                    const Point point2(row2, column2);

                    if (can_form_line(point1, point2))
                    {
                        all_lines.insert(Line(point1, point2));
                    }
                }
            }
        }
    }

    std::unordered_set<Square> all_squares{};
    for (const auto &line1 : all_lines)
    {
        for (const auto &line2 : all_lines)
        {
            for (const auto &line3 : all_lines)
            {
                for (const auto &line4 : all_lines)
                {
                    if (Square::possible(line1, line2, line3, line4))
                    {
                        all_squares.insert(Square(line1, line2, line3, line4));
                    }
                }
            }
        }
    }

    for (const auto &square : all_squares)
    {
        std::cout << std::format("({},{}, {},{}) - ({},{}, {},{}) - ({},{}, {},{}) - ({},{}, {},{})",
                                 square.lines.get(0).points.get(0).row,
                                 square.lines.get(0).points.get(0).column,
                                 square.lines.get(0).points.get(1).row,
                                 square.lines.get(0).points.get(1).column,
                                 square.lines.get(1).points.get(0).row,
                                 square.lines.get(1).points.get(0).column,
                                 square.lines.get(1).points.get(1).row,
                                 square.lines.get(1).points.get(1).column,
                                 square.lines.get(2).points.get(0).row,
                                 square.lines.get(2).points.get(0).column,
                                 square.lines.get(2).points.get(1).row,
                                 square.lines.get(2).points.get(1).column,
                                 square.lines.get(3).points.get(0).row,
                                 square.lines.get(3).points.get(0).column,
                                 square.lines.get(3).points.get(1).row,
                                 square.lines.get(3).points.get(1).column);
    }
}