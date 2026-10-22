#include "Circuit.hpp"
#include "Gates.hpp"
#include "Measurement.hpp"

#include <cmath>
#include <complex>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>

namespace {

int checks = 0;

void require(bool condition, const std::string& message) {
    ++checks;
    if (!condition) {
        throw std::runtime_error("FAIL: " + message);
    }
}

bool close(std::complex<double> a, std::complex<double> b, double tol = 1e-10) {
    return std::abs(a - b) <= tol;
}

bool close(double a, double b, double tol = 1e-10) {
    return std::abs(a - b) <= tol;
}

template <typename Exception, typename Function>
void requireThrows(Function function, const std::string& message) {
    bool threw = false;
    try { function(); } catch (const Exception&) { threw = true; }
    require(threw, message);
}

void testStateAndSingleQubitGates() {
    QuantumSim::StateVector state(1);
    QuantumSim::Gates::pauliX(state, 0);
    require(close(state.getAmplitude(0), {0.0, 0.0})
            && close(state.getAmplitude(1), {1.0, 0.0}), "X|0> = |1>");
    QuantumSim::Gates::pauliX(state, 0);
    require(close(state.getAmplitude(0), {1.0, 0.0}), "XX = I");

    QuantumSim::Gates::hadamard(state, 0);
    const double s = 1.0 / std::sqrt(2.0);
    require(close(state.getAmplitude(0), {s, 0.0})
            && close(state.getAmplitude(1), {s, 0.0}), "H creates equal superposition");
    require(close(state.probability(0), 0.5) && close(state.probability(1), 0.5),
            "Born probabilities after H");
    QuantumSim::Gates::hadamard(state, 0);
    require(close(state.getAmplitude(0), {1.0, 0.0}), "HH = I");

    QuantumSim::Gates::pauliY(state, 0);
    require(close(state.getAmplitude(1), {0.0, 1.0}), "Y|0> = i|1>");
    QuantumSim::Gates::pauliY(state, 0);
    require(close(state.getAmplitude(0), {1.0, 0.0}), "YY = I");

    QuantumSim::Gates::pauliZ(state, 0);
    require(close(state.getAmplitude(0), {1.0, 0.0}), "Z|0> = |0>");
    QuantumSim::Gates::pauliX(state, 0);
    QuantumSim::Gates::pauliZ(state, 0);
    require(close(state.getAmplitude(1), {-1.0, 0.0}), "Z|1> = -|1>");

    state.initBasisState(1);
    QuantumSim::Gates::phaseGate(state, 0, 3.14159265358979323846);
    require(close(state.getAmplitude(1), {-1.0, 0.0}), "P(pi) = Z on |1>");
    state.initBasisState(0);
    QuantumSim::Gates::rx(state, 0, 3.14159265358979323846);
    require(close(state.getAmplitude(1), {0.0, -1.0}), "RX(pi)|0> = -i|1>");
    state.initBasisState(0);
    QuantumSim::Gates::ry(state, 0, 3.14159265358979323846);
    require(close(state.getAmplitude(1), {1.0, 0.0}), "RY(pi)|0> = |1>");
    state.initBasisState(0);
    QuantumSim::Gates::rz(state, 0, 3.14159265358979323846);
    require(close(state.getAmplitude(0), {0.0, -1.0}), "RZ(pi)|0> = -i|0>");
    require(state.isNormalized(), "single-qubit gates preserve normalization");
}

void testControlledGates() {
    QuantumSim::Circuit bell(2);
    bell.h(0).cnot(0, 1).run();
    const double s = 1.0 / std::sqrt(2.0);
    const auto& state = bell.state();
    require(close(state.getAmplitude(0), {s, 0.0})
            && close(state.getAmplitude(3), {s, 0.0}), "Bell state amplitudes");
    require(close(state.probability(0), 0.5) && close(state.probability(3), 0.5)
            && close(state.probability(1), 0.0) && close(state.probability(2), 0.0),
            "Bell state measurement probabilities");

    QuantumSim::StateVector cnot_state(2);
    cnot_state.initBasisState(1); // q0 = 1; control q0 flips target q1.
    QuantumSim::Gates::cnot(cnot_state, 0, 1);
    require(close(cnot_state.getAmplitude(3), {1.0, 0.0}), "CNOT respects q0 control bit");
    QuantumSim::Gates::cnot(cnot_state, 0, 1);
    require(close(cnot_state.getAmplitude(1), {1.0, 0.0}), "CNOT squared is identity");

    QuantumSim::StateVector controlled_state(2);
    controlled_state.initBasisState(1);
    QuantumSim::Gates::applyControlledGate(controlled_state, 0, 1,
                                           QuantumSim::Gates::hMatrix());
    require(close(controlled_state.getAmplitude(1), {s, 0.0})
            && close(controlled_state.getAmplitude(3), {s, 0.0}),
            "generic controlled-H acts only when the control bit is one");

    QuantumSim::StateVector cz_state(2);
    cz_state.initBasisState(3);
    QuantumSim::Gates::controlledZ(cz_state, 0, 1);
    require(close(cz_state.getAmplitude(3), {-1.0, 0.0}), "CZ applies phase to |11>");

    QuantumSim::StateVector swap_state(2);
    swap_state.initBasisState(1);
    QuantumSim::Gates::swapGate(swap_state, 0, 1);
    require(close(swap_state.getAmplitude(2), {1.0, 0.0}), "SWAP exchanges basis bits");

    QuantumSim::StateVector toffoli_state(3);
    toffoli_state.initBasisState(3);
    QuantumSim::Gates::toffoli(toffoli_state, 0, 1, 2);
    require(close(toffoli_state.getAmplitude(7), {1.0, 0.0}), "Toffoli flips target when both controls are one");
}

void testMeasurementAndCircuit() {
    QuantumSim::Circuit circuit(2);
    circuit.initializeBasisState(2).x(0).x(0);
    circuit.run();
    require(close(circuit.state().getAmplitude(2), {1.0, 0.0}),
            "Circuit starts from configured basis state");
    circuit.run();
    require(close(circuit.state().getAmplitude(2), {1.0, 0.0}), "Circuit run is repeatable");

    QuantumSim::Circuit bell(2);
    bell.h(0).cnot(0, 1).run();
    const auto first = QuantumSim::Measurement::sampleShots(bell.state(), 2000, 123456);
    const auto second = QuantumSim::Measurement::sampleShots(bell.state(), 2000, 123456);
    require(first == second, "measurement sampling is deterministic for a fixed seed");
    std::size_t total = 0;
    for (const auto& item : first) total += item.second;
    require(total == 2000, "measurement returns exactly the requested shot count");
    require(first.size() == 2 && first.count("00") == 1 && first.count("11") == 1,
            "Bell measurement has only correlated outcomes");
    const auto probs = QuantumSim::Measurement::probabilities(bell.state());
    require(probs.size() == 4 && close(probs[0] + probs[3], 1.0),
            "probabilities are returned in basis-index order");
}

void testRandomizedCircuits() {
    constexpr unsigned seed = 20240517;
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> gate_choice(0, 7);
    std::uniform_int_distribution<int> qubit_choice(0, 3);
    std::uniform_real_distribution<double> angle(-6.0, 6.0);

    for (int trial = 0; trial < 50; ++trial) {
        QuantumSim::StateVector state(4);
        for (int depth = 0; depth < 100; ++depth) {
            int q0 = qubit_choice(generator);
            int q1 = qubit_choice(generator);
            if (q1 == q0) q1 = (q1 + 1) % 4;
            switch (gate_choice(generator)) {
            case 0: QuantumSim::Gates::hadamard(state, q0); break;
            case 1: QuantumSim::Gates::pauliX(state, q0); break;
            case 2: QuantumSim::Gates::pauliY(state, q0); break;
            case 3: QuantumSim::Gates::pauliZ(state, q0); break;
            case 4: QuantumSim::Gates::rx(state, q0, angle(generator)); break;
            case 5: QuantumSim::Gates::ry(state, q0, angle(generator)); break;
            case 6: QuantumSim::Gates::rz(state, q0, angle(generator)); break;
            default: QuantumSim::Gates::cnot(state, q0, q1); break;
            }
            require(state.isNormalized(2e-10), "random circuit normalization (seed 20240517)");
        }
    }
}

void testErrors() {
    requireThrows<std::invalid_argument>([] { QuantumSim::StateVector invalid(0); },
                                         "zero-qubit state rejected");
    requireThrows<std::out_of_range>([] { QuantumSim::StateVector(2).getAmplitude(4); },
                                     "out-of-range amplitude rejected");
    requireThrows<std::out_of_range>([] {
        QuantumSim::StateVector state(2); QuantumSim::Gates::hadamard(state, 2);
    }, "out-of-range gate target rejected");
    requireThrows<std::invalid_argument>([] {
        QuantumSim::StateVector state(2); QuantumSim::Gates::cnot(state, 0, 0);
    }, "identical CNOT control and target rejected");
    requireThrows<std::invalid_argument>([] {
        QuantumSim::StateVector state(2); state.setAmplitude(0, {2.0, 0.0});
        (void)QuantumSim::Measurement::probabilities(state);
    }, "measurement rejects non-normalized states");
    requireThrows<std::invalid_argument>([] {
        QuantumSim::StateVector state(1);
        auto invalid_gate = QuantumSim::Gates::xMatrix();
        invalid_gate[0][0] = {2.0, 0.0};
        QuantumSim::Gates::applySingleQubitGate(state, 0, invalid_gate);
    }, "generic gate rejects non-unitary matrices");
    requireThrows<std::invalid_argument>([] {
        QuantumSim::StateVector state(1);
        QuantumSim::Gates::phaseGate(state, 0, std::numeric_limits<double>::infinity());
    }, "phase gate rejects non-finite angle");
}

} // namespace

int main() {
    try {
        testStateAndSingleQubitGates();
        testControlledGates();
        testMeasurementAndCircuit();
        testRandomizedCircuits();
        testErrors();
        std::cout << "PASS: " << checks << " validation checks. Random seed: 20240517.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
