#ifndef QUANTUMSIM_GATES_HPP
#define QUANTUMSIM_GATES_HPP

#include "StateVector.hpp"

#include <array>
#include <complex>

namespace QuantumSim {
namespace Gates {

// ---------------------------------------------------------------------------
// Gate matrix type
// ---------------------------------------------------------------------------

/// @brief A 2x2 unitary matrix representing a single-qubit gate.
///
/// Layout: gate[row][col], so for a matrix
///   | u00  u01 |
///   | u10  u11 |
/// we have gate[0][0] = u00, gate[0][1] = u01, etc.
using Gate2x2 = std::array<std::array<std::complex<double>, 2>, 2>;

// ---------------------------------------------------------------------------
// Generic single-qubit gate applicator
// ---------------------------------------------------------------------------

/// @brief Apply an arbitrary 2x2 unitary gate to a specific qubit.
///
/// Algorithm (direct amplitude manipulation):
///   For each pair of basis state indices (i0, i1) that differ only at bit `qubit`:
///     i0 has bit `qubit` = 0
///     i1 = i0 | (1 << qubit) has bit `qubit` = 1
///
///     new state[i0] = gate[0][0] * state[i0] + gate[0][1] * state[i1]
///     new state[i1] = gate[1][0] * state[i0] + gate[1][1] * state[i1]
///
/// Time complexity: O(2^n) — processes 2^(n-1) pairs, 4 complex multiplications each.
/// Space complexity: O(1) extra — modifies state in-place.
///
/// @param sv The state vector to modify (in-place).
/// @param qubit Target qubit index (0-indexed, little-endian).
/// @param gate The 2x2 unitary matrix to apply.
/// @throws std::out_of_range if qubit index is invalid.
void applySingleQubitGate(StateVector& sv, int qubit, const Gate2x2& gate);

// ---------------------------------------------------------------------------
// Gate matrix constructors
// ---------------------------------------------------------------------------

/// @brief Pauli-X (quantum NOT) matrix:
///   | 0  1 |
///   | 1  0 |
/// Swaps |0⟩ ↔ |1⟩.
Gate2x2 xMatrix();

/// @brief Pauli-Y matrix:
///   | 0  -i |
///   | i   0 |
/// Combined bit-flip and phase-flip.
Gate2x2 yMatrix();

/// @brief Pauli-Z matrix:
///   | 1   0 |
///   | 0  -1 |
/// Flips the phase of |1⟩.
Gate2x2 zMatrix();

/// @brief Hadamard matrix:
///   1/√2 | 1   1 |
///        | 1  -1 |
/// Creates equal superposition from basis states.
Gate2x2 hMatrix();

/// @brief Phase gate P(φ) matrix:
///   | 1       0      |
///   | 0   e^(iφ)     |
/// Adds a relative phase φ to the |1⟩ component.
///
/// Special cases: P(π) = Z, P(π/2) = S, P(π/4) = T.
///
/// @param phi Phase angle in radians.
Gate2x2 pMatrix(double phi);

/// @brief RX(θ) rotation matrix:
///   | cos(θ/2)    -i·sin(θ/2) |
///   | -i·sin(θ/2)  cos(θ/2)   |
/// Rotation around the X axis on the Bloch sphere.
///
/// @param theta Rotation angle in radians.
Gate2x2 rxMatrix(double theta);

/// @brief RY(θ) rotation matrix:
///   | cos(θ/2)   -sin(θ/2) |
///   | sin(θ/2)    cos(θ/2) |
/// Rotation around the Y axis on the Bloch sphere.
///
/// @param theta Rotation angle in radians.
Gate2x2 ryMatrix(double theta);

/// @brief RZ(θ) rotation matrix:
///   | e^(-iθ/2)     0       |
///   |     0      e^(iθ/2)   |
/// Rotation around the Z axis on the Bloch sphere.
///
/// @param theta Rotation angle in radians.
Gate2x2 rzMatrix(double theta);

// ---------------------------------------------------------------------------
// Named gate functions (convenient wrappers with optimizations)
// ---------------------------------------------------------------------------

/// @brief Apply Pauli-X gate to a qubit.
///
/// Optimized: swaps amplitude pairs without multiplication.
/// X|0⟩ = |1⟩, X|1⟩ = |0⟩.
void pauliX(StateVector& sv, int qubit);

/// @brief Apply Pauli-Y gate to a qubit.
///
/// Y|0⟩ = i|1⟩, Y|1⟩ = -i|0⟩.
void pauliY(StateVector& sv, int qubit);

/// @brief Apply Pauli-Z gate to a qubit.
///
/// Optimized: only negates amplitudes where target qubit is |1⟩.
/// Z|0⟩ = |0⟩, Z|1⟩ = -|1⟩.
void pauliZ(StateVector& sv, int qubit);

/// @brief Apply Hadamard gate to a qubit.
///
/// H|0⟩ = (|0⟩ + |1⟩)/√2, H|1⟩ = (|0⟩ - |1⟩)/√2.
void hadamard(StateVector& sv, int qubit);

/// @brief Apply phase gate P(φ) to a qubit.
///
/// Optimized: only multiplies amplitudes where target qubit is |1⟩.
/// P(φ)|0⟩ = |0⟩, P(φ)|1⟩ = e^(iφ)|1⟩.
///
/// @param phi Phase angle in radians.
void phaseGate(StateVector& sv, int qubit, double phi);

/// @brief Apply RX(θ) rotation to a qubit.
///
/// @param theta Rotation angle in radians.
void rx(StateVector& sv, int qubit, double theta);

/// @brief Apply RY(θ) rotation to a qubit.
///
/// @param theta Rotation angle in radians.
void ry(StateVector& sv, int qubit, double theta);

/// @brief Apply RZ(θ) rotation to a qubit.
///
/// Optimized: diagonal matrix, only multiplies individual amplitudes.
///
/// @param theta Rotation angle in radians.
void rz(StateVector& sv, int qubit, double theta);

// ---------------------------------------------------------------------------
// Controlled (multi-qubit) gates
// ---------------------------------------------------------------------------

/// @brief Apply an arbitrary controlled-U gate.
///
/// Algorithm: same pair-iteration as applySingleQubitGate, but only
/// applies the 2x2 matrix when the control qubit's bit is 1.
///
/// For each pair (i0, i1) differing at bit `target`:
///   if bit `control` of i0 is 1:
///     apply gate to (state[i0], state[i1])
///
/// Time: O(2^n). Space: O(1) extra.
///
/// @param sv State vector to modify in-place.
/// @param control Control qubit index.
/// @param target Target qubit index (must differ from control).
/// @param gate 2x2 unitary to apply to the target qubit.
/// @throws std::invalid_argument if control == target.
/// @throws std::out_of_range if qubit indices are invalid.
void applyControlledGate(StateVector& sv, int control, int target,
                         const Gate2x2& gate);

/// @brief CNOT (Controlled-X) gate.
///
/// Flips the target qubit if the control qubit is |1⟩.
/// CNOT|c,t⟩ = |c, c⊕t⟩.
///
/// Optimized: conditional swap (no multiplication).
void cnot(StateVector& sv, int control, int target);

/// @brief Controlled-Z gate.
///
/// Applies a phase flip (-1) when both qubits are |1⟩.
/// CZ is symmetric: CZ(a,b) = CZ(b,a).
///
/// Optimized: negate amplitude where both bits are 1.
void controlledZ(StateVector& sv, int qubit0, int qubit1);

/// @brief SWAP gate.
///
/// Exchanges the states of two qubits.
/// SWAP|a,b⟩ = |b,a⟩.
///
/// Implemented by directly swapping amplitudes that differ in both
/// qubit positions (not via three CNOTs, for efficiency).
void swapGate(StateVector& sv, int qubit0, int qubit1);

/// @brief Toffoli (CCX) gate — doubly-controlled X.
///
/// Flips the target qubit only when both control qubits are |1⟩.
///
/// @param control0 First control qubit.
/// @param control1 Second control qubit.
/// @param target Target qubit (flipped when both controls are |1⟩).
void toffoli(StateVector& sv, int control0, int control1, int target);

} // namespace Gates
} // namespace QuantumSim

#endif // QUANTUMSIM_GATES_HPP
