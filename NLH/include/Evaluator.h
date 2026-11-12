#pragma once
#include <iostream>
#include <array>
#include "Hand.h"
#include "HandEvaluator.h"

class Evaluator
{
public:
    inline uint16_t evaluate(uint8_t c1, uint8_t c2, std::array<uint8_t, 5> board)
    {
        omp::Hand hand = omp::Hand::empty();

        hand += omp::Hand(c1);
        hand += omp::Hand(c2);
        hand += omp::Hand(board[0]);
        hand += omp::Hand(board[1]);
        hand += omp::Hand(board[2]);
        hand += omp::Hand(board[3]);
        hand += omp::Hand(board[4]);

        return eval.evaluate(hand);
    }

private:
    omp::HandEvaluator eval;

};
