#include "Gates.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace QuantumSim {
namespace Gates {

namespace {

void validateUnitary(const Gate2x2& gate) {
    for (const auto& row : gate) {
        for (const auto& value : row) {
            if (!std::isfinite(value.real()) || !std::isfinite(value.imag())) {
                throw std::invalid_argument("Gate matrix entries must be finite.");
            }
        }
    }
    for (std::size_t column0 = 0; column0 < 2; ++column0) {
        for (std::size_t column1 = 0; column1 < 2; ++column1) {
            std::complex<double> inner{0.0, 0.0};
            for (std::size_t row = 0; row < 2; ++row) {
                inner += std::conj(gate[row][column0]) * gate[row][column1];
            }
            const std::complex<double> expected = column0 == column1
                ? std::complex<double>{1.0, 0.0} : std::complex<double>{0.0, 0.0};
            if (std::abs(inner - expected) > 1e-12) {
                throw std::invalid_argument("Gate matrix must be unitary.");
            }
        }
    }
}

void validateAngle(double angle) {
    if (!std::isfinite(angle)) {
        throw std::invalid_argument("Gate angle must be finite.");
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Generic single-qubit gate applicator
// ---------------------------------------------------------------------------

void applySingleQubitGate(StateVector& sv, int qubit, const Gate2x2& gate) {
    validateUnitary(gate);
    // Validate qubit index.
    if (qubit < 0 || qubit >= sv.getNumQubits()) {
        throw std::out_of_range(
            "Qubit index " + std::to_string(qubit)
            + " is out of range for " + std::to_string(sv.getNumQubits())
            + "-qubit state (valid: 0 to "
            + std::to_string(sv.getNumQubits() - 1) + ")."
        );
    }

    auto& amps = sv.getAmplitudes();
    const std::size_t n = sv.getNumAmplitudes();

    // The bit mask for the target qubit position.
    // For qubit q, this is 2^q.
    const std::size_t qubit_mask = static_cast<std::size_t>(1) << qubit;

    // Iterate over all basis state indices.
    // We only process indices where bit `qubit` is 0 (i.e., i0).
    // The partner index i1 = i0 | qubit_mask has bit `qubit` set to 1.
    //
    // This ensures each pair (i0, i1) is processed exactly once.
    for (std::size_t i = 0; i < n; ++i) {
        // Skip if bit `qubit` of i is already 1 — we handle this index
        // as the partner of some earlier i0.
        if (i & qubit_mask) {
            continue;
        }

        const std::size_t i0 = i;
        const std::size_t i1 = i | qubit_mask;

        // Read current amplitudes.
        const std::complex<double> a0 = amps[i0];
        const std::complex<double> a1 = amps[i1];

        // Apply the 2x2 matrix:
        //   | a0' |   | gate[0][0]  gate[0][1] | | a0 |
        //   | a1' | = | gate[1][0]  gate[1][1] | | a1 |
        amps[i0] = gate[0][0] * a0 + gate[0][1] * a1;
        amps[i1] = gate[1][0] * a0 + gate[1][1] * a1;
    }
}

// ---------------------------------------------------------------------------
// Gate matrix constructors
// ---------------------------------------------------------------------------

Gate2x2 xMatrix() {
    // Pauli-X: | 0  1 |
    //          | 1  0 |
    return {{
        {{{0.0, 0.0}, {1.0, 0.0}}},
        {{{1.0, 0.0}, {0.0, 0.0}}}
    }};
}

Gate2x2 yMatrix() {
    // Pauli-Y: |  0  -i |
    //          |  i   0 |
    return {{
        {{  {0.0, 0.0}, {0.0, -1.0} }},
        {{  {0.0, 1.0}, {0.0,  0.0} }}
    }};
}

Gate2x2 zMatrix() {
    // Pauli-Z: | 1   0 |
    //          | 0  -1 |
    return {{
        {{{1.0, 0.0}, {0.0, 0.0}}},
        {{{0.0, 0.0}, {-1.0, 0.0}}}
    }};
}

Gate2x2 hMatrix() {
    // Hadamard: 1/√2 | 1   1 |
    //                | 1  -1 |
    const double s = 1.0 / std::sqrt(2.0);
    return {{
        {{{s, 0.0}, {s, 0.0}}},
        {{{s, 0.0}, {-s, 0.0}}}
    }};
}

Gate2x2 pMatrix(double phi) {
    // Phase: | 1       0      |
    //        | 0   e^(iφ)     |
    return {{
        {{{1.0, 0.0}, {0.0, 0.0}}},
        {{{0.0, 0.0}, std::polar(1.0, phi)}}
    }};
}

Gate2x2 rxMatrix(double theta) {
    // RX(θ): | cos(θ/2)    -i·sin(θ/2) |
    //        | -i·sin(θ/2)  cos(θ/2)   |
    const double c = std::cos(theta / 2.0);
    const double s = std::sin(theta / 2.0);
    return {{
        {{{c, 0.0}, {0.0, -s}}},
        {{{0.0, -s}, {c, 0.0}}}
    }};
}

Gate2x2 ryMatrix(double theta) {
    // RY(θ): | cos(θ/2)   -sin(θ/2) |
    //        | sin(θ/2)    cos(θ/2) |
    const double c = std::cos(theta / 2.0);
    const double s = std::sin(theta / 2.0);
    return {{
        {{{c, 0.0}, {-s, 0.0}}},
        {{{s, 0.0}, {c, 0.0}}}
    }};
}

Gate2x2 rzMatrix(double theta) {
    // RZ(θ): | e^(-iθ/2)     0       |
    //        |     0      e^(iθ/2)   |
    return {{
        {{std::polar(1.0, -theta / 2.0), {0.0, 0.0}}},
        {{{0.0, 0.0}, std::polar(1.0, theta / 2.0)}}
    }};
}

// ---------------------------------------------------------------------------
// Named gate functions with optimizations
// ---------------------------------------------------------------------------

void pauliX(StateVector& sv, int qubit) {
    // Optimization: X just swaps amplitude pairs — no multiplication needed.
    //
    // For each pair (i0, i1) differing at bit `qubit`:
    //   | 0  1 | | a0 |   | a1 |
    //   | 1  0 | | a1 | = | a0 |
    //
    // So: swap(state[i0], state[i1]).

    if (qubit < 0 || qubit >= sv.getNumQubits()) {
        throw std::out_of_range(
            "Qubit index " + std::to_string(qubit)
            + " is out of range for " + std::to_string(sv.getNumQubits())
            + "-qubit state."
        );
    }

    auto& amps = sv.getAmplitudes();
    const std::size_t n = sv.getNumAmplitudes();
    const std::size_t qubit_mask = static_cast<std::size_t>(1) << qubit;

    for (std::size_t i = 0; i < n; ++i) {
        if (i & qubit_mask) continue; // only process i0 (bit q = 0)

        const std::size_t i0 = i;
        const std::size_t i1 = i | qubit_mask;

        std::swap(amps[i0], amps[i1]);
    }
}

void pauliY(StateVector& sv, int qubit) {
    // Y = | 0  -i |
    //     | i   0 |
    // Uses the generic path.
    applySingleQubitGate(sv, qubit, yMatrix());
}

void pauliZ(StateVector& sv, int qubit) {
    // Optimization: Z is diagonal — only negates amplitudes where qubit is |1⟩.
    //
    //   | 1   0 | | a0 |   |  a0 |
    //   | 0  -1 | | a1 | = | -a1 |
    //
    // So: state[i1] = -state[i1], leave state[i0] unchanged.

    if (qubit < 0 || qubit >= sv.getNumQubits()) {
        throw std::out_of_range(
            "Qubit index " + std::to_string(qubit)
            + " is out of range for " + std::to_string(sv.getNumQubits())
            + "-qubit state."
        );
    }

    auto& amps = sv.getAmplitudes();
    const std::size_t n = sv.getNumAmplitudes();
    const std::size_t qubit_mask = static_cast<std::size_t>(1) << qubit;

    for (std::size_t i = 0; i < n; ++i) {
        if (i & qubit_mask) {
            amps[i] = -amps[i];
        }
    }
}

void hadamard(StateVector& sv, int qubit) {
    // H = 1/√2 | 1   1 |
    //          | 1  -1 |
    // Uses the generic path.
    applySingleQubitGate(sv, qubit, hMatrix());
}

void phaseGate(StateVector& sv, int qubit, double phi) {
    // Optimization: P(φ) is diagonal — only multiplies amplitudes where qubit is |1⟩.
    //
    //   | 1       0      | | a0 |   |      a0       |
    //   | 0   e^(iφ)     | | a1 | = | e^(iφ) · a1  |
    //
    // So: state[i1] *= e^(iφ), leave state[i0] unchanged.

    validateAngle(phi);
    if (qubit < 0 || qubit >= sv.getNumQubits()) {
        throw std::out_of_range(
            "Qubit index " + std::to_string(qubit)
            + " is out of range for " + std::to_string(sv.getNumQubits())
            + "-qubit state."
        );
    }

    auto& amps = sv.getAmplitudes();
    const std::size_t n = sv.getNumAmplitudes();
    const std::size_t qubit_mask = static_cast<std::size_t>(1) << qubit;

    const std::complex<double> phase_factor = std::polar(1.0, phi);

    for (std::size_t i = 0; i < n; ++i) {
        if (i & qubit_mask) {
            amps[i] *= phase_factor;
        }
    }
}

void rx(StateVector& sv, int qubit, double theta) {
    // RX(θ) uses the generic path.
    applySingleQubitGate(sv, qubit, rxMatrix(theta));
}

void ry(StateVector& sv, int qubit, double theta) {
    // RY(θ) uses the generic path.
    applySingleQubitGate(sv, qubit, ryMatrix(theta));
}

void rz(StateVector& sv, int qubit, double theta) {
    // Optimization: RZ(θ) is diagonal.
    //
    //   | e^(-iθ/2)     0       | | a0 |   | e^(-iθ/2) · a0 |
    //   |     0      e^(iθ/2)   | | a1 | = | e^(iθ/2)  · a1 |
    //
    // Each amplitude is multiplied by a phase factor depending on
    // whether the target qubit is |0⟩ or |1⟩.

    validateAngle(theta);
    if (qubit < 0 || qubit >= sv.getNumQubits()) {
        throw std::out_of_range(
            "Qubit index " + std::to_string(qubit)
            + " is out of range for " + std::to_string(sv.getNumQubits())
            + "-qubit state."
        );
    }

    auto& amps = sv.getAmplitudes();
    const std::size_t n = sv.getNumAmplitudes();
    const std::size_t qubit_mask = static_cast<std::size_t>(1) << qubit;

    const std::complex<double> phase0 = std::polar(1.0, -theta / 2.0);
    const std::complex<double> phase1 = std::polar(1.0,  theta / 2.0);

    for (std::size_t i = 0; i < n; ++i) {
        if (i & qubit_mask) {
            amps[i] *= phase1;
        } else {
            amps[i] *= phase0;
        }
    }
}

void applyControlledGate(StateVector& sv, int control, int target,
                         const Gate2x2& gate) {
    validateUnitary(gate);
    if (control < 0 || control >= sv.getNumQubits()
        || target < 0 || target >= sv.getNumQubits()) {
        throw std::out_of_range("Controlled gate qubit index is out of range.");
    }
    if (control == target) {
        throw std::invalid_argument("Control and target qubits must differ.");
    }

    auto& amps = sv.getAmplitudes();
    const std::size_t control_mask = static_cast<std::size_t>(1) << control;
    const std::size_t target_mask = static_cast<std::size_t>(1) << target;

    for (std::size_t i0 = 0; i0 < amps.size(); ++i0) {
        if ((i0 & target_mask) != 0 || (i0 & control_mask) == 0) {
            continue;
        }
        const std::size_t i1 = i0 | target_mask;
        const std::complex<double> a0 = amps[i0];
        const std::complex<double> a1 = amps[i1];
        amps[i0] = gate[0][0] * a0 + gate[0][1] * a1;
        amps[i1] = gate[1][0] * a0 + gate[1][1] * a1;
    }
}

void cnot(StateVector& sv, int control, int target) {
    if (control < 0 || control >= sv.getNumQubits()
        || target < 0 || target >= sv.getNumQubits()) {
        throw std::out_of_range("CNOT qubit index is out of range.");
    }
    if (control == target) {
        throw std::invalid_argument("CNOT control and target must differ.");
    }

    auto& amps = sv.getAmplitudes();
    const std::size_t control_mask = static_cast<std::size_t>(1) << control;
    const std::size_t target_mask = static_cast<std::size_t>(1) << target;
    for (std::size_t i = 0; i < amps.size(); ++i) {
        if ((i & control_mask) != 0 && (i & target_mask) == 0) {
            std::swap(amps[i], amps[i | target_mask]);
        }
    }
}

void controlledZ(StateVector& sv, int qubit0, int qubit1) {
    if (qubit0 < 0 || qubit0 >= sv.getNumQubits()
        || qubit1 < 0 || qubit1 >= sv.getNumQubits()) {
        throw std::out_of_range("Controlled-Z qubit index is out of range.");
    }
    if (qubit0 == qubit1) {
        throw std::invalid_argument("Controlled-Z qubits must differ.");
    }

    auto& amps = sv.getAmplitudes();
    const std::size_t mask0 = static_cast<std::size_t>(1) << qubit0;
    const std::size_t mask1 = static_cast<std::size_t>(1) << qubit1;
    for (std::size_t i = 0; i < amps.size(); ++i) {
        if ((i & mask0) != 0 && (i & mask1) != 0) {
            amps[i] = -amps[i];
        }
    }
}

void swapGate(StateVector& sv, int qubit0, int qubit1) {
    if (qubit0 < 0 || qubit0 >= sv.getNumQubits()
        || qubit1 < 0 || qubit1 >= sv.getNumQubits()) {
        throw std::out_of_range("SWAP qubit index is out of range.");
    }
    if (qubit0 == qubit1) {
        throw std::invalid_argument("SWAP qubits must differ.");
    }

    auto& amps = sv.getAmplitudes();
    const std::size_t mask0 = static_cast<std::size_t>(1) << qubit0;
    const std::size_t mask1 = static_cast<std::size_t>(1) << qubit1;
    for (std::size_t i = 0; i < amps.size(); ++i) {
        const bool bit0 = (i & mask0) != 0;
        const bool bit1 = (i & mask1) != 0;
        if (bit0 && !bit1) {
            std::swap(amps[i], amps[i ^ (mask0 | mask1)]);
        }
    }
}

void toffoli(StateVector& sv, int control0, int control1, int target) {
    const int qubits[] = {control0, control1, target};
    for (int qubit : qubits) {
        if (qubit < 0 || qubit >= sv.getNumQubits()) {
            throw std::out_of_range("Toffoli qubit index is out of range.");
        }
    }
    if (control0 == control1 || control0 == target || control1 == target) {
        throw std::invalid_argument("Toffoli qubits must be distinct.");
    }

    auto& amps = sv.getAmplitudes();
    const std::size_t control0_mask = static_cast<std::size_t>(1) << control0;
    const std::size_t control1_mask = static_cast<std::size_t>(1) << control1;
    const std::size_t target_mask = static_cast<std::size_t>(1) << target;
    for (std::size_t i = 0; i < amps.size(); ++i) {
        if ((i & control0_mask) != 0 && (i & control1_mask) != 0
            && (i & target_mask) == 0) {
            std::swap(amps[i], amps[i | target_mask]);
        }
    }
}

} // namespace Gates
} // namespace QuantumSim
