#include "Measurement.hpp"

#include <cmath>
#include <random>
#include <stdexcept>

namespace QuantumSim {
namespace Measurement {

std::vector<double> probabilities(const StateVector& state) {
    std::vector<double> result;
    result.reserve(state.getNumAmplitudes());
    double total = 0.0;
    for (std::size_t i = 0; i < state.getNumAmplitudes(); ++i) {
        const double p = state.probability(i);
        if (!std::isfinite(p)) {
            throw std::invalid_argument("State contains a non-finite amplitude.");
        }
        result.push_back(p);
        total += p;
    }
    if (std::abs(total - 1.0) > 1e-9) {
        throw std::invalid_argument("Measurement requires a normalized state.");
    }
    return result;
}

std::map<std::string, std::size_t> sampleShots(
    const StateVector& state, std::size_t shots, std::uint64_t seed) {
    std::map<std::string, std::size_t> counts;
    if (shots == 0) {
        return counts;
    }
    const std::vector<double> probs = probabilities(state);
    std::discrete_distribution<std::size_t> distribution(probs.begin(), probs.end());
    std::mt19937_64 generator(seed);
    for (std::size_t shot = 0; shot < shots; ++shot) {
        ++counts[state.basisLabel(distribution(generator))];
    }
    return counts;
}

} // namespace Measurement
} // namespace QuantumSim
