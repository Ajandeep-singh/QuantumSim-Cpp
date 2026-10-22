#include "Circuit.hpp"

#include "Gates.hpp"

#include <stdexcept>

namespace QuantumSim {

Circuit::Circuit(int num_qubits) : state_(num_qubits) {}

Circuit& Circuit::initializeBasisState(std::size_t basis_index) {
    if (basis_index >= state_.getNumAmplitudes()) {
        throw std::out_of_range("Initial basis index is outside the state vector.");
    }
    initial_basis_index_ = basis_index;
    return *this;
}

Circuit& Circuit::append(Gate gate, int q0, int q1, int q2, double angle) {
    operations_.push_back({gate, q0, q1, q2, angle});
    return *this;
}

Circuit& Circuit::x(int q) { return append(Gate::X, q); }
Circuit& Circuit::y(int q) { return append(Gate::Y, q); }
Circuit& Circuit::z(int q) { return append(Gate::Z, q); }
Circuit& Circuit::h(int q) { return append(Gate::H, q); }
Circuit& Circuit::phase(int q, double a) { return append(Gate::Phase, q, -1, -1, a); }
Circuit& Circuit::rx(int q, double a) { return append(Gate::RX, q, -1, -1, a); }
Circuit& Circuit::ry(int q, double a) { return append(Gate::RY, q, -1, -1, a); }
Circuit& Circuit::rz(int q, double a) { return append(Gate::RZ, q, -1, -1, a); }
Circuit& Circuit::cnot(int c, int t) { return append(Gate::CNOT, c, t); }
Circuit& Circuit::cz(int a, int b) { return append(Gate::CZ, a, b); }
Circuit& Circuit::swap(int a, int b) { return append(Gate::SWAP, a, b); }
Circuit& Circuit::toffoli(int c0, int c1, int t) { return append(Gate::Toffoli, c0, c1, t); }

void Circuit::run() {
    state_.initBasisState(initial_basis_index_);
    for (const Operation& op : operations_) {
        switch (op.gate) {
        case Gate::X: Gates::pauliX(state_, op.q0); break;
        case Gate::Y: Gates::pauliY(state_, op.q0); break;
        case Gate::Z: Gates::pauliZ(state_, op.q0); break;
        case Gate::H: Gates::hadamard(state_, op.q0); break;
        case Gate::Phase: Gates::phaseGate(state_, op.q0, op.angle); break;
        case Gate::RX: Gates::rx(state_, op.q0, op.angle); break;
        case Gate::RY: Gates::ry(state_, op.q0, op.angle); break;
        case Gate::RZ: Gates::rz(state_, op.q0, op.angle); break;
        case Gate::CNOT: Gates::cnot(state_, op.q0, op.q1); break;
        case Gate::CZ: Gates::controlledZ(state_, op.q0, op.q1); break;
        case Gate::SWAP: Gates::swapGate(state_, op.q0, op.q1); break;
        case Gate::Toffoli: Gates::toffoli(state_, op.q0, op.q1, op.q2); break;
        }
    }
}

const StateVector& Circuit::state() const noexcept { return state_; }
int Circuit::numQubits() const noexcept { return state_.getNumQubits(); }
std::size_t Circuit::depth() const noexcept { return operations_.size(); }

} // namespace QuantumSim
