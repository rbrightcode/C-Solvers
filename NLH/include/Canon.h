#pragma once

#include <array>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <bit>
#include "Combos.h"
#include <vector>
#include <fstream>
#include <stdexcept>
#include <functional>
#include <string>

class Canon
{
public:
    using Card = uint8_t;
    using ComboId = uint16_t;
    using Suit = uint8_t;

    static constexpr uint8_t N_SUITS = 4;
    static constexpr uint8_t N_PERMS = 24;
    static constexpr uint8_t N_CARDS = 52;
    ComboIndexer Combos;

    struct SuitPermutation
    {
        std::array<Suit, 4> map;
    };

    std::array<SuitPermutation, N_PERMS> perms;
    std::array<std::array<Card, N_CARDS>, N_PERMS> card_map;
    std::array<std::array<ComboId, ComboIndexer::N_COMBOS>, N_PERMS> combo_map;

    // Index 0 = flop (3 cards)
    // Index 1 = turn (4 cards)
    // Index 2 = river (5 cards)
    std::array<std::vector<uint8_t>, 3> perm_lookup;

    Canon()
    {
        build_permutations();
        build_card_maps();
        build_combo_maps();
    }

    uint64_t get_canon_mask(uint64_t mask, uint8_t perm_id) const
    {
        assert(perm_id < N_PERMS);

        uint64_t canon_mask = 0;

        while (mask)
        {
            uint8_t card = static_cast<uint8_t>(std::countr_zero(mask));

            mask &= mask - 1;

            Card canon_card = card_map[perm_id][card];

            canon_mask |= uint64_t(1) << canon_card;
        }

        return canon_mask;
    }

    std::array<Card, 5> get_canon_board(const std::array<Card, 5>& board, uint8_t perm_id, uint8_t count) const
    { 
        std::array<Card, 5> canon_board = board;
        for (size_t i = 0; i < count; ++i)
        { 
            canon_board[i] = card_map[perm_id][board[i]];
        } 
        return canon_board;
    }

    // C(n, k)
    static uint32_t choose(uint32_t n, uint32_t k)
    {
        if (k > n)
            return 0;

        if (k == 0)
            return 1;

        uint32_t result = 1;

        for (uint32_t i = 1; i <= k; ++i)
            result = result * (n - k + i) / i;

        return result;
    }

    // Combinatorial number system (colexicographic rank).
    // Input cards must be sorted in ascending order.
    static uint32_t board_index(const std::array<Card, 5>& board, uint8_t count)
    {
        uint32_t index = 0;

        for (uint32_t i = 0; i < count; ++i)
            index += choose(board[i], i + 1);

        return index;
    }

    // Compute the canonical permutation ID using the same
    // lexicographic minimization as the original get_perm_id().
    uint8_t calculate_perm_id(const std::array<Card, 5>& board, uint8_t count) const
    {
        std::array<Card, 5> best_board = board;
        uint8_t best_id = 0;

        for (uint8_t p = 0; p < N_PERMS; ++p)
        {
            std::array<Card, 5> candidate{};

            for (uint8_t i = 0; i < count; ++i)
                candidate[i] = card_map[p][board[i]];

            std::sort(candidate.begin(), candidate.begin() + count);

            if (std::lexicographical_compare(candidate.begin(), candidate.begin() + count, best_board.begin(), best_board.begin() + count))
            {
                best_board = candidate;
                best_id = p;
            }
        }

        return best_id;
    }

    // Generate every k-card board and store its permutation ID.
    void precompute_perm_ids()
    {
        for (uint8_t k = 3; k <= 5; ++k)
        {
            auto& table = perm_lookup[k - 3];

            table.resize(choose(52, k));

            std::array<Card, 5> board{};

            auto generate = [&](auto&& self, uint8_t depth,
                uint8_t start) -> void
                {
                    if (depth == k)
                    {
                        const uint32_t index = board_index(board, k);

                        table[index] = calculate_perm_id(board, k);
                        return;
                    }

                    for (uint8_t card = start; card <= 52 - (k - depth); ++card)
                    {
                        board[depth] = card;
                        self(self, depth + 1, card + 1);
                    }
                };

            generate(generate, 0, 0);
        }
    }

    // Fast runtime lookup.
    // Replaces the original 24-permutation get_perm_id().
    uint8_t get_perm_id(std::array<Card, 5> board, uint8_t count) const
    {
        if (count < 3 || count > 5)
            throw std::runtime_error("Invalid board size for perm lookup");

        std::sort(board.begin(), board.begin() + count);

        const uint32_t index = board_index(board, count);

        const auto& table = perm_lookup[count - 3];

        if (table.empty() || index >= table.size())
            throw std::runtime_error("Permutation lookup not loaded");

        return table[index];
    }

    void save_perm_ids(const std::string& filename) const
    {
        std::ofstream file(filename, std::ios::binary);

        if (!file)
            throw std::runtime_error("Could not open permutation file");

        const uint32_t magic = 0x5045524D; // "PERM"
        const uint32_t version = 1;

        file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));

        for (const auto& table : perm_lookup)
        {
            const uint32_t size = static_cast<uint32_t>(table.size());

            file.write(reinterpret_cast<const char*>(&size), sizeof(size));
            file.write(reinterpret_cast<const char*>(table.data()), size * sizeof(uint8_t));
        }

        if (!file)
            throw std::runtime_error("Error writing permutation file");
    }

    void load_perm_ids(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::binary);

        if (!file)
            throw std::runtime_error("Could not open permutation file");

        uint32_t magic = 0;
        uint32_t version = 0;

        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        file.read(reinterpret_cast<char*>(&version), sizeof(version));

        if (magic != 0x5045524D || version != 1)
            throw std::runtime_error("Invalid permutation file");

        for (uint8_t k = 3; k <= 5; ++k)
        {
            uint32_t size = 0;

            file.read(reinterpret_cast<char*>(&size), sizeof(size));

            const uint32_t expected = choose(52, k);

            if (!file || size != expected)
                throw std::runtime_error("Invalid permutation table size");

            auto& table = perm_lookup[k - 3];
            table.resize(size);

            file.read(reinterpret_cast<char*>(table.data()), size * sizeof(uint8_t));

            if (!file)
                throw std::runtime_error("Error reading permutation table");
        }
    }

private:

    void build_permutations()
    {
        std::array<Suit, 4> p = { 0, 1, 2, 3 };

        uint8_t i = 0;

        do
        {
            perms[i++].map = p;
        } while (std::next_permutation(p.begin(), p.end()));

        assert(i == N_PERMS);
    }

    void build_card_maps()
    {
        for (uint8_t p = 0; p < N_PERMS; ++p)
        {
            for (Card card = 0; card < N_CARDS; ++card)
            {
                uint8_t rank = card / 4;
                uint8_t suit = card % 4;

                uint8_t new_suit = perms[p].map[suit];

                card_map[p][card] = rank * 4 + new_suit;
            }
        }
    }

    void build_combo_maps()
    {
        for (uint8_t p = 0; p < N_PERMS; ++p)
        {
            for (ComboId combo = 0; combo < ComboIndexer::N_COMBOS; ++combo)
            {
                const auto& c = Combos.get_combo(combo);

                uint8_t c1 = card_map[p][c.card1];
                uint8_t c2 = card_map[p][c.card2];

                combo_map[p][combo] = Combos.get_id(c1, c2);
            }
        }
    }
};