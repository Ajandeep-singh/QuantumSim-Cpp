# QuantumSim-C++

A small, research-oriented state-vector simulator written in C++17. It stores pure states as `std::complex<double>` amplitudes and applies gates directly to amplitude pairs. The project is intended for learning and controlled experiments; it is not positioned as a replacement for optimized production simulators.

## Features

- Computational-basis state initialization and state-vector inspection.
- Pauli X/Y/Z, Hadamard, phase P(phi), RX/RY/RZ, CNOT, CZ, SWAP, Toffoli, and generic controlled 2x2 unitary gates.
- Reusable circuits with deterministic re-execution from their configured initial basis state.
- Born probabilities and seeded computational-basis sampling over repeated shots.
- Bell/GHZ and basis-state CLI examples.
- Automated analytical, error-handling, seeded-random circuit, and normalization checks.
- Reproducible benchmark executable, raw CSV, grouped summary, and SVG plots.

## State convention and mathematics

For n qubits, the pure state is

`|psi> = sum(i=0..2^n-1) a[i] |i>` with `sum |a[i]|^2 = 1`.

The probability of measuring basis index i is `|a[i]|^2` (Born's rule). The simulator uses little-endian indexing: qubit 0 is the least significant bit. For index i, qubit q is 1 exactly when `(i >> q) & 1` is nonzero. Labels print the most significant bit at the left and q0 at the right:

| Index | Label | q1 | q0 |
|---:|:---:|---:|---:|
| 0 | `00` | 0 | 0 |
| 1 | `01` | 0 | 1 |
| 2 | `10` | 1 | 0 |
| 3 | `11` | 1 | 1 |

A state vector has `2^n` amplitudes. With 16-byte `std::complex<double>`, amplitude storage is `16 * 2^n` bytes: 20 qubits need 16 MiB, 25 need 512 MiB, and 30 would need 16 GiB. The implementation accepts 1 through 25 qubits as a hard upper bound; allocation can still fail earlier depending on available memory. Constructing and printing the full vector is O(2^n). A single-qubit gate usually updates O(2^n) amplitudes and uses O(1) extra memory; diagonal and permutation specializations avoid the generic 2x2 arithmetic.

The Bell-state circuit `H(q0); CNOT(q0,q1)` produces `( |00> + |11> ) / sqrt(2)`. Its nonzero outcomes are correlated, which demonstrates entanglement: its amplitudes cannot be represented as a product of two single-qubit state vectors.

## Build

The CLI blocks requests above 20 qubits by default and estimates known vector and probability-buffer storage before an opted-in allocation. Standard-library sampler workspace and allocator overhead are implementation-dependent. The library still has a hard limit of 25 qubits.

### CMake (recommended)

Requires CMake 3.16 or newer and a C++17 compiler.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Direct MinGW g++ build

From this directory, compile the command-line program:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include src/main.cpp src/StateVector.cpp src/Gates.cpp src/Circuit.cpp src/Measurement.cpp -o quantumsim.exe
```

Compile and run the automated validation suite:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include tests/test_quantumsim.cpp src/StateVector.cpp src/Gates.cpp src/Circuit.cpp src/Measurement.cpp -o quantumsim_tests.exe
.\quantumsim_tests.exe
```

Compile the benchmark:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include benchmarks/benchmark.cpp src/StateVector.cpp src/Gates.cpp src/Circuit.cpp src/Measurement.cpp -o quantumsim_benchmarks.exe
```

## CLI examples

```powershell
.\quantumsim.exe --help
.\quantumsim.exe bell 10000 17
.\quantumsim.exe ghz 8 1000 17
.\quantumsim.exe basis 4 9
```

`bell` accepts optional shots and seed. `ghz` requires a qubit count and accepts optional shots and seed. `basis` initializes an arbitrary computational basis index. The CLI prints nonzero amplitudes and their Born probabilities. Sampling uses an independent seeded pseudo-random draw for each shot; it does not collapse or mutate the stored state. Requests above 20 qubits are rejected before allocation unless `--allow-large` is provided; the CLI prints an estimated memory requirement first. For example, `ghz 21 0 42 --allow-large` opts in to a 21-qubit state with no shot-sampling buffer.

## Validation

The self-contained test executable checks X, Y, Z, H, phase and rotation gates; inverse identities; Bell state amplitudes and probabilities; CNOT control/target bit order, generic controlled-H, CZ, SWAP and Toffoli behavior; seeded shot reproducibility; invalid qubits, non-unitary matrices and non-normalized measurement input; and 50 reproducible 100-gate random circuits (seed `20240517`). It checks normalization after every random gate.

## Benchmark experiment

Timing batches reduce timer noise, while numerical errors are measured after one logical circuit rather than the entire batch. The generic matrix path checks its 2x2 matrix for unitarity on every call, so its measured runtime includes that fixed validation cost.

Research question: how do generic matrix application and specialized amplitude operations compare in runtime and numerical behavior as state size and repeated gate depth increase?

The benchmark builds a deterministic uniform superposition with H gates, then applies either X or P(0.371) to the highest-index qubit. For each qubit count (4 through 18 by 2), gate depth (1, 10, 100), implementation (generic 2x2 matrix or specialized), and repeat (3), it records average elapsed nanoseconds per circuit, amplitude-vector capacity in bytes, absolute norm drift, and L2 difference from the generic reference result. It batches short measurements to reduce timer noise. Timings use QueryPerformanceCounter on Windows and steady_clock elsewhere. Results are machine- and compiler-specific.

Reproduce the checked-in measurements from PowerShell:

```powershell
.\quantumsim_benchmarks.exe results/benchmark_raw.csv 18 3
powershell -ExecutionPolicy Bypass -File benchmarks/summarize_results.ps1
powershell -ExecutionPolicy Bypass -File benchmarks/plot_results.ps1
```

The raw observations are in `results/benchmark_raw.csv`; medians and error maxima are in `results/benchmark_summary.csv`. Four SVG graphs are in `results/figures/`. Experiment setup and machine/compiler details are in `results/README.md`.

The research report source and rendered deliverables are `report/QuantumSim_Research_Report.md`, `report/QuantumSim_Research_Report.html`, and `report/QuantumSim_Research_Report.pdf`. A concise, qualified CV bullet is in `report/CV_Bullet.md`. Rebuild the HTML and PDF with:

```powershell
powershell -ExecutionPolicy Bypass -File report/build_html.ps1
powershell -ExecutionPolicy Bypass -File report/build_pdf.ps1
```

On the recorded Windows / MinGW GCC 6.3.0 run, at 18 qubits and depth 100 the median generic/specialized runtimes were 410.30/24.38 ms for X and 405.20/105.69 ms for P. Both implementations produced identical states at the stored double-precision comparison (`L2 error 0` in these observations). The largest norm drift over one logical circuit was `3.46e-15` after 100 phase applications. Vector storage grew from 256 bytes at 4 qubits to 4 MiB at 18 qubits, a factor of 16,384 for 14 additional qubits. These results only compare implementations in this project on this machine; they are not comparisons against other simulators.

## Project layout

```text
include/       Public state, gate, circuit, and measurement interfaces
src/           Simulator implementation and CLI
tests/         Command-line validation suite
benchmarks/    Benchmark, summary, and SVG plotting scripts
results/       Raw measurements, summary, metadata, and generated figures
report/        Technical report source and PDF
```

## References

- Nielsen, M. A., & Chuang, I. L. (2010). *Quantum Computation and Quantum Information* (10th anniversary ed.). Cambridge University Press.
- Markov, I. L., & Shi, Y. (2008). Simulating quantum computation by contracting tensor networks. *SIAM Journal on Computing, 38*(3), 963-981. https://doi.org/10.1137/050644756
- IBM Quantum, *Bit-ordering in the Qiskit SDK*. https://quantum.cloud.ibm.com/docs/en/guides/bit-ordering

## License

MIT. See [LICENSE](LICENSE).
