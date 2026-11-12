#pragma once

#include <array>
#include <cstdint>

class ComboIndexer
{
public:
    static constexpr uint16_t N_CARDS = 52;
    static constexpr uint16_t N_COMBOS = 1326;

    struct Combo
    {
        uint64_t mask;
        uint8_t card1;
        uint8_t card2;
    };

private:

    // card1, card2 -> combo ID
    std::array<std::array<uint16_t, N_CARDS>, N_CARDS> combo_to_id{};

    // combo ID -> combo information
    std::array<Combo, N_COMBOS> id_to_combo{};

public:
    ComboIndexer()
    {
        uint16_t id = 0;

        for (uint8_t c1 = 0; c1 < N_CARDS; ++c1)
        {
            for (uint8_t c2 = c1 + 1; c2 < N_CARDS; ++c2)
            {
                uint64_t mask = (uint64_t(1) << c1) | (uint64_t(1) << c2);

                combo_to_id[c1][c2] = id;
                combo_to_id[c2][c1] = id;

                id_to_combo[id] = {mask, c1, c2};

                ++id;
            }
        }
    }

    uint16_t get_id(uint8_t c1, uint8_t c2) const
    {
        return combo_to_id[c1][c2];
    }

    const Combo& get_combo(uint16_t id) const
    {
        return id_to_combo[id];
    }
};