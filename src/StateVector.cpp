#include "StateVector.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace QuantumSim {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

StateVector::StateVector(int num_qubits)
    : num_qubits_(num_qubits)
{
    // Validate qubit count
    if (num_qubits < 1 || num_qubits > MAX_QUBITS) {
        throw std::invalid_argument(
            "Number of qubits must be between 1 and "
            + std::to_string(MAX_QUBITS)
            + " (requested " + std::to_string(num_qubits) + ")."
        );
    }

    // Calculate 2^n using bit shift.
    // This is safe because num_qubits <= 25, so 2^25 = 33,554,432 fits in size_t.
    num_amplitudes_ = static_cast<std::size_t>(1) << num_qubits;

    // Allocate and initialize all amplitudes to zero.
    amplitudes_.assign(num_amplitudes_, {0.0, 0.0});

    // Set the initial state to |00...0⟩ (basis state 0).
    amplitudes_[0] = {1.0, 0.0};
}

// ---------------------------------------------------------------------------
// State initialization
// ---------------------------------------------------------------------------

void StateVector::initBasisState(std::size_t basis_index) {
    if (basis_index >= num_amplitudes_) {
        throw std::out_of_range(
            "Basis state index " + std::to_string(basis_index)
            + " is out of range for " + std::to_string(num_qubits_)
            + " qubits (valid range: 0 to "
            + std::to_string(num_amplitudes_ - 1) + ")."
        );
    }

    // Zero all amplitudes, then set the requested basis state to 1.
    for (auto& amp : amplitudes_) {
        amp = {0.0, 0.0};
    }
    amplitudes_[basis_index] = {1.0, 0.0};
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

int StateVector::getNumQubits() const {
    return num_qubits_;
}

std::size_t StateVector::getNumAmplitudes() const {
    return num_amplitudes_;
}

const std::vector<std::complex<double>>& StateVector::getAmplitudes() const {
    return amplitudes_;
}

std::vector<std::complex<double>>& StateVector::getAmplitudes() {
    return amplitudes_;
}

std::complex<double> StateVector::getAmplitude(std::size_t index) const {
    if (index >= num_amplitudes_) {
        throw std::out_of_range(
            "Amplitude index " + std::to_string(index)
            + " is out of range (valid: 0 to "
            + std::to_string(num_amplitudes_ - 1) + ")."
        );
    }
    return amplitudes_[index];
}

void StateVector::setAmplitude(std::size_t index, std::complex<double> value) {
    if (index >= num_amplitudes_) {
        throw std::out_of_range(
            "Amplitude index " + std::to_string(index)
            + " is out of range (valid: 0 to "
            + std::to_string(num_amplitudes_ - 1) + ")."
        );
    }
    amplitudes_[index] = value;
}

// ---------------------------------------------------------------------------
// Measurement probability (Born's rule)
// ---------------------------------------------------------------------------

double StateVector::probability(std::size_t index) const {
    if (index >= num_amplitudes_) {
        throw std::out_of_range(
            "Probability index " + std::to_string(index)
            + " is out of range (valid: 0 to "
            + std::to_string(num_amplitudes_ - 1) + ")."
        );
    }
    // P(i) = |a_i|^2 = Re(a_i)^2 + Im(a_i)^2
    return std::norm(amplitudes_[index]);
}

// ---------------------------------------------------------------------------
// Normalization
// ---------------------------------------------------------------------------

double StateVector::normSquared() const {
    double sum = 0.0;
    for (const auto& amp : amplitudes_) {
        sum += std::norm(amp); // std::norm returns |z|^2 for complex z
    }
    return sum;
}

bool StateVector::isNormalized(double tolerance) const {
    return std::abs(normSquared() - 1.0) < tolerance;
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

std::string StateVector::basisLabel(std::size_t index) const {
    // Build binary string with little-endian convention:
    //   qubit 0 is the rightmost character.
    //
    // For index=2 (binary 10) with 2 qubits:
    //   Bit 0 = 0, Bit 1 = 1 → string "10"
    //
    // We construct the string from left (high qubit) to right (low qubit).
    std::string label(static_cast<std::size_t>(num_qubits_), '0');
    for (int q = 0; q < num_qubits_; ++q) {
        if ((index >> q) & 1) {
            // Qubit q is |1⟩. In the label string, qubit 0 is at position
            // (num_qubits_ - 1) so that it appears on the right.
            label[static_cast<std::size_t>(num_qubits_ - 1 - q)] = '1';
        }
    }
    return label;
}

void StateVector::print() const {
    std::cout << "State vector (" << num_qubits_ << " qubit"
              << (num_qubits_ > 1 ? "s" : "") << ", "
              << num_amplitudes_ << " amplitudes):\n";

    std::cout << std::fixed << std::setprecision(6);

    for (std::size_t i = 0; i < num_amplitudes_; ++i) {
        const auto& amp = amplitudes_[i];
        double prob = std::norm(amp);

        std::cout << "  |" << basisLabel(i) << "> (index " << i << "): "
                  << "(" << amp.real() << ", " << amp.imag() << ")"
                  << "  P=" << prob << "\n";
    }

    double ns = normSquared();
    std::cout << "Normalized: " << (isNormalized() ? "YES" : "NO")
              << " (sum = " << ns << ")\n";
}

} // namespace QuantumSim
