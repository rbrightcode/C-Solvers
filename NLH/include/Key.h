#pragma once

#include <iostream>

enum class Actions : uint8_t
{
    Fold = 0,
    Check,
    Call,
    B33,
    B50,
    B67,
    B100,
    B125,
    Jam
};

struct Key
{
    uint16_t bucket;
    uint64_t history;

    bool operator==(const Key& other) const
    {
        return bucket == other.bucket &&
            history == other.history;
    }
};

struct Hash
{
    static inline uint64_t mix64(uint64_t x) noexcept
    {
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ULL;

        x ^= x >> 27;
        x *= 0x94d049bb133111ebULL;

        x ^= x >> 31;

        return x;
    }

    inline size_t operator()(const Key& key) const noexcept
    {
        uint64_t x = key.history ^ (static_cast<uint64_t>(key.bucket) << 48);

        return static_cast<size_t>(mix64(x));
    }
};

struct History
{
    uint64_t data = 0;
    uint64_t length = 0;

    static constexpr uint8_t MAX_LENGTH = 20;

    void push(Actions action)
    {
        assert(length < MAX_LENGTH);

        data = data * 9 + static_cast<uint64_t>(action);
        ++length;
    }

    inline Actions back() const
    {
        assert(length > 0);

        return static_cast<Actions>(data % 9);
    }

    inline Actions get(uint8_t index) const
    {
        assert(index < length);

        const uint8_t shift = length - 1 - index;

        uint64_t divisor = 1;

        for (uint8_t i = 0; i < shift; ++i)
            divisor *= 9;

        return static_cast<Actions>((data / divisor) % 9);
    }
};

struct LegalActions
{
    std::array<uint8_t, Node::MAX_ACTIONS> actions;
    uint8_t count = 0;

    inline void add(Actions action)
    {
        actions[count++] = static_cast<uint8_t>(action);
    }
};

struct State
{
    uint64_t board_mask = 0;
    uint64_t canon_mask = 0;
    uint8_t perm_id = 0;
    uint8_t flop_id = 0;
    uint8_t turn_id = 0;
    uint8_t river_id = 0;

    std::array<uint16_t, 2> combos{};

    std::array<uint8_t, 5> board{};
    uint8_t board_count = 0;

    History history = { 0, 0 };

    uint8_t player = 1;

    std::array<uint32_t, 2> contributions = { 0,0 };
    std::array<uint32_t, 2> stacks = { 198, 199 };

    uint32_t last_raise = 1;
    uint8_t round_raises = 0;
    uint8_t round_start = 0;
};