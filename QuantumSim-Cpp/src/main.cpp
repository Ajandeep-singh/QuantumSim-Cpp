#include "Circuit.hpp"
#include "Measurement.hpp"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

std::size_t parseSize(const char* value, const char* name) {
    const std::string text(value);
    if (text.empty() || text[0] == '-') {
        throw std::invalid_argument(std::string(name) + " must be a non-negative integer.");
    }
    std::size_t parsed = 0;
    const unsigned long long number = std::stoull(text, &parsed);
    if (parsed != text.size() || number > std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument(std::string("Invalid ") + name + ".");
    }
    return static_cast<std::size_t>(number);
}

void printUsage(const char* program) {
    std::cout << "QuantumSim-C++ state-vector simulator\n\n"
              << "Usage:\n"
              << "  " << program << " bell [shots] [seed]\n"
              << "  " << program << " ghz <qubits> [shots] [seed] [--allow-large]\n"
              << "  " << program << " basis <qubits> <basis-index> [--allow-large]\n\n"
              << "Qubit 0 is the least-significant index bit. Shots default to 0; "
              << "seed defaults to 42. Requests above 20 qubits require --allow-large.\n";
}

void printState(const QuantumSim::StateVector& state) {
    std::cout << std::setprecision(10)
              << "Final state (non-zero amplitudes; basis labels use q0 at right):\n";
    for (std::size_t i = 0; i < state.getNumAmplitudes(); ++i) {
        const auto amp = state.getAmplitude(i);
        if (std::norm(amp) < 1e-24) {
            continue;
        }
        std::cout << "  |" << state.basisLabel(i) << "> index=" << i
                  << " amplitude=" << amp << " probability=" << state.probability(i) << '\n';
    }
    std::cout << "Norm squared: " << state.normSquared() << '\n';
}

void printMemoryEstimate(std::size_t qubits, bool sampling) {
    const std::size_t amplitudes = static_cast<std::size_t>(1) << qubits;
    const std::size_t state_bytes = amplitudes * sizeof(std::complex<double>);
    const std::size_t sampling_bytes = sampling ? amplitudes * sizeof(double) : 0;
    std::cout << "Estimated known buffers before allocation: "
              << std::fixed << std::setprecision(2)
              << static_cast<double>(state_bytes + sampling_bytes) / (1024.0 * 1024.0)
              << " MiB (state vector"
              << (sampling ? " plus probability buffer" : "") << ").\n";
    if (qubits >= 21) {
        std::cout << "Note: this is a large allocation; the system may not have enough free memory.\n";
    }
}

void guardLargeAllocation(std::size_t qubits, bool sampling, int argc, char** argv) {
    if (qubits <= 20) {
        return;
    }
    printMemoryEstimate(qubits, sampling);
    bool allowed = false;
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--allow-large") allowed = true;
    }
    if (!allowed) {
        throw std::invalid_argument("Requests above 20 qubits are blocked by default; "
                                    "pass --allow-large after reviewing the memory estimate.");
    }
}

void maybeSample(const QuantumSim::StateVector& state, std::size_t shots,
                 std::uint64_t seed) {
    if (shots == 0) {
        return;
    }
    std::cout << "\nMeasurement counts (shots=" << shots << ", seed=" << seed << "):\n";
    for (const auto& entry : QuantumSim::Measurement::sampleShots(state, shots, seed)) {
        std::cout << "  |" << entry.first << "> " << entry.second << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
            printUsage(argv[0]);
            return 0;
        }

        const std::string command(argv[1]);
        QuantumSim::Circuit circuit(command == "bell" ? 2 : 1);
        std::size_t shots = 0;
        std::uint64_t seed = 42;

        if (command == "bell") {
            if (argc > 4) throw std::invalid_argument("Too many arguments for bell.");
            if (argc >= 3) shots = parseSize(argv[2], "shots");
            if (argc >= 4) seed = parseSize(argv[3], "seed");
            circuit.h(0).cnot(0, 1);
            std::cout << "Circuit: Bell state H(q0), CNOT(q0 -> q1)\n";
        } else if (command == "ghz") {
            if (argc < 3 || argc > 6) throw std::invalid_argument("ghz requires qubits [shots] [seed] [--allow-large].");
            if (argc == 6 && (std::string(argv[4]) == "--allow-large"
                              || std::string(argv[5]) != "--allow-large")) {
                throw std::invalid_argument("Use --allow-large only as the final GHZ argument.");
            }
            const std::size_t qubits = parseSize(argv[2], "qubits");
            if (qubits < 2 || qubits > QuantumSim::StateVector::MAX_QUBITS) {
                throw std::invalid_argument("GHZ qubits must be between 2 and the simulator limit.");
            }
            if (argc >= 4) shots = parseSize(argv[3], "shots");
            if (argc == 5 && std::string(argv[4]) != "--allow-large") seed = parseSize(argv[4], "seed");
            if (argc == 6) seed = parseSize(argv[4], "seed");
            guardLargeAllocation(qubits, shots > 0, argc, argv);
            circuit = QuantumSim::Circuit(static_cast<int>(qubits));
            circuit.h(0);
            for (std::size_t q = 1; q < qubits; ++q) {
                circuit.cnot(0, static_cast<int>(q));
            }
            std::cout << "Circuit: " << qubits << "-qubit GHZ state\n";
        } else if (command == "basis") {
            if (argc < 4 || argc > 5) throw std::invalid_argument("basis requires qubits, basis-index, and optional --allow-large.");
            if (argc == 5 && std::string(argv[4]) != "--allow-large") {
                throw std::invalid_argument("The only optional basis argument is --allow-large.");
            }
            const std::size_t qubits = parseSize(argv[2], "qubits");
            if (qubits < 1 || qubits > QuantumSim::StateVector::MAX_QUBITS) {
                throw std::invalid_argument("Qubits must be between 1 and the simulator limit.");
            }
            guardLargeAllocation(qubits, false, argc, argv);
            circuit = QuantumSim::Circuit(static_cast<int>(qubits));
            circuit.initializeBasisState(parseSize(argv[3], "basis-index"));
            std::cout << "Circuit: computational basis initialization\n";
        } else {
            printUsage(argv[0]);
            return 2;
        }

        circuit.run();
        printState(circuit.state());
        maybeSample(circuit.state(), shots, seed);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 2;
    }
}
