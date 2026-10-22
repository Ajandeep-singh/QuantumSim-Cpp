# Benchmark result provenance

## Environment

- Run date: 2026-10-12 (client local date).
- Platform: Windows, launched from PowerShell.
- Compiler: MinGW GCC 6.3.0.
- Compiler flags: `-std=c++17 -O2 -Wall -Wextra -Wpedantic`.
- Processor environment identifier: `AMD64 Family 25 Model 68, Stepping 1, AuthenticAMD`; 12 logical processors were exposed to the process. The exact CPU product name and OS build were not available to the sandbox, so they are not inferred here.
- Threading: single-threaded benchmark executable.

## Procedure

The program creates the same deterministic all-qubit `|+>` product state for every run, applies one fixed gate family to the highest-index qubit, and times only the repeated gate applications. Tested qubit counts are 4, 6, 8, 10, 12, 14, 16, and 18; depths are 1, 10, and 100 repeated applications; strategies are generic 2x2 matrix application and a specialized implementation for X and P(0.371); there are three repetitions per condition.

`elapsed_ns_per_circuit` is the timed batch divided by the number of batch copies. Small cases are batched to process at least about one million amplitude positions per timed sample. Windows uses QueryPerformanceCounter; other platforms use `std::chrono::steady_clock`. The CSV records each individual repetition. The grouped CSV records medians and error maxima.

`state_bytes` is the vector capacity multiplied by `sizeof(std::complex<double>)`: it measures the simulator's amplitude buffer, not whole-process RSS. `norm_error` is `abs(sum(|a_i|^2)-1)` after one logical circuit of the stated depth. `reference_l2_error` compares the specialized result to the generic result after that same single-circuit sequence. Timer batches repeat gate sequences to reduce timer noise, but do not accumulate into the recorded error values.

## Files

- `benchmark_raw.csv`: 288 raw repetition rows.
- `benchmark_summary.csv`: 96 operation/strategy/qubit/depth summary rows.
- `figures/runtime_by_qubits.svg`: depth-100 runtime medians.
- `figures/runtime_by_depth.svg`: runtime medians at 14 qubits.
- `figures/memory_by_qubits.svg`: measured amplitude-vector capacity.
- `figures/normalization_error.svg`: max norm drift by depth.

These measurements are a small single-machine experiment. They do not establish performance relative to other simulators or different compiler and hardware configurations.
