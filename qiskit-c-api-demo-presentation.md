# Qiskit C API Demo: Language Bindings Presentation
**Duration: 15-20 minutes**

> **IMPORTANT NOTE**: This demo uses the **[qiskit-cpp](./deps/qiskit-cpp)** C++ wrapper library, which provides an object-oriented interface on top of the lower-level C API. The C++ wrapper is more ergonomic for application development, while the raw C API (located in [`qiskit/crates/cext`](qiskit/crates/cext)) is available for maximum control and direct use from C or other languages.

---

## 1. Introduction

### What is the Qiskit C API?

The Qiskit C API enables **compiled hybrid quantum-classical workflows** by exposing Qiskit's core functionality through a C interface. This allows:

- **HPC Integration**: Run quantum algorithms on supercomputers with MPI/OpenMP
- **Language Interoperability**: Call Qiskit from C, C++, Julia, Fortran, and other languages
- **Performance**: Eliminate Python overhead in compute-intensive workflows
- **Scalability**: Deploy across thousands of compute nodes

### Demo Overview: Sample-based Quantum Diagonalization (SQD)

This demo computes the ground state energy of the Fe₄S₄ cluster using:
1. **Circuit construction** (C++ → Qiskit C API)
2. **Transpilation** (C++ → Qiskit Rust core)
3. **Quantum execution** (via QRMI middleware)
4. **Classical post-processing** (MPI-parallel SQD algorithm)

---

## 2. Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                    C++ Application                       │
│  (main.cpp - MPI/OpenMP parallel, HPC-ready)            │
│  File: qiskit-c-api-demo/src/main.cpp                   │
└────────────────┬────────────────────────────────────────┘
                 │
                 ├─► qiskit-cpp (C++ Wrapper Library)
                 │   └─► Qiskit C API (qiskit/crates/cext)
                 │       └─► Qiskit Rust Core
                 │
                 ├─► QRMI (Quantum Resource Management)
                 │   └─► IBM Quantum Backend
                 │
                 └─► SQD Addon + SBD Eigensolver
                     └─► Classical post-processing
```

### Key Components

| Component | Language | Purpose |
|-----------|----------|---------|
| **main.cpp** | C++17 | Orchestrates workflow, MPI coordination |
| **Qiskit C API** | C/Rust | Circuit building, transpilation |
| **QRMI** | Rust | Quantum backend interface |
| **SQD Addon** | C++ | Configuration recovery algorithm |
| **SBD** | C++ | Davidson eigensolver |

---

## 3. Language Binding Architecture

### How Language Bindings Work

The Qiskit C API enables multiple language ecosystems through a layered architecture:

```
┌─────────────────────────────────────────────────────────┐
│         Your Application (C++, Julia, Fortran, etc.)    │
└────────────────┬────────────────────────────────────────┘
                 │
                 ├─► Language-Specific Wrapper (Recommended)
                 │   └─► C++: qiskit-cpp (qiskit-c-api-demo/deps/qiskit-cpp)
                 │   └─► Julia: Qiskit.jl (Qiskit.jl/src/Qiskit.jl)
                 │
                 ├─► C API Layer (qiskit/crates/cext)
                 │   └─► Rust Core (qiskit/crates/*)
                 │
                 └─► Direct C API Usage
                     └─► Functions like qk_circuit_new(), qk_circuit_gate()
```

### Two Approaches to Using the C API

1. **Using Pre-built Wrappers** (Recommended for most users)
   - Julia: [`Qiskit.jl`](https://github.com/Qiskit/Qiskit.jl) package
   - Python: Direct C extension imports
   - Benefit: Idiomatic language syntax, automatic memory management

2. **Writing Custom C++ Code** (For advanced integration)
   - Direct FFI calls to C API functions
   - Full control over memory and performance
   - Ideal for HPC, embedded systems, or custom workflows

---

## 4. Code Walkthrough: Circuit Construction (4 min)

### Step 1: Include Qiskit C++ Headers

**File**: [`./src/main.cpp:33-42`](./src/main.cpp:33)

```cpp
#include "circuit/quantumcircuit.hpp"
#include "compiler/transpiler.hpp"
#include "primitives/backend_sampler_v2.hpp"
#include "service/qiskit_runtime_service.hpp"

using namespace Qiskit::circuit;
using namespace Qiskit::providers;
using namespace Qiskit::primitives;
using namespace Qiskit::service;
using namespace Qiskit::compiler;
```

> **Note**: These are C++ wrapper headers from qiskit-cpp, not the raw C API headers.

### Step 2: Create Quantum Circuit

**File**: [`./src/main.cpp:283-286`](./src/main.cpp:283)

```cpp
// Create quantum and classical registers
auto qr = QuantumRegister(2 * norb);   // 2*norb qubits (spin-up + spin-down)
auto cr = ClassicalRegister(2 * norb); // Classical bits for measurement
auto circ = QuantumCircuit(qr, cr);    // Combine into circuit
```

**Raw C API equivalent** (from [`qiskit/crates/cext/src/circuit.rs:50`](qiskit/crates/cext/src/circuit.rs:50)):
```c
QkCircuit *qc = qk_circuit_new(2 * norb, 2 * norb);
```

### Step 3: Build LUCJ Ansatz Circuit

The demo uses the **Local Unitary Cluster Jastrow (LUCJ)** ansatz from quantum chemistry:

**File**: [`./src/main.cpp:280-305`](./src/main.cpp:280)

```cpp
// Generate gate instructions from ffsim library
auto instructions = hf_and_ucj_op_spin_balanced_jw(qubits, nelec, ucj_op);

// Add gates to Qiskit circuit (C++ wrapper API)
for (const auto &instr : instructions) {
    if (instr.gate == "x") {
        circ.x(instr.qubits[0]);
    } else if (instr.gate == "rz") {
        circ.rz(instr.params[0], instr.qubits[0]);
    } else if (instr.gate == "cp") {
        circ.cp(instr.params[0], instr.qubits[0], instr.qubits[1]);
    } else if (instr.gate == "xx_plus_yy") {
        circ.xx_plus_yy(
            instr.params[0], instr.params[1],
            instr.qubits[0], instr.qubits[1]
        );
    }
}
```

**Raw C API equivalent** (from [`qiskit/crates/cext/src/circuit.rs:412`](qiskit/crates/cext/src/circuit.rs:412)):
```c
uint32_t qubit[1] = {0};
qk_circuit_gate(qc, QkGate_X, qubit, NULL);

double rz_param[1] = {theta};
qk_circuit_gate(qc, QkGate_RZ, qubit, rz_param);
```

### Step 4: Add Measurements

```cpp
// Measure all qubits into classical register
for (size_t i = 0; i < circ.num_qubits(); ++i) {
    circ.measure(i, i);
}
```

**Key Insight**: The C++ API mirrors Python's syntax while providing compile-time type safety!

---

## 5. Code Walkthrough: Transpilation & Execution

### Step 5: Get Backend from Runtime Service

**Note**: This uses qiskit-cpp wrapper and QRMI middleware, not direct C API.

```cpp
// Connect to IBM Quantum (requires environment variables)
// QISKIT_IBM_TOKEN = "your_api_key"
// QISKIT_IBM_INSTANCE = "your_CRN"

std::string backend_name = "ibm_torino";  // or any available backend
auto service = QiskitRuntimeService();
auto backend = service.backend(backend_name);
```

> The C API does not directly provide runtime service access. This demo uses QRMI middleware for backend communication.

### Step 6: Transpile Circuit

**File**: C++ wrapper function (wraps C API internally)

```cpp
// Transpile for target hardware topology and gate set
auto transpiled = transpile(circ, backend);
```

**Raw C API equivalent** (from [`qiskit/crates/cext/src/transpiler/transpile_function.rs:741`](qiskit/crates/cext/src/transpiler/transpile_function.rs:741)):
```c
QkTranspileResult result;
QkTranspileOptions *options = qk_transpiler_default_options();
qk_transpile(qc, target, options, &result);
// result.circuit contains transpiled circuit
// result.layout contains qubit mapping
```

**What happens here?**
- Circuit optimization (gate fusion, cancellation)
- Layout mapping (logical → physical qubits)
- Gate decomposition (to native gate set)
- Routing (SWAP insertion for connectivity)

### Step 7: Execute with Sampler

```cpp
// Configure sampler with shot count
uint64_t num_shots = 10000;
auto sampler = BackendSamplerV2(backend, num_shots);

// Submit job and wait for results
auto job = sampler.run({SamplerPub(transpiled)});
auto result = job->result();
auto pub_result = result[0];

// Extract measurement counts
std::unordered_map<std::string, uint64_t> counts = 
    pub_result.data().get_counts();
```

**Output format**:
```cpp
counts = {
    "0000000000": 1523,
    "0000000001": 892,
    "0000000010": 1104,
    // ... (bitstring → count)
}
```

---

## 6. Code Walkthrough: Classical Post-Processing (3 min)

### Step 8: Configuration Recovery

**File**: [`./src/main.cpp:138-161`](./src/main.cpp:138) (counts_to_arrays helper)

The SQD algorithm recovers physically valid configurations from noisy measurements:

```cpp
// Convert counts to probability distribution
auto [bitstring_matrix, probs] = counts_to_arrays(counts);

// Recover configurations using prior occupancies
// From qiskit-addon-sqd-hpc library
auto recovered = Qiskit::addon::sqd::recover_configurations(
    bitstring_matrix,
    probs,
    latest_occupancies,
    {num_elec_a, num_elec_b},  // Electron count constraint
    rng
);
```

### Step 9: Subsampling & Diagonalization

```cpp
// Subsample to manageable batch size
std::vector<boost::dynamic_bitset<>> batch;
Qiskit::addon::sqd::subsample(
    batch, 
    recovered_bitstrings, 
    recovered_probs, 
    samples_per_batch, 
    rng
);

// Run Davidson eigensolver (MPI-parallel)
auto [energy, occupancies] = sbd_main(MPI_COMM_WORLD, diag_data);

std::cout << "Ground state energy: " << energy << " Hartree" << std::endl;
```

### Step 10: Iterative Refinement

```cpp
// Update occupancies for next recovery iteration
for (size_t j = 0; j < latest_occupancies[0].size(); ++j) {
    latest_occupancies[0][j] = occupancies[2*j];      // alpha
    latest_occupancies[1][j] = occupancies[2*j + 1];  // beta
}
```

---

## 7. Running the Demo (2 min)

### Test Mode (No Quantum Hardware)

```bash
cd./build
cmake .. -DCMAKE_CXX_FLAGS="-DUSE_RANDOM_SHOTS=1"
make

./c-api-demo \
  --fcidump ../data/fcidump_Fe4S4_MO.txt \
  -v \
  --tolerance 1.0e-3 \
  --max_time 60 \
  --recovery 1 \
  --number_of_samples 10 \
  --num_shots 100
```

### Production Mode (IBM Quantum)

```bash
export QISKIT_IBM_TOKEN="your_api_key"
export QISKIT_IBM_INSTANCE="your_CRN"

mpirun -np 96 ./c-api-demo \
  --fcidump ../data/fcidump_Fe4S4_MO.txt \
  -v \
  --tolerance 1.0e-3 \
  --max_time 600 \
  --recovery 3 \
  --number_of_samples 2000 \
  --num_shots 10000 \
  --backend_name ibm_torino
```

### Expected Output

```
2026-02-13 11:03:42: initial parameters are loaded. param_length=2632
2026-02-13 11:03:42: start recovery: iteration=0
2026-02-13 11:03:42: Number of recovered bitstrings: 100
2026-02-13 11:03:42: number of items in a batch: 10
 Davidson iteration 0.0 (tol=0.0034763): -326.524
 Davidson iteration 0.1 (tol=2.55306e-05): -326.524 -326.052
 Elapsed time for diagonalization 0.006687 (sec) 
 Energy = -326.5243602758733
2026-02-13 11:03:43: energy: -326.524360
```

---

## 8. Key Takeaways (2 min)

### Why Use the C API?

**Performance**: Compiled code, no Python interpreter overhead  
**Scalability**: MPI/OpenMP parallelism across HPC clusters  
**Integration**: Call from any language with C FFI  
**Deployment**: Single executable, no Python environment needed  

### API Design Principles

1. **Familiar Syntax**: Mirrors Python API where possible
2. **Type Safety**: Compile-time checks prevent runtime errors
3. **Resource Management**: RAII patterns for automatic cleanup
4. **Error Handling**: Exceptions with clear error messages

### Current Capabilities (Qiskit 2.2)

| Feature | Status |
|---------|--------|
| Circuit construction | Full support |
| Transpilation | Full support |
| Backend execution | Via QRMI |
| Observables | SparsePauliOp |
| Primitives | Sampler V2 |

### Future Roadmap

- **Estimator V2** primitive support
- **Direct backend access** (no QRMI dependency)
- **More language bindings** (Julia, Fortran, Go)
- **Extended gate library** (custom gates, pulse-level)

---

## 9. Q&A and Resources

### Documentation

- **Qiskit C API Docs**: https://quantum.cloud.ibm.com/docs/en/api/qiskit-c
- **Demo Repository**: https://github.com/qiskit-community/qiskit-c-api-demo
- **SQD Paper**: https://www.science.org/doi/10.1126/sciadv.adu9991

### Try It Yourself

```bash
git clone https://github.com/qiskit-community/qiskit-c-api-demo
cd qiskit-c-api-demo
git submodule update --init --recursive

# Build dependencies
cd deps/qiskit && make c && cd ../..
cd deps/qrmi && cargo build --release && cd ../..

# Build demo
mkdir build && cd build
cmake .. -DCMAKE_CXX_FLAGS="-DUSE_RANDOM_SHOTS=1"
make

# Run!
./c-api-demo --fcidump ../data/fcidump_Fe4S4_MO.txt -v
```

### Contact

- **GitHub Issues**: Report bugs or request features
- **Qiskit Slack**: #c-api channel
- **Email**: qiskit@qiskit.org

---

## Appendix: Complete Code Flow

```cpp
// 1. Initialize MPI
MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

// 2. Load parameters
load_initial_parameters(input_file, norb, nelec, interactions, params);

// 3. Build circuit
auto qr = QuantumRegister(2 * norb);
auto cr = ClassicalRegister(2 * norb);
auto circ = QuantumCircuit(qr, cr);
// ... add gates ...
circ.measure_all();

// 4. Transpile
auto service = QiskitRuntimeService();
auto backend = service.backend("ibm_torino");
auto transpiled = transpile(circ, backend);

// 5. Execute
auto sampler = BackendSamplerV2(backend, num_shots);
auto job = sampler.run({SamplerPub(transpiled)});
auto counts = job->result()[0].data().get_counts();

// 6. Post-process
auto recovered = recover_configurations(counts, occupancies, nelec);
auto [energy, new_occupancies] = sbd_main(MPI_COMM_WORLD, diag_data);

// 7. Finalize
MPI_Finalize();
```

**Total Lines of Code**: ~437 lines (including comments)  
**Compilation Time**: ~30 seconds  
**Runtime**: 1-10 minutes (depending on backend queue)

---

---

## Appendix B: C++ Wrapper API Reference

**IMPORTANT**: The API below is the **C++ wrapper** used in this demo, which provides a high-level interface over the underlying C API from [`qiskit/crates/cext`](qiskit/crates/cext).

### Core Circuit Classes

**Source**: [`deps/qiskit-cpp/src/circuit/`](deps/qiskit-cpp/src/circuit/)

```cpp
// Register classes
class QuantumRegister {
    QuantumRegister(uint_t size);
    QuantumRegister(uint_t size, std::string name);
};

class ClassicalRegister {
    ClassicalRegister(uint_t size);
    ClassicalRegister(uint_t size, std::string name);
};

// Circuit class
class QuantumCircuit {
    // Constructors
    QuantumCircuit(uint_t num_qubits, uint_t num_clbits, double global_phase = 0.0);
    QuantumCircuit(QuantumRegister& qreg, ClassicalRegister& creg, double global_phase = 0.0);
    
    // Properties
    uint_t num_qubits() const;
    uint_t num_clbits() const;
    
    // Single-qubit gates
    void h(uint_t qubit);
    void x(uint_t qubit);
    void y(uint_t qubit);
    void z(uint_t qubit);
    void s(uint_t qubit);
    void t(uint_t qubit);
    void rx(double theta, uint_t qubit);
    void ry(double theta, uint_t qubit);
    void rz(double theta, uint_t qubit);
    
    // Two-qubit gates
    void cx(uint_t control, uint_t target);
    void cy(uint_t control, uint_t target);
    void cz(uint_t control, uint_t target);
    void swap(uint_t qubit1, uint_t qubit2);
    
    // Measurement
    void measure(uint_t qubit, uint_t clbit);
    void measure(QuantumRegister& qreg, ClassicalRegister& creg);
    
    // Other operations
    void barrier(uint_t qubit);
    void reset(uint_t qubit);
};
```

### Transpilation

**Source**: [`deps/qiskit-cpp/src/compiler/transpiler.hpp`](deps/qiskit-cpp/src/compiler/transpiler.hpp)

```cpp
namespace Qiskit::compiler {
    // Transpile a circuit for a backend
    QuantumCircuit transpile(
        QuantumCircuit& circ,
        providers::BackendV2& backend,
        int optimization_level = 2,
        double approximation_degree = 1.0,
        int seed_transpiler = -1
    );
}
```

### Runtime Service and Backend

**Source**: [`deps/qiskit-cpp/src/service/qiskit_runtime_service.hpp`](deps/qiskit-cpp/src/service/qiskit_runtime_service.hpp)

```cpp
namespace Qiskit::service {
    class QiskitRuntimeService {
        QiskitRuntimeService();  // Loads from ~/.qiskit/qiskit-ibm.json
        
        std::vector<std::string> backends();
        providers::QkrtBackend backend(std::string name);
        providers::QkrtBackend least_busy();
    };
}

namespace Qiskit::providers {
    class QkrtBackend : public BackendV2 {
        std::shared_ptr<transpiler::Target> target();
        std::shared_ptr<Job> run(std::vector<primitives::SamplerPub>& pubs, uint_t shots);
    };
}
```

### Primitives

**Source**: [`deps/qiskit-cpp/src/primitives/`](deps/qiskit-cpp/src/primitives/)

```cpp
namespace Qiskit::primitives {
    // Sampler publication
    class SamplerPub {
        SamplerPub(QuantumCircuit& circ, uint_t shots = 0);
        const QuantumCircuit& circuit() const;
        uint_t shots();
    };
    
    // Backend sampler
    class BackendSamplerV2 {
        BackendSamplerV2(providers::BackendV2& backend, uint_t shots = 1024);
        
        std::shared_ptr<BasePrimitiveJob> run(std::vector<SamplerPub> pubs);
    };
}
```

**Note**: The C++ wrapper automatically handles memory management using RAII and smart pointers, eliminating the need for manual `free()` calls required by the raw C API.

### Example: Creating a Bell State Circuit

```cpp
#include "circuit/quantumcircuit.hpp"
#include "service/qiskit_runtime_service.hpp"
#include "compiler/transpiler.hpp"
#include "primitives/backend_sampler_v2.hpp"

using namespace Qiskit;

int main() {
    // 1. Create a 2-qubit, 2-classical bit circuit
    auto qc = circuit::QuantumCircuit(2, 2);
    
    // 2. Build Bell state circuit
    qc.h(0);           // Hadamard on qubit 0
    qc.cx(0, 1);       // CNOT with control=0, target=1
    qc.measure(0, 0);  // Measure qubit 0 -> clbit 0
    qc.measure(1, 1);  // Measure qubit 1 -> clbit 1
    
    // 3. Initialize runtime service and get backend
    auto service = service::QiskitRuntimeService();
    auto backend = service.backend("ibm_kyiv");
    
    // 4. Transpile for the backend
    auto transpiled = compiler::transpile(qc, backend);
    
    // 5. Execute on backend
    auto sampler = primitives::BackendSamplerV2(backend, 1024);
    auto pub = primitives::SamplerPub(transpiled);
    auto job = sampler.run({pub});
    
    // 6. Get results (job management handled by wrapper)
    // Results would be retrieved through job->result()
    
    return 0;
}
```

**Key Differences from Raw C API:**
- **Automatic memory management**: No manual `free()` calls needed
- **Method chaining**: Natural C++ syntax like `qc.h(0).cx(0,1)`
- **Type safety**: Compile-time checking of parameters
- **RAII**: Resources automatically cleaned up when objects go out of scope

### Underlying C API (for reference)

The C++ wrapper internally uses the C API from [`qiskit/crates/cext`](qiskit/crates/cext):

```c
// Raw C API (wrapped by C++ classes above)
QkCircuit* qk_circuit_new(uint32_t num_qubits, uint32_t num_clbits);
void qk_circuit_free(QkCircuit* circuit);
QkExitCode qk_circuit_gate(QkCircuit* circuit, QkGate gate,
                           const uint32_t* qubits, const double* params);
QkExitCode qk_circuit_measure(QkCircuit* circuit, uint32_t qubit, uint32_t clbit);
```

The C++ wrapper provides RAII-based memory management, eliminating manual memory management.

---

## Appendix C: macOS Build Fix

### Problem Encountered

When running the demo on macOS, we encountered dynamic library linking errors:

```
dyld: Library not loaded: @rpath/libc++.1.dylib
dyld: Library not loaded: @rpath/libpython3.12.dylib
```

### Root Cause

The Rust-built `libqiskit_cext.dylib` used `@rpath` references that weren't resolving correctly on macOS.

### Solution Applied

Fixed library paths using `install_name_tool`:

```bash
# Fix libc++ reference
install_name_tool -change @rpath/libc++.1.dylib \
  /usr/lib/libc++.1.dylib \
  deps/qiskit/target/release/deps/libqiskit_cext.dylib

# Fix Python reference
install_name_tool -change @rpath/libpython3.12.dylib \
  /opt/homebrew/opt/python@3.12/Frameworks/Python.framework/Versions/3.12/lib/libpython3.12.dylib \
  deps/qiskit/target/release/deps/libqiskit_cext.dylib
```

**Result**: Demo now runs successfully on macOS!
