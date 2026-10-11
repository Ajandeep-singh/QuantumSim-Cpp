#include "Gates.hpp"
#include "StateVector.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

using Clock = std::chrono::steady_clock;
using Apply = void (*)(QuantumSim::StateVector&, int);

long long readTicks() {
#ifdef _WIN32
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        Clock::now().time_since_epoch()).count();
#endif
}

double ticksToNanoseconds(long long ticks) {
#ifdef _WIN32
    static const double frequency = [] {
        LARGE_INTEGER value;
        QueryPerformanceFrequency(&value);
        return static_cast<double>(value.QuadPart);
    }();
    return static_cast<double>(ticks) * 1.0e9 / frequency;
#else
    return static_cast<double>(ticks);
#endif
}

void prepareState(QuantumSim::StateVector& state) {
    for (int q = 0; q < state.getNumQubits(); ++q) {
        QuantumSim::Gates::hadamard(state, q);
    }
}

double distance(const QuantumSim::StateVector& a, const QuantumSim::StateVector& b) {
    double sum = 0.0;
    for (std::size_t i = 0; i < a.getNumAmplitudes(); ++i) {
        sum += std::norm(a.getAmplitude(i) - b.getAmplitude(i));
    }
    return std::sqrt(sum);
}

double timeRepeated(QuantumSim::StateVector& state, int target,
                    std::size_t depth, std::size_t batches, Apply apply) {
    const long long begin = readTicks();
    for (std::size_t batch = 0; batch < batches; ++batch) {
        for (std::size_t i = 0; i < depth; ++i) apply(state, target);
    }
    const long long end = readTicks();
    return ticksToNanoseconds(end - begin);
}

void genericX(QuantumSim::StateVector& state, int q) {
    QuantumSim::Gates::applySingleQubitGate(state, q, QuantumSim::Gates::xMatrix());
}
void specializedX(QuantumSim::StateVector& state, int q) {
    QuantumSim::Gates::pauliX(state, q);
}

constexpr double PHASE_ANGLE = 0.371;
void genericPhase(QuantumSim::StateVector& state, int q) {
    QuantumSim::Gates::applySingleQubitGate(state, q, QuantumSim::Gates::pMatrix(PHASE_ANGLE));
}
void specializedPhase(QuantumSim::StateVector& state, int q) {
    QuantumSim::Gates::phaseGate(state, q, PHASE_ANGLE);
}

void writeRow(std::ofstream& out, const std::string& operation,
              const std::string& implementation, int qubits, std::size_t depth,
              int repeat, std::size_t batches, double elapsed_ns,
              const QuantumSim::StateVector& state,
              double ref_error) {
    const std::size_t bytes = state.getAmplitudes().capacity()
                              * sizeof(std::complex<double>);
    out << operation << ',' << implementation << ',' << qubits << ',' << depth << ','
        << repeat << ',' << batches << ',' << elapsed_ns << ',' << bytes << ',' << std::setprecision(17)
        << std::abs(state.normSquared() - 1.0) << ',' << ref_error << '\n';
}

} // namespace

int main(int argc, char** argv) {
    try {
        const std::string output_path = argc > 1 ? argv[1] : "results/benchmark_raw.csv";
        const int max_qubits = argc > 2 ? std::stoi(argv[2]) : 16;
        const int repeats = argc > 3 ? std::stoi(argv[3]) : 3;
        if (max_qubits < 4 || max_qubits > QuantumSim::StateVector::MAX_QUBITS || repeats < 1) {
            throw std::invalid_argument("Use max-qubits 4..25 and repeats >= 1.");
        }

        std::ofstream out(output_path);
        if (!out) throw std::runtime_error("Could not open output CSV: " + output_path);
        out << "operation,implementation,qubits,depth,repeat,batch_count,elapsed_ns_per_circuit,state_bytes,norm_error,reference_l2_error\n";
        const std::vector<std::size_t> depths = {1, 10, 100};
        const struct Strategy { const char* operation; const char* implementation; Apply apply; } strategies[] = {
            {"X", "generic_matrix", genericX}, {"X", "specialized", specializedX},
            {"P", "generic_matrix", genericPhase}, {"P", "specialized", specializedPhase}
        };

        for (int qubits = 4; qubits <= max_qubits; qubits += (qubits < 10 ? 2 : 2)) {
            const int target = qubits - 1; // Exercise the strided high-bit case consistently.
            for (std::size_t depth : depths) {
                for (const Strategy& strategy : strategies) {
                    for (int repeat = 0; repeat < repeats; ++repeat) {
                        QuantumSim::StateVector reference(qubits);
                        QuantumSim::StateVector measured(qubits);
                        prepareState(reference);
                        prepareState(measured);
                        const std::size_t target_work = reference.getNumAmplitudes() * depth;
                        const std::size_t batches = std::max<std::size_t>(
                            1, 1000000 / target_work);
                        Apply reference_apply = strategy.operation[0] == 'X'
                            ? genericX : genericPhase;
                        for (std::size_t i = 0; i < depth; ++i) {
                            reference_apply(reference, target);
                        }
                        const double total_elapsed = timeRepeated(
                            measured, target, depth, batches, strategy.apply);
                        const double elapsed = total_elapsed / static_cast<double>(batches);

                        // Error metrics describe one logical circuit, not the
                        // repeated timing batch used to improve timer precision.
                        QuantumSim::StateVector quality(qubits);
                        prepareState(quality);
                        for (std::size_t i = 0; i < depth; ++i) {
                            strategy.apply(quality, target);
                        }
                        writeRow(out, strategy.operation, strategy.implementation, qubits,
                                 depth, repeat, batches, elapsed, quality,
                                 distance(reference, quality));
                    }
                }
            }
            std::cout << "Completed " << qubits << " qubits\n";
        }
        std::cout << "Wrote raw measurements to " << output_path << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Benchmark error: " << error.what() << '\n';
        return 2;
    }
}
