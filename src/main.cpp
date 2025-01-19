#include <iostream>
#include <ranges>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <format>
#include "hash_combine.hpp"

constexpr auto ROWS = 2;
constexpr auto COLUMNS = 2;

struct Point
{
    uint32_t row;
    uint32_t column;

    Point(uint32_t row, uint32_t column) : row(row), column(column) {}
    Point() {}
};

std::ostream &operator<<(std::ostream &os, const Point &point)
{
    return os << std::format("({}, {})", point.row, point.column);
}

enum class SideState
{
    Opponent,
    Player,
    Empty
};

enum class Player
{
    Opponent,
    Player
};

SideState player_to_side_state(Player player)
{
    return player == Player::Opponent ? SideState::Opponent : SideState::Player;
}

Player opposite(Player player)
{
    return player == Player::Opponent ? Player::Player : Player::Opponent;
}

std::ostream &operator<<(std::ostream &os, const SideState &state)
{
    return os << (state == SideState::Opponent ? "Opponent" : state == SideState::Player ? "Player"
                                                                                         : "Empty");
}

struct Square
{
    Point top_left_corner;
    SideState top = SideState::Empty;
    SideState left = SideState::Empty;
    SideState right = SideState::Empty;
    SideState bottom = SideState::Empty;
    std::optional<Player> filled_by;
    std::optional<std::reference_wrapper<Square>> top_neighbor;
    std::optional<std::reference_wrapper<Square>> left_neighbor;
    std::optional<std::reference_wrapper<Square>> right_neighbor;
    std::optional<std::reference_wrapper<Square>> bottom_neighbor;

    Square() {}

    void fill(SideState &side, SideState state)
    {
        side = state;
        if (top != SideState::Empty && left != SideState::Empty && right != SideState::Empty && bottom != SideState::Empty)
        {
            filled_by = state == SideState::Player ? Player::Player : Player::Opponent;
        }
        else
        {
            filled_by = std::nullopt;
        }
    }

    void set_top(SideState player, bool update_neighbors)
    {
        fill(top, player);

        if (top_neighbor.has_value() && update_neighbors)
        {
            top_neighbor.value().get().set_bottom(player, false);
        }
    }

    void set_left(SideState player, bool update_neighbors)
    {
        fill(left, player);

        if (left_neighbor.has_value() && update_neighbors)
        {
            left_neighbor.value().get().set_right(player, false);
        }
    }

    void set_right(SideState player, bool update_neighbors)
    {
        fill(right, player);

        if (right_neighbor.has_value() && update_neighbors)
        {
            right_neighbor.value().get().set_left(player, false);
        }
    }

    void set_bottom(SideState player, bool update_neighbors)
    {
        fill(bottom, player);

        if (bottom_neighbor.has_value() && update_neighbors)
        {
            bottom_neighbor.value().get().set_top(player, false);
        }
    }

    bool is_full() const
    {
        return filled_by != std::nullopt && top != SideState::Empty && left != SideState::Empty && right != SideState::Empty && bottom != SideState::Empty;
    }
};
using Board = std::array<std::array<Square, COLUMNS>, ROWS>;

std::ostream &operator<<(std::ostream &os, const Square &square)
{
    return os << "Top left: " << square.top_left_corner << square.top << "," << square.left << "," << square.right << "," << square.bottom;
}

void asdf(Board &board, Player turn, size_t &total_moves)
{
    for (const auto row : std::views::iota(0, ROWS))
    {
        for (const auto column : std::views::iota(0, COLUMNS))
        {
            auto& square = board[row][column];

            if (square.is_full())
                continue;

            if (square.top == SideState::Empty)
            {
                square.set_top(player_to_side_state(turn), true);
                asdf(board, opposite(turn), ++total_moves);
                square.set_top(SideState::Empty, true);
            }

            if (square.left == SideState::Empty)
            {
                square.set_left(player_to_side_state(turn), true);
                asdf(board, opposite(turn), ++total_moves);
                square.set_left(SideState::Empty, true);
            }

            if (square.right == SideState::Empty)
            {
                square.set_right(player_to_side_state(turn), true);
                asdf(board, opposite(turn), ++total_moves);
                square.set_right(SideState::Empty, true);
            }

            if (square.bottom == SideState::Empty)
            {
                square.set_bottom(player_to_side_state(turn), true);
                asdf(board, opposite(turn), ++total_moves);
                square.set_bottom(SideState::Empty, true);
            }
        }
    }
}

int main()
{
    Board board{};

    for (const auto row : std::views::iota(0, ROWS))
    {
        for (const auto column : std::views::iota(0, COLUMNS))
        {
            board[row][column] = Square();
            board[row][column].top_left_corner = Point(row, column);
        }
    }

    for (const auto row : std::views::iota(0, ROWS))
    {
        for (const auto column : std::views::iota(0, COLUMNS))
        {
            if (row > 0)
            {
                board[row][column].top_neighbor = board[row - 1][column];
            }

            if (column > 0)
            {
                board[row][column].left_neighbor = board[row][column - 1];
            }

            if (column < COLUMNS - 1)
            {
                board[row][column].right_neighbor = board[row][column + 1];
            }

            if (row < ROWS - 1)
            {
                board[row][column].bottom_neighbor = board[row + 1][column];
            }
        }
    }

    size_t total_moves = 0;
    asdf(board, Player::Player, total_moves);
    std::cout << total_moves << std::endl;
}