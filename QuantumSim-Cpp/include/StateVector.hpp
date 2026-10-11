#ifndef QUANTUMSIM_STATEVECTOR_HPP
#define QUANTUMSIM_STATEVECTOR_HPP

#include <complex>
#include <cstddef>
#include <string>
#include <vector>

namespace QuantumSim {

/// @brief Represents the state vector of an n-qubit quantum system.
///
/// The state of n qubits is a vector of 2^n complex amplitudes:
///
///   |ψ⟩ = Σ_{i=0}^{2^n - 1} a_i |i⟩
///
/// where each a_i ∈ ℂ and Σ |a_i|^2 = 1 (normalization).
///
/// Qubit indexing convention (little-endian):
///   Qubit 0 is the least significant bit of the basis state index.
///   For basis state index i, qubit q is |1⟩ iff (i >> q) & 1 == 1.
///
/// Example for 2 qubits:
///   Index 0 → |00⟩   (qubit 1 = 0, qubit 0 = 0)
///   Index 1 → |01⟩   (qubit 1 = 0, qubit 0 = 1)
///   Index 2 → |10⟩   (qubit 1 = 1, qubit 0 = 0)
///   Index 3 → |11⟩   (qubit 1 = 1, qubit 0 = 1)
///
class StateVector {
public:
    /// Maximum number of qubits supported.
    /// 25 qubits → 2^25 amplitudes × 16 bytes = 512 MB.
    static constexpr int MAX_QUBITS = 25;

    /// @brief Construct a state vector for num_qubits qubits.
    ///
    /// Allocates 2^num_qubits complex amplitudes and initializes
    /// the state to |00...0⟩ (all amplitude in basis state 0).
    ///
    /// @param num_qubits Number of qubits (must be 1 ≤ num_qubits ≤ MAX_QUBITS).
    /// @throws std::invalid_argument if num_qubits is out of range.
    explicit StateVector(int num_qubits);

    /// @brief Re-initialize the state to a computational basis state |k⟩.
    ///
    /// Sets amplitude[k] = 1 and all other amplitudes to 0.
    ///
    /// @param basis_index Index of the desired basis state (0 ≤ basis_index < 2^n).
    /// @throws std::out_of_range if basis_index is out of range.
    void initBasisState(std::size_t basis_index);

    /// @brief Get the number of qubits.
    int getNumQubits() const;

    /// @brief Get the total number of amplitudes (2^n).
    std::size_t getNumAmplitudes() const;

    /// @brief Get a const reference to the amplitude vector.
    const std::vector<std::complex<double>>& getAmplitudes() const;

    /// @brief Get a mutable reference to the amplitude vector.
    ///
    /// Used by gate operations to modify the state in-place.
    std::vector<std::complex<double>>& getAmplitudes();

    /// @brief Get the amplitude at a specific basis state index.
    ///
    /// @param index Basis state index.
    /// @throws std::out_of_range if index is out of range.
    std::complex<double> getAmplitude(std::size_t index) const;

    /// @brief Set the amplitude at a specific basis state index.
    ///
    /// @note This does NOT automatically re-normalize the state.
    /// @param index Basis state index.
    /// @param value New amplitude value.
    /// @throws std::out_of_range if index is out of range.
    void setAmplitude(std::size_t index, std::complex<double> value);

    /// @brief Calculate the measurement probability of a specific basis state.
    ///
    /// Returns |a_i|^2 (Born's rule).
    ///
    /// @param index Basis state index.
    /// @throws std::out_of_range if index is out of range.
    double probability(std::size_t index) const;

    /// @brief Check whether the state vector is normalized within a tolerance.
    ///
    /// Computes Σ |a_i|^2 and checks |sum - 1| < tolerance.
    ///
    /// @param tolerance Acceptable deviation from 1.0 (default: 1e-10).
    /// @return true if the state is normalized within the given tolerance.
    bool isNormalized(double tolerance = 1e-10) const;

    /// @brief Compute the sum of squared magnitudes: Σ |a_i|^2.
    ///
    /// For a valid quantum state, this should equal 1.0.
    double normSquared() const;

    /// @brief Print the state vector in a human-readable format.
    ///
    /// For each basis state, prints the index, binary label, amplitude,
    /// and measurement probability.
    void print() const;

    /// @brief Convert a basis state index to its binary string representation.
    ///
    /// Uses little-endian convention: qubit 0 is the rightmost character.
    /// The string has length num_qubits.
    ///
    /// @param index Basis state index.
    /// @return Binary string, e.g. "00", "01", "10", "11" for 2 qubits.
    std::string basisLabel(std::size_t index) const;

private:
    int num_qubits_;                              ///< Number of qubits.
    std::size_t num_amplitudes_;                   ///< 2^num_qubits.
    std::vector<std::complex<double>> amplitudes_; ///< State vector amplitudes.
};

} // namespace QuantumSim

#endif // QUANTUMSIM_STATEVECTOR_HPP
