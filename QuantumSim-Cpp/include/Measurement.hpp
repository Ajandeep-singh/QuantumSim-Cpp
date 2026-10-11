#ifndef QUANTUMSIM_MEASUREMENT_HPP
#define QUANTUMSIM_MEASUREMENT_HPP

#include "StateVector.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace QuantumSim {
namespace Measurement {

/// Return Born probabilities in basis-index order.
std::vector<double> probabilities(const StateVector& state);

/// Draw independent computational-basis samples using a reproducible seed.
/// The returned keys are basis labels in the simulator's little-endian convention.
std::map<std::string, std::size_t> sampleShots(
    const StateVector& state, std::size_t shots, std::uint64_t seed);

} // namespace Measurement
} // namespace QuantumSim

#endif
