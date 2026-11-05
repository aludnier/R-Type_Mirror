/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** FixedStep.h
*/

#ifndef FIXEDSTEP_H_
#define FIXEDSTEP_H_

#include <algorithm>

namespace RType {
/**
 * @brief Turns the time elapsed between frames into a number of fixed updates,
 *        so that the game runs at the same speed on any machine.
 */
class FixedStep {
public:
    /**
     * @param step Duration of one update, in seconds.
     * @param maxSteps Most updates run in one frame: after a long freeze the
     *        game slows down instead of never catching up.
     */
    explicit FixedStep(double step, int maxSteps = 5) :
        _step(step), _maxSteps(maxSteps)
    {
    }

    /**
     * @brief Adds the time of a frame.
     * @param elapsed Seconds since the previous frame.
     * @return Number of updates to run for this frame.
     */
    int advance(double elapsed)
    {
        _accumulated += elapsed;
        const int steps =
            std::min(static_cast<int>(_accumulated / _step), _maxSteps);
        _accumulated -= steps * _step;
        if (steps == _maxSteps)
            _accumulated = 0.0;
        return steps;
    }

    [[nodiscard]] double step() const
    {
        return _step;
    }

private:
    double _step;
    int _maxSteps;
    double _accumulated = 0.0;
};
} // namespace RType

#endif /* !FIXEDSTEP_H_ */
