#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include "Combos.h"
#include "Evaluator.h"
#include <fstream>
#include <stdexcept>
#include <random>
#include <limits>
#include "Canon.h"
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include <filesystem>


class KMeans
{
public:
    static constexpr int N_FEATURES = 21;
    using FeatureVector = std::array<float, N_FEATURES>;
    using Centroid = std::array<float, N_FEATURES>;
    std::vector<Centroid> centroids;
    std::vector<uint16_t> labels;


    void fit(const std::vector<FeatureVector>& data, uint16_t k, uint32_t iterations = 50)
    {
        if (data.empty())
            return;

        if (k > data.size())
            throw std::runtime_error("K cannot exceed number of samples");

        centroids.resize(k);
        labels.resize(data.size());

        initialize_centroids(data, k);

        std::cout << "Initialization done \n";

        for (uint32_t iteration = 0; iteration < iterations; ++iteration)
        {
            assign_clusters(data, k);
            update_centroids(data, k);
            std::cout << "Iteration: " << iteration << std::endl;
        }

        assign_clusters(data, k);

        double total_error = 0.0f;

        for (size_t i = 0; i < data.size(); ++i)
        {
            total_error += distance_squared(data[i], centroids[labels[i]]);
        }

        std::cout << total_error << std::endl;
    }


private:

    void initialize_centroids(const std::vector<FeatureVector>& data, uint16_t k)
    {
        std::random_device rd;
        std::mt19937 rng(rd());

        std::uniform_int_distribution<size_t> dist(0, data.size() - 1);

        for (uint16_t i = 0; i < k; ++i)
        {
            centroids[i] = data[dist(rng)];
        }
    }


    void assign_clusters(const std::vector<FeatureVector>& data, uint16_t k)
    {
        double total_error = 0;
        for (size_t i = 0; i < data.size(); ++i)
        {
            float best_distance = std::numeric_limits<float>::max();
            uint16_t best_cluster = 0;

            for (uint16_t c = 0; c < k; ++c)
            {
                float d = distance_squared(data[i], centroids[c]);

                if (d < best_distance)
                {
                    best_distance = d;
                    best_cluster = c;
                }
            }

            total_error += best_distance;
            labels[i] = best_cluster;
        }
        std::cout << total_error << std::endl;
    }


    void update_centroids(const std::vector<FeatureVector>& data, uint16_t k)
    {
        std::random_device rd;
        std::mt19937 rng(rd());

        std::uniform_int_distribution<size_t> dist(0, data.size() - 1);

        std::vector<Centroid> sums(k);
        std::vector<uint32_t> counts(k, 0);

        for (size_t i = 0; i < data.size(); ++i)
        {
            uint16_t cluster = labels[i];

            ++counts[cluster];

            for (int f = 0; f < N_FEATURES; ++f)
            {
                sums[cluster][f] += static_cast<float>(data[i][f]);
            }
        }

        for (uint16_t c = 0; c < k; ++c)
        {
            if (counts[c] == 0)
            {
                centroids[c] = data[dist(rng)];
                continue;
            }

            float inv = 1.0f / counts[c];

            for (int f = 0; f < N_FEATURES; ++f)
            {
                centroids[c][f] = sums[c][f] * inv;
            }
        }
    }


    static float distance_squared(const FeatureVector& a, const Centroid& b)
    {
        float result = 0.0f;

        for (int i = 0; i < N_FEATURES; ++i)
        {
            float d = static_cast<float>(a[i]) - b[i];

            result += d * d;
        }

        return result;
    }
};

class KMeansR
{
public:
    static constexpr int N_FEATURES = 10;
    using FeatureVector = std::array<float, N_FEATURES>;
    using Centroid = std::array<float, N_FEATURES>;
    std::vector<Centroid> centroids;
    std::vector<uint16_t> labels;


    void fit(const std::vector<FeatureVector>& data, uint16_t k, uint32_t iterations = 50)
    {
        if (data.empty())
            return;

        if (k > data.size())
            throw std::runtime_error("K cannot exceed number of samples");

        centroids.resize(k);
        labels.resize(data.size());

        initialize_centroids(data, k);

        std::cout << "Initialization done \n";

        for (uint32_t iteration = 0; iteration < iterations; ++iteration)
        {
            assign_clusters(data, k);
            update_centroids(data, k);
            std::cout << "Iteration: " << iteration << std::endl;
        }

        assign_clusters(data, k);
    }


private:

    void initialize_centroids(const std::vector<FeatureVector>& data, uint16_t k)
    {
        std::random_device rd;
        std::mt19937 rng(rd());

        std::uniform_int_distribution<size_t> dist(0, data.size() - 1);

        for (uint16_t i = 0; i < k; ++i)
        {
            centroids[i] = data[dist(rng)];
        }
    }


    void assign_clusters(const std::vector<FeatureVector>& data, uint16_t k)
    {
        for (size_t i = 0; i < data.size(); ++i)
        {
            float best_distance = std::numeric_limits<float>::max();
            uint16_t best_cluster = 0;

            for (uint16_t c = 0; c < k; ++c)
            {
                float d = distance_squared(data[i], centroids[c]);

                if (d < best_distance)
                {
                    best_distance = d;
                    best_cluster = c;
                }
            }

            labels[i] = best_cluster;
        }
    }


    void update_centroids(const std::vector<FeatureVector>& data, uint16_t k)
    {
        std::random_device rd;
        std::mt19937 rng(rd());

        std::uniform_int_distribution<size_t> dist(0, data.size() - 1);

        std::vector<Centroid> sums(k);
        std::vector<uint32_t> counts(k, 0);

        for (size_t i = 0; i < data.size(); ++i)
        {
            uint16_t cluster = labels[i];

            ++counts[cluster];

            for (int f = 0; f < N_FEATURES; ++f)
            {
                sums[cluster][f] += static_cast<float>(data[i][f]);
            }
        }

        for (uint16_t c = 0; c < k; ++c)
        {
            if (counts[c] == 0)
            {
                centroids[c] = data[dist(rng)];
                continue;
            }

            float inv = 1.0f / counts[c];

            for (int f = 0; f < N_FEATURES; ++f)
            {
                centroids[c][f] = sums[c][f] * inv;
            }
        }
    }


    static float distance_squared(const FeatureVector& a, const Centroid& b)
    {
        float result = 0.0f;

        for (int i = 0; i < N_FEATURES; ++i)
        {
            float d = static_cast<float>(a[i]) - b[i];

            result += d * d;
        }

        return result;
    }
};

class BucketManager
{
public:
    using ComboId = uint16_t;
    using Mask = uint64_t;

    static constexpr uint16_t INVALID_BUCKET = 0xFFFF;
    static constexpr uint16_t PREFLOP_BUCKETS = 169;
    static constexpr uint16_t FLOP_BUCKETS = 2000;
    static constexpr uint16_t TURN_BUCKETS = 2000;
    static constexpr uint16_t RIVER_BUCKETS = 1000;

    struct BucketLookup
    {
        uint32_t num_boards = 0;
        uint16_t num_buckets = 0;

        std::vector<uint64_t> board_masks;
        std::vector<uint16_t> data;
        std::unordered_map<uint64_t, uint32_t> board_lookup;

        inline uint16_t get(uint64_t board_mask, ComboId combo) const
        {
            uint32_t board_id = board_lookup.at(board_mask);

            return data[static_cast<size_t>(board_id) * 1326 + combo];
        }

        inline void set_boards(const std::vector<uint64_t>& masks)
        {
            board_masks = masks;

            board_lookup.clear();

            board_lookup.reserve(board_masks.size());

            for (uint32_t i = 0; i < board_masks.size(); ++i)
            {
                board_lookup.emplace(board_masks[i], i);
            }

            num_boards = static_cast<uint32_t>(board_masks.size());

            data.resize(static_cast<size_t>(num_boards) * 1326, INVALID_BUCKET);
        }
    };

    struct BucketFileHeader
    {
        uint32_t magic = 0x54425543; // "TBUC"
        uint32_t version = 1;

        uint32_t num_boards;
        uint32_t combos_per_board;

        uint16_t num_buckets;
        uint16_t reserved = 0;
    };

    BucketLookup preflop;
    BucketLookup flop;
    BucketLookup turn;
    BucketLookup river;
    ComboIndexer Combos;
    Evaluator eval;
    Canon canon;

    BucketManager()
    {
        canon.load_perm_ids("Data/PermIds.dat");
    }

    std::pair<std::vector<std::array<uint8_t, 5>>, std::vector<uint64_t>> get_canon_flops()
    {
        std::vector<uint64_t> canon_masks;
        std::vector<std::array<uint8_t, 5>> canon_boards;
        std::unordered_map<uint64_t, uint32_t> canon_lookup;
        canon_lookup.reserve(2000);

        for (uint8_t a = 0; a < 50; ++a)
        {
            for (uint8_t b = a + 1; b < 51; ++b)
            {
                for (uint8_t c = b + 1; c < 52; ++c)
                {
                    uint64_t flop_mask = uint64_t(1) << a | uint64_t(1) << b | uint64_t(1) << c;
                    std::array<uint8_t, 5> flop_board = { a,b,c };
                    uint8_t perm_id = canon.get_perm_id(flop_board, 3);
                    uint64_t canon_mask = canon.get_canon_mask(flop_mask, perm_id);

                    auto [it, inserted] = canon_lookup.emplace(canon_mask, static_cast<uint32_t>(canon_masks.size()));

                    if (inserted)
                    {
                        canon_masks.push_back(canon_mask);

                        canon_boards.push_back(canon.get_canon_board(flop_board, perm_id, 3));
                    }
                }
            }
        }
        return std::pair<std::vector<std::array<uint8_t, 5>>, std::vector<uint64_t>> {canon_boards, canon_masks};
    }

    std::pair<std::vector<std::array<uint8_t, 5>>, std::vector<uint64_t>> get_canon_turns()
    {
        std::vector<uint64_t> canon_masks;
        std::vector<std::array<uint8_t, 5>> canon_boards;
        std::unordered_map<uint64_t, uint32_t> canon_lookup;
        canon_lookup.reserve(20000);

        for (uint8_t a = 0; a < 49; ++a)
        {
            for (uint8_t b = a + 1; b < 50; ++b)
            {
                for (uint8_t c = b + 1; c < 51; ++c)
                {
                    for (uint8_t d = c + 1; d < 52; ++d)
                    {
                        uint64_t turn_mask = uint64_t(1) << a | uint64_t(1) << b | uint64_t(1) << c | uint64_t(1) << d;
                        std::array<uint8_t, 5> turn_board = { a,b,c,d };
                        uint8_t perm_id = canon.get_perm_id(turn_board, 4);
                        uint64_t canon_mask = canon.get_canon_mask(turn_mask, perm_id);

                        auto [it, inserted] = canon_lookup.emplace(canon_mask, static_cast<uint32_t>(canon_masks.size()));

                        if (inserted)
                        {
                            canon_masks.push_back(canon_mask);

                            canon_boards.push_back(canon.get_canon_board(turn_board, perm_id, 4));
                        }
                    }
                }
            }
        }
        return std::pair<std::vector<std::array<uint8_t, 5>>, std::vector<uint64_t>> {canon_boards, canon_masks};
    }

    std::pair<std::vector<std::array<uint8_t, 5>>, std::vector<uint64_t>> get_canon_rivers()
    {
        std::vector<uint64_t> canon_masks;
        std::vector<std::array<uint8_t, 5>> canon_boards;
        std::unordered_map<uint64_t, uint32_t> canon_lookup;
        canon_lookup.reserve(200000);

        for (uint8_t a = 0; a < 48; ++a)
        {
            for (uint8_t b = a + 1; b < 49; ++b)
            {
                for (uint8_t c = b + 1; c < 50; ++c)
                {
                    for (uint8_t d = c + 1; d < 51; ++d)
                    {
                        for (uint8_t e = d + 1; e < 52; ++e)
                        {
                            uint64_t river_mask = uint64_t(1) << a | uint64_t(1) << b | uint64_t(1) << c | uint64_t(1) << d | uint64_t(1) << e;
                            std::array<uint8_t, 5> river_board = { a,b,c,d,e };
                            uint8_t perm_id = canon.get_perm_id(river_board, 5);
                            uint64_t canon_mask = canon.get_canon_mask(river_mask, perm_id);

                            auto [it, inserted] = canon_lookup.emplace(canon_mask, static_cast<uint32_t>(canon_masks.size()));

                            if (inserted)
                            {
                                canon_masks.push_back(canon_mask);

                                canon_boards.push_back(canon.get_canon_board(river_board, perm_id, 5));
                            }
                        }
                    }
                }
            }
        }
        return std::pair<std::vector<std::array<uint8_t, 5>>, std::vector<uint64_t>> {canon_boards, canon_masks};
    }

    struct ScoreCounts
    {
        int wins;
        int ties;
        int total;
    };

    ScoreCounts get_score_counts(uint16_t hero_score, uint8_t c1, uint8_t c2, const std::vector<uint16_t>& all_scores, const std::array<std::vector<uint16_t>, 52>& card_scores)
    {
        // Count all hands with a lower score.
        // Higher evaluator score = stronger hand.
        int wins = static_cast<int>(std::lower_bound(all_scores.begin(), all_scores.end(), hero_score) - all_scores.begin());

        // Remove hands containing either hero card.
        for (uint8_t c : { c1, c2 })
        {
            const auto& scores = card_scores[c];

            wins -= static_cast<int>(std::lower_bound(scores.begin(), scores.end(), hero_score) - scores.begin());
        }

        // Count all hands tying the hero.
        auto lo = std::lower_bound(all_scores.begin(), all_scores.end(), hero_score);

        auto hi = std::upper_bound(all_scores.begin(),all_scores.end(), hero_score);

        int ties = static_cast<int>(hi - lo);

        // Remove ties blocked by either hero card.
        for (uint8_t c : { c1, c2 })
        {
            const auto& scores = card_scores[c];

            auto card_lo = std::lower_bound(scores.begin(), scores.end(), hero_score);

            auto card_hi = std::upper_bound(scores.begin(), scores.end(), hero_score);

            ties -= static_cast<int>(card_hi - card_lo);
        }

        // The hero combo itself was subtracted twice.
        // Add it back once to correct inclusion-exclusion.
        ++ties;

        // Exclude all hands containing either hero card.
        // Add 1 back because the hero combo was counted twice.
        int total = static_cast<int>(all_scores.size()) - static_cast<int>(card_scores[c1].size()) - static_cast<int>(card_scores[c2].size()) + 1;

        return { wins, ties, total };
    }

    std::vector<std::array<float, 21>> get_flop_features(const std::pair<std::vector<std::array<uint8_t, 5>>, std::vector<uint64_t>>& flop_pair)
    {
        const auto& flop_boards = flop_pair.first;
        const auto& flop_masks = flop_pair.second;

        std::vector<std::array<float, 21>> features;
        features.resize(flop_boards.size() * 1326);

        for (uint32_t i = 0; i < flop_boards.size(); ++i)
        {
            if (i % 10 == 0)
                std::cout << i << std::endl;

            const uint64_t flop_mask = flop_masks[i];

            // Loop over every possible turn.
            for (uint8_t t = 0; t < 52; ++t)
            {
                const uint64_t turn_bit = uint64_t(1) << t;

                if (flop_mask & turn_bit)
                    continue;

                const uint64_t turn_mask = flop_mask | turn_bit;

                // Accumulate results across all rivers for this turn.
                std::array<uint32_t, 1326> wins{};
                std::array<uint32_t, 1326> ties{};
                std::array<uint32_t, 1326> total{};

                // Reuse these containers across rivers.
                std::array<uint16_t, 1326> scores{};

                std::vector<uint16_t> all_scores;
                all_scores.reserve(1326);

                std::array<std::vector<uint16_t>, 52> card_scores;

                // Each card can appear in at most 51 Hold'em combos.
                for (auto& v : card_scores)
                    v.reserve(51);

                // Loop over every possible river.
                for (uint8_t r = 0; r < 52; ++r)
                {
                    const uint64_t river_bit = uint64_t(1) << r;

                    if (turn_mask & river_bit)
                        continue;

                    const uint64_t board_mask = turn_mask | river_bit;

                    const std::array<uint8_t, 5> board = {flop_boards[i][0], flop_boards[i][1], flop_boards[i][2], t, r};

                    // Clear the previous river's score data.
                    all_scores.clear();

                    for (auto& v : card_scores)
                        v.clear();

                    // Evaluate every combo legal on this board.
                    for (ComboId c = 0; c < 1326; ++c)
                    {
                        const auto& hand = Combos.get_combo(c);

                        if (hand.mask & board_mask)
                            continue;

                        const uint16_t score =
                            eval.evaluate(hand.card1, hand.card2, board);

                        scores[c] = score;

                        // Add to the complete legal score list.
                        all_scores.push_back(score);

                        // Add to each card-specific score list.
                        card_scores[hand.card1].push_back(score);
                        card_scores[hand.card2].push_back(score);
                    }

                    // Sort all score lists for binary searching.
                    std::sort(all_scores.begin(), all_scores.end());

                    for (auto& v : card_scores)
                        std::sort(v.begin(), v.end());

                    // Evaluate every legal hero combo.
                    for (ComboId hero_id = 0; hero_id < 1326; ++hero_id)
                    {
                        const auto& hero = Combos.get_combo(hero_id);

                        // Skip hero combos blocked by this river board.
                        if (hero.mask & board_mask)
                            continue;

                        const ScoreCounts result = get_score_counts(scores[hero_id], hero.card1, hero.card2, all_scores, card_scores);

                        // Aggregate matchup counts across rivers.
                        wins[hero_id] += result.wins;
                        ties[hero_id] += result.ties;
                        total[hero_id] += result.total;
                    }
                }

                // One equity histogram observation per turn.
                for (ComboId hero_id = 0; hero_id < 1326; ++hero_id)
                {
                    const auto& hero = Combos.get_combo(hero_id);

                    // Exclude hands blocked by the flop or turn.
                    if (hero.mask & turn_mask)
                        continue;

                    // Safety guard: avoid division by zero.
                    if (total[hero_id] == 0)
                        continue;

                    const float eq = (static_cast<float>(wins[hero_id]) + 0.5f * static_cast<float>(ties[hero_id])) / static_cast<float>(total[hero_id]);

                    // Map equity [0, 1] to histogram bins [0, 50].
                    const int bin = std::clamp(static_cast<int>(eq * 20.0f), 0, 20);

                    features[static_cast<size_t>(i) * 1326 + hero_id][bin] += 1.0f;
                }
            }
        }

        return features;
    }

    std::vector<std::array<float, 21>> get_turn_features(const std::pair<std::vector<std::array<uint8_t, 5>>, std::vector<uint64_t>>&turn_pair)
    {
        const auto& turn_boards = turn_pair.first;
        const auto& turn_masks = turn_pair.second;

        std::vector<std::array<float, 21>> features;
        features.resize(turn_boards.size() * 1326);

        for (uint32_t i = 0; i < turn_boards.size(); ++i)
        {
            if (i % 100 == 0)
                std::cout << i << std::endl;

            const uint64_t turn_mask = turn_masks[i];

            // Every possible river is one observation.
            for (uint8_t r = 0; r < 52; ++r)
            {
                const uint64_t river_bit = uint64_t(1) << r;

                if (turn_mask & river_bit)
                    continue;

                const uint64_t river_mask = turn_mask | river_bit;

                const std::array<uint8_t, 5> board = { turn_boards[i][0], turn_boards[i][1], turn_boards[i][2], turn_boards[i][3], r};

                // Scores for every combo on this river.
                std::array<uint16_t, 1326> scores{};

                // All legal villain scores.
                std::vector<uint16_t> all_scores;
                all_scores.reserve(1081);

                // Scores grouped by cards, used to remove blocked villain combos.
                std::array<std::vector<uint16_t>, 52> card_scores;

                for (auto& v : card_scores)
                    v.reserve(51);

                // Evaluate every legal combo.
                for (ComboId c = 0; c < 1326; ++c)
                {
                    const auto& hand = Combos.get_combo(c);

                    if (hand.mask & river_mask)
                        continue;

                    const uint16_t score = eval.evaluate(hand.card1, hand.card2, board);

                    scores[c] = score;

                    all_scores.push_back(score);

                    card_scores[hand.card1].push_back(score);
                    card_scores[hand.card2].push_back(score);
                }

                // Sort once so get_score_counts() can use binary search.
                std::sort(all_scores.begin(), all_scores.end());

                for (auto& v : card_scores)
                    std::sort(v.begin(), v.end());

                // Generate one 21-bin equity histogram observation
                // for every legal hero combo on this river.
                for (ComboId hero_id = 0; hero_id < 1326; ++hero_id)
                {
                    const auto& hero = Combos.get_combo(hero_id);

                    if (hero.mask & river_mask)
                        continue;

                    const ScoreCounts result = get_score_counts(scores[hero_id], hero.card1, hero.card2, all_scores, card_scores);

                    if (result.total == 0)
                        continue;

                    const float equity = (static_cast<float>(result.wins) + 0.5f * static_cast<float>(result.ties)) / static_cast<float>(result.total);

                    // 21 bins: [0.00, 0.05, ..., 1.00]
                    const int bin = std::clamp(static_cast<int>(equity * 20.0f), 0, 20);

                    features[static_cast<size_t>(i) * 1326 + hero_id][bin] += 1.0f;
                }
            }
        }

        return features;
    }

    std::vector<std::array<float, 10>> get_river_features(const std::pair<std::vector<std::array<uint8_t, 5>>, std::vector<uint64_t>>&river_pair)
    {
        const auto& river_boards = river_pair.first;
        const auto& river_masks = river_pair.second;

        constexpr int N_COMBOS = 1326;
        constexpr int N_RANGES = 10;

        std::vector<std::array<float, N_RANGES>> features;
        features.resize(static_cast<size_t>(river_boards.size()) * N_COMBOS);

        // ------------------------------------------------------------
        // Build range assignment once.
        // ------------------------------------------------------------

        std::array<uint8_t, N_COMBOS> ranges;

        for (uint16_t c = 0; c < 1326; ++c)
        {
            const auto& hand = Combos.get_combo(c);

            int r1 = hand.card1 / 4;
            int r2 = hand.card2 / 4;

            int s1 = hand.card1 % 4;
            int s2 = hand.card2 % 4;

            int max = std::max(r1, r2);
            int min = std::min(r1, r2);

            if ((r1 == r2 && r1 > 5) || (r1 + r2 >= 21) || (r1 + r2 >= 19 && s1 == s2))
                ranges[c] = 0;

            else if ((r1 == r2 && r1 >= 3) ||
                (s1 == s2 && ((max == 12 && min >= 1) || (max == 11 && min >= 5) || (max >= 9 && min >= 7))) ||
                (r1 + r2 >= 19 || (max == 12 && min == 6)))
            {
                ranges[c] = 1;
            }

            else if ((r1 == 2 && r2 == 2) ||
                (s1 == s2 && (max == 12 || (max == 11 && min >= 1) || r1 + r2 >= 15 || (max == 8 && min == 6))) ||
                ((max == 12 && min >= 4) || (r1 + r2 >= 18) || (r1 + r2 == 17 && max != 11)))
            {
                ranges[c] = 2;
            }

            else if ((r1 == 1 && r2 == 1) ||
                (s1 == s2 && (max >= 11 || (max == 10 && min >= 3) || min >= 6 || (max >= 8 && min >= 5))) ||
                (max == 12 || (max >= 11 && min >= 4) || (max >= 10 && min >= 6) || (max >= 8 && min >= 7)))
            {
                ranges[c] = 3;
            }

            else if ((r1 + r2 == 0) ||
                (s1 == s2 && (max >= 10 || (max >= 9 && min >= 1) || min >= 4)) ||
                ((max >= 11 && min >= 2) || (max >= 10 && min >= 5) || min >= 6))
            {
                ranges[c] = 4;
            }

            else if ((s1 == s2 && (max >= 8 || min >= 3)) ||
                (max >= 11 || (max >= 10 && min >= 2) || r1 + r2 >= 13))
            {
                ranges[c] = 5;
            }

            else if ((s1 == s2 && (max >= 7 || min >= 2 || (max == 3 && min == 1))) ||
                (max >= 10 || (max >= 9 && min >= 2) || (max >= 6 && min >= 4)))
            {
                ranges[c] = 6;
            }

            else if ((s1 == s2 && (max >= 3 || min >= 1)) ||
                (max >= 9 || (max >= 8 && min >= 2) || (max >= 6 && min >= 3) || (max >= 5 && min >= 4)))
            {
                ranges[c] = 7;
            }

            else if (s1 == s2 ||
                (max >= 7 || min >= 2))
            {
                ranges[c] = 8;
            }

            else
            {
                ranges[c] = 9;
            }
        }

        // ------------------------------------------------------------
        // Temporary storage.
        //
        // range_scores[r] = all legal combo scores belonging to range r
        //
        // card_scores[r][card] = scores of range-r combos containing card
        //
        // All are sorted, allowing binary searches.
        // ------------------------------------------------------------

        std::array<std::vector<uint16_t>, N_RANGES> range_scores;

        std::array<std::array<std::vector<uint16_t>, 52>, N_RANGES> card_scores;

        std::array<uint16_t, N_COMBOS> scores;

        // ------------------------------------------------------------
        // Process every river.
        // ------------------------------------------------------------

        for (uint32_t i = 0; i < river_boards.size(); ++i)
        {
            if (i % 1000 == 0)
                std::cout << i << '\n';

            const uint64_t board_mask = river_masks[i];

            const auto& board = river_boards[i];

            // Clear temporary containers.
            for (int r = 0; r < N_RANGES; ++r)
            {
                range_scores[r].clear();

                for (int card = 0; card < 52; ++card)
                    card_scores[r][card].clear();
            }

            // --------------------------------------------------------
            // Evaluate every legal combo once.
            // --------------------------------------------------------

            for (ComboId c = 0; c < N_COMBOS; ++c)
            {
                const auto& hand = Combos.get_combo(c);

                if (hand.mask & board_mask)
                    continue;

                const uint16_t score = eval.evaluate(hand.card1, hand.card2, board);

                scores[c] = score;

                const uint8_t r = ranges[c];

                range_scores[r].push_back(score);

                card_scores[r][hand.card1].push_back(score);
                card_scores[r][hand.card2].push_back(score);
            }

            // --------------------------------------------------------
            // Sort score lists.
            // --------------------------------------------------------

            for (int r = 0; r < N_RANGES; ++r)
            {
                std::sort(range_scores[r].begin(), range_scores[r].end());

                for (int card = 0; card < 52; ++card)
                {
                    std::sort(card_scores[r][card].begin(), card_scores[r][card].end());
                }
            }

            // --------------------------------------------------------
            // Evaluate every legal hero.
            // --------------------------------------------------------

            for (ComboId hero_id = 0; hero_id < N_COMBOS; ++hero_id)
            {
                const auto& hero = Combos.get_combo(hero_id);

                if (hero.mask & board_mask)
                    continue;

                const uint16_t hero_score = scores[hero_id];

                for (int r = 0; r < N_RANGES; ++r)
                {
                    const auto& all = range_scores[r];

                    if (all.empty())
                    {
                        features[static_cast<size_t>(i) * N_COMBOS + hero_id][r] = 0.0f;

                        continue;
                    }

                    // Number of range-r hands with score < hero.
                    int wins = static_cast<int>(std::lower_bound(all.begin(), all.end(), hero_score) - all.begin());

                    // Number with score <= hero.
                    int not_greater = static_cast<int>(std::upper_bound(all.begin(), all.end(), hero_score) - all.begin());

                    int ties = not_greater - wins;

                    // ------------------------------------------------
                    // Remove hands containing hero.card1.
                    // ------------------------------------------------

                    const auto& c1_scores = card_scores[r][hero.card1];

                    wins -= static_cast<int>(std::lower_bound(c1_scores.begin(), c1_scores.end(), hero_score) - c1_scores.begin());

                    int c1_not_greater = static_cast<int>(std::upper_bound(c1_scores.begin(), c1_scores.end(), hero_score) - c1_scores.begin());

                    ties -= c1_not_greater - static_cast<int>(std::lower_bound(c1_scores.begin(), c1_scores.end(), hero_score) - c1_scores.begin());

                    // ------------------------------------------------
                    // Remove hands containing hero.card2.
                    // ------------------------------------------------

                    const auto& c2_scores = card_scores[r][hero.card2];

                    wins -= static_cast<int>(std::lower_bound(c2_scores.begin(), c2_scores.end(), hero_score) - c2_scores.begin());

                    int c2_not_greater = static_cast<int>(std::upper_bound(c2_scores.begin(), c2_scores.end(), hero_score) - c2_scores.begin());

                    ties -= c2_not_greater - static_cast<int>(std::lower_bound(c2_scores.begin(), c2_scores.end(), hero_score) - c2_scores.begin());

                    // The hero itself was removed twice from the tie
                    // count, so add it back once.
                    //
                    // It is not present in wins because its score is
                    // equal to hero_score.
                    if (scores[hero_id] == hero_score)
                        ++ties;

                    const int total = static_cast<int>(all.size()) - static_cast<int>(card_scores[r][hero.card1].size()) - static_cast<int>(card_scores[r][hero.card2].size()) + 1;

                    features[static_cast<size_t>(i) * N_COMBOS + hero_id][r] = (wins + 0.5f * ties) / static_cast<float>(total);
                }
            }
        }

        return features;
    }

    void save_preflop_buckets(const std::string& filename)
    {
        std::unordered_map<uint32_t, uint16_t> offsuit;
        std::unordered_map<uint32_t, uint16_t> suit;
        std::unordered_map<uint8_t, uint16_t> pair;

        BucketFileHeader header;

        BucketLookup lookup;
        lookup.num_buckets = 169;

        std::vector<uint64_t> board_masks;
        board_masks.push_back(0);

        lookup.set_boards(board_masks);
        header.num_boards = lookup.num_boards;
        header.combos_per_board = 1326;
        header.num_buckets = lookup.num_buckets;

        uint16_t bucket = 0;

        for (uint16_t combo = 0; combo < 1326; ++combo)
        {
            const auto& hand = Combos.get_combo(combo);

            uint8_t c1 = hand.card1;
            uint8_t c2 = hand.card2;

            if (c1 / 4 == c2 / 4)
            {
                auto cached = pair.find(c1 / 4);

                if (cached != pair.end())
                {
                    lookup.data[combo] = cached->second;
                    continue;
                }

                auto [it, inserted] = pair.emplace(c1 / 4, bucket);

                if (inserted)
                    ++bucket;

                lookup.data[combo] = it->second;
            }

            else if (c1 % 4 == c2 % 4)
            {
                uint32_t key = (uint32_t(1) << (c1 / 4)) | (uint32_t(1) << (c2 / 4));
                auto cached = suit.find(key);

                if (cached != suit.end())
                {
                    lookup.data[combo] = cached->second;
                    continue;
                }

                auto [it, inserted] = suit.emplace(key, bucket);

                if (inserted)
                    ++bucket;

                lookup.data[combo] = it->second;
            }

            else
            {
                uint32_t key = (uint32_t(1) << (c1 / 4)) | (uint32_t(1) << (c2 / 4));
                auto cached = offsuit.find(key);

                if (cached != offsuit.end())
                {
                    lookup.data[combo] = cached->second;
                    continue;
                }

                auto [it, inserted] = offsuit.emplace(key, bucket);

                if (inserted)
                    ++bucket;

                lookup.data[combo] = it->second;
            }
        }

        std::ofstream file(filename, std::ios::binary);

        if (!file)
            throw std::runtime_error("Could not open bucket file for writing");

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        file.write(reinterpret_cast<const char*>(lookup.board_masks.data()), lookup.board_masks.size() * sizeof(uint64_t));

        file.write(reinterpret_cast<const char*>(lookup.data.data()), lookup.data.size() * sizeof(uint16_t));

        if (!file)
            throw std::runtime_error("Error writing bucket file");
    }

    void save_flop_buckets(const std::string& filename)
    {
        BucketFileHeader header;

        BucketLookup lookup;
        lookup.num_buckets = FLOP_BUCKETS;

        std::pair<std::vector<std::array<uint8_t, 5>>, std::vector<uint64_t>> flop_pair = get_canon_flops();

        std::cout << "flops canonicalized\n";

        std::vector<std::array<float, 21>> features = get_flop_features(flop_pair);

        std::cout << "Features Done\n";

        std::vector<std::array<uint8_t, 5>> flop_boards = flop_pair.first;
        std::vector<uint64_t> flop_masks = flop_pair.second;

        lookup.set_boards(flop_masks);
        header.num_boards = lookup.num_boards;
        header.combos_per_board = 1326;
        header.num_buckets = lookup.num_buckets;

        std::vector<std::array<float, 21>> coarse_training_data;
        std::vector<size_t> coarse_training_indices;

        for (uint32_t i = 0; i < flop_boards.size(); ++i)
        {
            uint64_t flop_mask = flop_masks[i];

            for (ComboId combo = 0; combo < 1326; ++combo)
            {
                const auto& hand = Combos.get_combo(combo);

                if (hand.mask & flop_mask)
                    continue;

                size_t index = static_cast<size_t>(i) * 1326 + combo;

                coarse_training_data.push_back(features[index]);
                coarse_training_indices.push_back(index);
            }
        }
        KMeans Coarse;
        Coarse.fit(coarse_training_data, 100, 50);

        for (uint8_t i = 0; i < 100; ++i)
        {
            KMeans Fine;

            std::vector<std::array<float, 21>> fine_training_data;
            std::vector<size_t> fine_training_indices;

            for (size_t j = 0; j < coarse_training_indices.size(); ++j)
            {
                size_t index = coarse_training_indices[j];

                if (Coarse.labels[j] == i)
                {
                    fine_training_data.push_back(coarse_training_data[j]);
                    fine_training_indices.push_back(coarse_training_indices[j]);
                }
            }

            Fine.fit(fine_training_data, 20, 50);

            for (size_t j = 0; j < fine_training_indices.size(); ++j)
            {
                size_t index = fine_training_indices[j];

                lookup.data[index] = i * 20 + Fine.labels[j];
            }

            std::cout << i << std::endl;
        }

        std::ofstream file(filename, std::ios::binary);

        if (!file)
            throw std::runtime_error("Could not open bucket file for writing");

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        file.write(reinterpret_cast<const char*>(lookup.board_masks.data()), lookup.board_masks.size() * sizeof(uint64_t));

        file.write(reinterpret_cast<const char*>(lookup.data.data()), lookup.data.size() * sizeof(uint16_t));

        if (!file)
            throw std::runtime_error("Error writing bucket file");
    }
    
    void save_turn_buckets(const std::string& filename)
    {
        BucketFileHeader header;

        BucketLookup lookup;
        lookup.num_buckets = TURN_BUCKETS;

        std::pair<std::vector<std::array<uint8_t, 5>>, std::vector<uint64_t>> turn_pair = get_canon_turns();

        std::cout << "Turns canonicalized\n";

        std::vector<std::array<float, 21>> features = get_turn_features(turn_pair);

        std::cout << "Features Done\n";

        std::vector<std::array<uint8_t, 5>> turn_boards = turn_pair.first;
        std::vector<uint64_t> turn_masks = turn_pair.second;

        lookup.set_boards(turn_masks);
        header.num_boards = lookup.num_boards;
        header.combos_per_board = 1326;
        header.num_buckets = lookup.num_buckets;

        std::vector<std::array<float, 21>> coarse_training_data;
        std::vector<size_t> coarse_training_indices;

        for (uint32_t i = 0; i < turn_boards.size(); ++i)
        {
            uint64_t turn_mask = turn_masks[i];

            for (ComboId combo = 0; combo < 1326; ++combo)
            {
                const auto& hand = Combos.get_combo(combo);

                if (hand.mask & turn_mask)
                    continue;

                size_t index = static_cast<size_t>(i) * 1326 + combo;

                coarse_training_data.push_back(features[index]);
                coarse_training_indices.push_back(index);
            }
        }
        KMeans Coarse;
        Coarse.fit(coarse_training_data, 100, 50);

        for (uint8_t i = 0; i < 100; ++i)
        {
            KMeans Fine;

            std::vector<std::array<float, 21>> fine_training_data;
            std::vector<size_t> fine_training_indices;

            for (size_t j = 0; j < coarse_training_indices.size(); ++j)
            {
                size_t index = coarse_training_indices[j];

                if (Coarse.labels[j] == i)
                {
                    fine_training_data.push_back(coarse_training_data[j]);
                    fine_training_indices.push_back(coarse_training_indices[j]);
                }
            }

            Fine.fit(fine_training_data, 20, 50);

            for (size_t j = 0; j < fine_training_indices.size(); ++j)
            {
                size_t index = fine_training_indices[j];

                lookup.data[index] = i * 20 + Fine.labels[j];
            }

        }

        std::ofstream file(filename, std::ios::binary);

        if (!file)
            throw std::runtime_error("Could not open bucket file for writing");

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        file.write(reinterpret_cast<const char*>(lookup.board_masks.data()), lookup.board_masks.size() * sizeof(uint64_t));

        file.write(reinterpret_cast<const char*>(lookup.data.data()), lookup.data.size() * sizeof(uint16_t));

        if (!file)
            throw std::runtime_error("Error writing bucket file");
    }

    void save_river_buckets(const std::string& filename)
    {
        BucketFileHeader header;

        BucketLookup lookup;
        lookup.num_buckets = RIVER_BUCKETS;

        std::pair<std::vector<std::array<uint8_t, 5>>, std::vector<uint64_t>> river_pair = get_canon_rivers();

        std::cout << "Rivers canonicalized\n";

        std::vector<std::array<float, 10>> features = get_river_features(river_pair);

        std::cout << "Features Done\n";

        std::vector<std::array<uint8_t, 5>> river_boards = river_pair.first;
        std::vector<uint64_t> river_masks = river_pair.second;

        lookup.set_boards(river_masks);
        header.num_boards = lookup.num_boards;
        header.combos_per_board = 1326;
        header.num_buckets = lookup.num_buckets;

        std::vector<std::array<float, 10>> coarse_training_data;
        std::vector<size_t> coarse_training_indices;

        for (uint32_t i = 0; i < river_boards.size(); ++i)
        {
            uint64_t river_mask = river_masks[i];

            for (ComboId combo = 0; combo < 1326; ++combo)
            {
                const auto& hand = Combos.get_combo(combo);

                if (hand.mask & river_mask)
                    continue;

                size_t index = static_cast<size_t>(i) * 1326 + combo;

                coarse_training_data.push_back(features[index]);
                coarse_training_indices.push_back(index);
            }
        }
        KMeansR Coarse;
        Coarse.fit(coarse_training_data, 50, 50);

        for (uint8_t i = 0; i < 50; ++i)
        {
			std::cout << "Fine iteration " << static_cast<int>(i) << std::endl;
            KMeansR Fine;

            std::vector<std::array<float, 10>> fine_training_data;
            std::vector<size_t> fine_training_indices;

            for (size_t j = 0; j < coarse_training_indices.size(); ++j)
            {
                size_t index = coarse_training_indices[j];

                if (Coarse.labels[j] == i)
                {
                    fine_training_data.push_back(coarse_training_data[j]);
                    fine_training_indices.push_back(coarse_training_indices[j]);
                }
            }

            Fine.fit(fine_training_data, 20, 50);

            std::cout << "fit done" << std::endl;

            for (size_t j = 0; j < fine_training_indices.size(); ++j)
            {
                size_t index = fine_training_indices[j];

                lookup.data[index] = i * 20 + Fine.labels[j];
            }

            std::cout << "lookup updated" << std::endl;

        }

        std::ofstream file(filename, std::ios::binary);

        if (!file)
            throw std::runtime_error("Could not open bucket file for writing");

            std::cout << "Before header write" << std::endl;

            file.write(
                reinterpret_cast<const char*>(&header),
                sizeof(header)
            );

            std::cout << "After header write" << std::endl;

            std::cout << "File state: "
                << "good=" << file.good()
                << ", fail=" << file.fail()
                << ", bad=" << file.bad()
                << std::endl;

        file.write(reinterpret_cast<const char*>(lookup.board_masks.data()), lookup.board_masks.size() * sizeof(uint64_t));

        std::cout << "file wrote2" << std::endl;

        file.write(reinterpret_cast<const char*>(lookup.data.data()), lookup.data.size() * sizeof(uint16_t));

        std::cout << "file wrote3" << std::endl;

        if (!file)
            throw std::runtime_error("Error writing bucket file");

        std::cout << "buecktes saved" << std::endl;
    }

    static BucketLookup load_buckets(const std::string& filename)
    {
        BucketLookup lookup;

        std::ifstream file(filename, std::ios::binary);

        if (!file)
            throw std::runtime_error("Could not open bucket file");

        BucketFileHeader header;

        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (header.magic != 0x54425543)
            throw std::runtime_error("Invalid bucket file");

        if (header.version != 1)
            throw std::runtime_error("Unsupported bucket file version");

        if (header.combos_per_board != 1326)
            throw std::runtime_error("Wrong combo count");

        lookup.num_boards = header.num_boards;
        lookup.num_buckets = header.num_buckets;

        // Load board masks
        lookup.board_masks.resize(header.num_boards);

        file.read(reinterpret_cast<char*>(lookup.board_masks.data()), lookup.board_masks.size() * sizeof(uint64_t));

        if (!file)
            throw std::runtime_error("Error reading board masks");

        // Rebuild board lookup
        lookup.board_lookup.reserve(lookup.board_masks.size());

        for (uint32_t i = 0; i < lookup.board_masks.size(); ++i)
        {
            lookup.board_lookup.emplace(lookup.board_masks[i], i);
        }

        // Load bucket data
        size_t count = static_cast<size_t>(header.num_boards) * 1326;

        lookup.data.resize(count);

        file.read(reinterpret_cast<char*>(lookup.data.data()), count * sizeof(uint16_t));

        if (!file)
            throw std::runtime_error("Error reading bucket data");

        return lookup;
    }

};