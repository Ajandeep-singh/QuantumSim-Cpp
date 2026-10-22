#ifndef QUANTUMSIM_CIRCUIT_HPP
#define QUANTUMSIM_CIRCUIT_HPP

#include "StateVector.hpp"

#include <cstddef>
#include <vector>

namespace QuantumSim {

/// A recorded sequence of gates applied to a state vector.
/// run() always starts from the configured computational basis state.
class Circuit {
public:
    explicit Circuit(int num_qubits);

    Circuit& initializeBasisState(std::size_t basis_index);
    Circuit& x(int qubit);
    Circuit& y(int qubit);
    Circuit& z(int qubit);
    Circuit& h(int qubit);
    Circuit& phase(int qubit, double phi);
    Circuit& rx(int qubit, double theta);
    Circuit& ry(int qubit, double theta);
    Circuit& rz(int qubit, double theta);
    Circuit& cnot(int control, int target);
    Circuit& cz(int qubit0, int qubit1);
    Circuit& swap(int qubit0, int qubit1);
    Circuit& toffoli(int control0, int control1, int target);

    void run();
    const StateVector& state() const noexcept;
    int numQubits() const noexcept;
    std::size_t depth() const noexcept;

private:
    enum class Gate { X, Y, Z, H, Phase, RX, RY, RZ, CNOT, CZ, SWAP, Toffoli };
    struct Operation {
        Gate gate;
        int q0;
        int q1;
        int q2;
        double angle;
    };

    Circuit& append(Gate gate, int q0, int q1 = -1, int q2 = -1,
                    double angle = 0.0);

    StateVector state_;
    std::size_t initial_basis_index_ = 0;
    std::vector<Operation> operations_;
};

} // namespace QuantumSim

#endif
