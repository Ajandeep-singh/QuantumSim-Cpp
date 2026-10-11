#include "StateVector.hpp"

#include <iostream>

int main() {
    // Two qubits in the computational basis state |00>.
    const QuantumSim::StateVector state(2);

    state.print();

    const double norm_squared = state.normSquared();
    std::cout << "\nNormalization check: sum(|a_i|^2) = "
              << norm_squared << '\n';

    if (!state.isNormalized()) {
        std::cerr << "ERROR: state is not normalized.\n";
        return 1;
    }

    std::cout << "PASS: state is normalized.\n";
    return 0;
}
