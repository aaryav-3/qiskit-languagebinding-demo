# Student Guide: Learning Quantum Computing with Qiskit C++ Bindings

This guide helps you learn quantum computing by writing your own experiments using **Qiskit C++ bindings** with a **fake backend** for local development—no quantum hardware needed!

## 🎯 What You'll Learn

- Build quantum circuits using **C++ and Qiskit C++ bindings**
- Understand quantum gates, measurements, and entanglement
- Test locally with a **mock backend** (instant results, no hardware)
- Transition to real IBM Quantum hardware when ready

---

## 🏗️ Architecture: C++ Bindings vs C API

```
Your C++ Code
    ↓
Qiskit C++ Bindings (circuit/quantumcircuit.hpp)
    ↓
Qiskit C API (qiskit.h - Rust FFI)
    ↓
Qiskit Rust Core
    ↓
Python Qiskit (via PyO3)
```

**Key Point**: You write idiomatic C++ code using classes like `QuantumCircuit`, `QuantumRegister`, etc. The bindings handle all the low-level C API calls for you!

---

## 🚀 Quick Start

### Step 1: One-Time Setup

```bash
# Clone repository
git clone https://github.com/qiskit-community/qiskit-c-api-demo
cd qiskit-c-api-demo
git submodule update --init --recursive

# Build Qiskit C extension
cd deps/qiskit && make c && cd ../..

# Build QRMI (quantum resource manager)
cd deps/qrmi && cargo build --release && cd ../..

# Create your workspace
mkdir -p student-experiments
cd student-experiments
```

### Step 2: Enable Fake Backend Mode

```bash
mkdir build && cd build

# Configure with FAKE BACKEND (no quantum hardware needed!)
cmake .. -DCMAKE_CXX_FLAGS="-DUSE_RANDOM_SHOTS=1"
make
```

**What does `-DUSE_RANDOM_SHOTS=1` do?**
- Activates a **mock quantum backend** in [`main.cpp:250`](../src/main.cpp:250)
- Generates random measurement results via [`generate_counts_uniform()`](../src/main.cpp:49-69)
- Perfect for **learning, debugging, and rapid prototyping**
- **Zero cost, instant results, unlimited experiments!**

---

## 📚 Example Experiments

### Experiment 1: Bell State (Quantum Entanglement)

Create `student-experiments/bell_state.cpp`:

```cpp
#include "circuit/quantumcircuit.hpp"
#include <iostream>

using namespace Qiskit::circuit;

int main() {
    // Create 2-qubit circuit with classical register
    auto qr = QuantumRegister(2);
    auto cr = ClassicalRegister(2);
    auto circ = QuantumCircuit(qr, cr);
    
    // Build Bell state: (|00⟩ + |11⟩)/√2
    circ.h(0);        // Hadamard on qubit 0: creates superposition
    circ.cx(0, 1);    // CNOT: entangles qubits 0 and 1
    
    // Measure both qubits
    circ.measure(0, 0);
    circ.measure(1, 1);
    
    std::cout << "✅ Bell state circuit created!" << std::endl;
    std::cout << "   Qubits: " << circ.num_qubits() << std::endl;
    std::cout << "   Gates: H(0), CNOT(0,1)" << std::endl;
    std::cout << "\n📊 Expected on REAL hardware:" << std::endl;
    std::cout << "   ~50% |00⟩ and ~50% |11⟩ (entangled!)" << std::endl;
    std::cout << "   Never |01⟩ or |10⟩" << std::endl;
    
    return 0;
}
```

**Build and run**:
```bash
# Add to CMakeLists.txt:
add_executable(bell_state bell_state.cpp)
target_link_libraries(bell_state qiskit_cpp)

# Build
cmake .. -DCMAKE_CXX_FLAGS="-DUSE_RANDOM_SHOTS=1"
make bell_state

# Run
./bell_state
```

---

### Experiment 2: Quantum Superposition

Create `student-experiments/superposition.cpp`:

```cpp
#include "circuit/quantumcircuit.hpp"
#include <iostream>

using namespace Qiskit::circuit;

int main() {
    // Single qubit circuit
    auto qr = QuantumRegister(1);
    auto cr = ClassicalRegister(1);
    auto circ = QuantumCircuit(qr, cr);
    
    // Create superposition: |0⟩ → (|0⟩ + |1⟩)/√2
    circ.h(0);
    circ.measure(0, 0);
    
    std::cout << "✅ Superposition circuit created!" << std::endl;
    std::cout << "\n📊 Expected on REAL hardware:" << std::endl;
    std::cout << "   ~50% |0⟩ and ~50% |1⟩" << std::endl;
    std::cout << "   This is quantum randomness!" << std::endl;
    
    return 0;
}
```

---

### Experiment 3: GHZ State (3-Qubit Entanglement)

Create `student-experiments/ghz_state.cpp`:

```cpp
#include "circuit/quantumcircuit.hpp"
#include <iostream>

using namespace Qiskit::circuit;

int main() {
    // 3-qubit circuit
    auto qr = QuantumRegister(3);
    auto cr = ClassicalRegister(3);
    auto circ = QuantumCircuit(qr, cr);
    
    // Build GHZ state: (|000⟩ + |111⟩)/√2
    circ.h(0);        // Superposition on qubit 0
    circ.cx(0, 1);    // Entangle 0 and 1
    circ.cx(0, 2);    // Entangle 0 and 2
    
    // Measure all qubits
    circ.measure_all();
    
    std::cout << "✅ GHZ state circuit created!" << std::endl;
    std::cout << "\n📊 Expected on REAL hardware:" << std::endl;
    std::cout << "   ~50% |000⟩ and ~50% |111⟩" << std::endl;
    std::cout << "   All 3 qubits are entangled!" << std::endl;
    
    return 0;
}
```

---

### Experiment 4: Quantum Phase Estimation

Create `student-experiments/phase_estimation.cpp`:

```cpp
#include "circuit/quantumcircuit.hpp"
#include <iostream>
#include <cmath>

using namespace Qiskit::circuit;

int main() {
    // 2 qubits: 1 for phase estimation, 1 for eigenstate
    auto qr = QuantumRegister(2);
    auto cr = ClassicalRegister(2);
    auto circ = QuantumCircuit(qr, cr);
    
    // Prepare eigenstate |1⟩ on qubit 1
    circ.x(1);
    
    // Apply Hadamard to counting qubit
    circ.h(0);
    
    // Controlled-phase gate (estimate phase of π/4)
    circ.cp(M_PI/4, 0, 1);
    
    // Inverse QFT (simplified for 1 qubit)
    circ.h(0);
    
    // Measure
    circ.measure_all();
    
    std::cout << "✅ Phase estimation circuit created!" << std::endl;
    std::cout << "   Estimating phase: π/4" << std::endl;
    
    return 0;
}
```

---

## 🎓 Available Quantum Gates

### Single-Qubit Gates

```cpp
circ.h(qubit);              // Hadamard: superposition
circ.x(qubit);              // Pauli-X: bit flip (NOT gate)
circ.y(qubit);              // Pauli-Y
circ.z(qubit);              // Pauli-Z: phase flip
circ.s(qubit);              // S gate: √Z
circ.sdg(qubit);            // S† gate
circ.t(qubit);              // T gate: √S
circ.tdg(qubit);            // T† gate
circ.rx(theta, qubit);      // Rotation around X-axis
circ.ry(theta, qubit);      // Rotation around Y-axis
circ.rz(theta, qubit);      // Rotation around Z-axis
circ.p(theta, qubit);       // Phase gate
circ.u(theta, phi, lam, q); // Universal single-qubit gate
```

### Two-Qubit Gates

```cpp
circ.cx(control, target);           // CNOT: controlled-X
circ.cy(control, target);           // Controlled-Y
circ.cz(control, target);           // Controlled-Z
circ.ch(control, target);           // Controlled-Hadamard
circ.cp(theta, control, target);    // Controlled-phase
circ.swap(qubit1, qubit2);          // SWAP gate
circ.xx_plus_yy(theta, beta, q1, q2); // XX+YY interaction
```

### Measurement

```cpp
circ.measure(qubit, clbit);         // Measure specific qubit
circ.measure_all();                 // Measure all qubits
```

---

## 🔧 Complete Student Template

Create `student-experiments/template.cpp`:

```cpp
#include "circuit/quantumcircuit.hpp"
#include "compiler/transpiler.hpp"
#include "primitives/backend_sampler_v2.hpp"
#include "service/qiskit_runtime_service.hpp"
#include <iostream>
#include <cmath>

using namespace Qiskit::circuit;
using namespace Qiskit::primitives;
using namespace Qiskit::service;
using namespace Qiskit::compiler;

int main() {
    // 1. CREATE CIRCUIT
    const int num_qubits = 2;
    auto qr = QuantumRegister(num_qubits);
    auto cr = ClassicalRegister(num_qubits);
    auto circ = QuantumCircuit(qr, cr);
    
    // 2. ADD YOUR GATES HERE
    circ.h(0);
    circ.cx(0, 1);
    
    // 3. MEASURE
    circ.measure_all();
    
    // 4. PRINT INFO
    std::cout << "Circuit created with " << circ.num_qubits() 
              << " qubits" << std::endl;
    
    // 5. (OPTIONAL) RUN ON REAL HARDWARE
    // Uncomment when ready to use real backend:
    /*
    auto service = QiskitRuntimeService();
    auto backend = service.backend("ibm_brisbane");
    auto transpiled = transpile(circ, backend);
    auto sampler = BackendSamplerV2(backend, 1000);
    auto job = sampler.run({SamplerPub(transpiled)});
    auto result = job->result();
    auto counts = result[0].data().get_counts();
    
    for (const auto& [bitstring, count] : counts) {
        std::cout << bitstring << ": " << count << std::endl;
    }
    */
    
    return 0;
}
```

---

## 📦 Student CMakeLists.txt

Create `student-experiments/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.10)
project(student-experiments LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Point to parent demo's dependencies
set(QISKIT_CPP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../deps/qiskit-cpp")
set(QISKIT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../deps/qiskit")
set(QRMI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../deps/qrmi")

# Qiskit C++ bindings
add_library(qiskit_cpp INTERFACE)
target_include_directories(qiskit_cpp INTERFACE
    ${QISKIT_CPP_ROOT}/src
    ${QISKIT_ROOT}/dist/c/include
    ${QRMI_ROOT}
)

# Student experiments
add_executable(bell_state bell_state.cpp)
target_link_libraries(bell_state qiskit_cpp)

add_executable(superposition superposition.cpp)
target_link_libraries(superposition qiskit_cpp)

add_executable(ghz_state ghz_state.cpp)
target_link_libraries(ghz_state qiskit_cpp)

add_executable(template template.cpp)
target_link_libraries(template qiskit_cpp)

# Link Qiskit libraries
if(APPLE)
    target_link_directories(bell_state PUBLIC ${QISKIT_ROOT}/dist/c/lib)
    target_link_libraries(bell_state qiskit)
    
    target_link_directories(superposition PUBLIC ${QISKIT_ROOT}/dist/c/lib)
    target_link_libraries(superposition qiskit)
    
    target_link_directories(ghz_state PUBLIC ${QISKIT_ROOT}/dist/c/lib)
    target_link_libraries(ghz_state qiskit)
    
    target_link_directories(template PUBLIC ${QISKIT_ROOT}/dist/c/lib)
    target_link_libraries(template qiskit)
endif()
```

---

## 🎯 Development Workflow

### Makefile for Easy Building

Create `student-experiments/Makefile`:

```makefile
.PHONY: fake real clean run-bell run-super run-ghz

# Build with fake backend (development mode)
fake:
	mkdir -p build && cd build && \
	cmake .. -DCMAKE_CXX_FLAGS="-DUSE_RANDOM_SHOTS=1" && \
	make

# Build for real hardware (production mode)
real:
	mkdir -p build && cd build && \
	cmake .. && \
	make

# Clean build artifacts
clean:
	rm -rf build

# Run experiments
run-bell: fake
	./build/bell_state

run-super: fake
	./build/superposition

run-ghz: fake
	./build/ghz_state

# Help
help:
	@echo "Student Experiments Makefile"
	@echo "  make fake      - Build with fake backend (instant results)"
	@echo "  make real      - Build for real quantum hardware"
	@echo "  make run-bell  - Build and run Bell state experiment"
	@echo "  make run-super - Build and run superposition experiment"
	@echo "  make run-ghz   - Build and run GHZ state experiment"
	@echo "  make clean     - Remove build artifacts"
```

**Usage**:
```bash
make fake          # Build all experiments with fake backend
make run-bell      # Build and run Bell state
make run-ghz       # Build and run GHZ state
make clean         # Clean up
```

---

## 🔄 Fake Backend vs Real Hardware

### How the Fake Backend Works

**Location**: [`qiskit-c-api-demo/src/main.cpp:49-69`](../src/main.cpp:49-69)

```cpp
std::unordered_map<std::string, uint64_t> generate_counts_uniform(
    int num_samples, int num_bits, std::optional<unsigned int> seed
) {
    std::mt19937 rng(seed.value_or(std::random_device{}()));
    std::bernoulli_distribution dist(0.5);  // 50/50 coin flip
    
    std::unordered_map<std::string, uint64_t> counts;
    
    for (int i = 0; i < num_samples; ++i) {
        std::string bitstring;
        for (int j = 0; j < num_bits; ++j) {
            bitstring += dist(rng) ? '1' : '0';  // Random bit
        }
        counts[bitstring]++;
    }
    return counts;
}
```

**Activated at**: [`main.cpp:250`](../src/main.cpp:250)

```cpp
#if USE_RANDOM_SHOTS != 0
    counts = generate_counts_uniform(sqd_data.num_shots, 2 * norb, 1234);
#else
    // Real quantum backend execution...
#endif
```

### Comparison

| Aspect | Fake Backend | Real Hardware |
|--------|--------------|---------------|
| **Speed** | Instant | Minutes (queue time) |
| **Cost** | Free | Uses quantum credits |
| **Results** | Random uniform | True quantum behavior |
| **Entanglement** | No | Yes |
| **Noise** | None | Hardware noise |
| **Use Case** | Learning, debugging | Research, validation |

### Example Output Comparison

**Bell State on Fake Backend**:
```
{"00": 253, "01": 241, "10": 256, "11": 250}  // Random!
```

**Bell State on Real Hardware**:
```
{"00": 512, "11": 488, "01": 0, "10": 0}  // Entangled!
```

---

## 🚀 Transitioning to Real Hardware

### Step 1: Get IBM Quantum Credentials

1. Sign up at [quantum.ibm.com](https://quantum.ibm.com)
2. Get your API token and CRN

### Step 2: Set Environment Variables

```bash
export QISKIT_IBM_TOKEN="your_api_key_here"
export QISKIT_IBM_INSTANCE="your_crn_here"
```

### Step 3: Rebuild Without Fake Backend

```bash
cd student-experiments/build
cmake ..  # No -DUSE_RANDOM_SHOTS flag!
make
```

### Step 4: Run on Real Hardware

```bash
./bell_state --backend_name ibm_brisbane --num_shots 1000
```

---

## 🐛 Troubleshooting

### Build Fails

```bash
# Check dependencies
which cmake
which mpirun
pkg-config --modversion eigen3

# Rebuild from scratch
cd student-experiments
rm -rf build
make fake
```

### Fake Backend Not Working

```bash
# Verify flag is set
grep "USE_RANDOM_SHOTS" build/CMakeCache.txt

# Should see: CMAKE_CXX_FLAGS:STRING=-DUSE_RANDOM_SHOTS=1
```

### Real Backend Fails

```bash
# Check credentials
echo $QISKIT_IBM_TOKEN
echo $QISKIT_IBM_INSTANCE

# Test connection
# (Add test code to verify backend access)
```

---

## 📖 Additional Resources

### Documentation
- [Qiskit C++ Bindings](https://github.com/Qiskit/qiskit-cpp)
- [Qiskit Textbook](https://qiskit.org/textbook/)
- [IBM Quantum Learning](https://learning.quantum.ibm.com/)

### Community
- [Qiskit Slack](https://qiskit.slack.com) - #c-api channel
- [GitHub Discussions](https://github.com/Qiskit/qiskit/discussions)

### Example Algorithms
- **Bell State**: Quantum entanglement demo
- **GHZ State**: Multi-qubit entanglement
- **QFT**: Quantum Fourier Transform
- **Grover**: Quantum search algorithm
- **VQE**: Variational Quantum Eigensolver

---

## 🎉 Next Steps

1. **Start with fake backend**: Build confidence without costs
2. **Experiment freely**: Try different gates and circuits
3. **Understand the math**: Learn why circuits behave as they do
4. **Graduate to real hardware**: Experience true quantum effects
5. **Share your work**: Contribute examples to the community

Happy quantum coding! 🚀⚛️

---

## 📝 Assignment Ideas

### Assignment 1: Quantum Coin Flip
Build a fair coin flip using Hadamard gate. Verify 50/50 distribution.

### Assignment 2: Three-Qubit Entanglement
Create GHZ state and explain why only |000⟩ and |111⟩ appear.

### Assignment 3: Quantum Teleportation
Implement teleportation protocol with Bell measurement.

### Assignment 4: Deutsch-Jozsa Algorithm
Determine if function is constant or balanced in one query.

### Assignment 5: Compare Fake vs Real
Run same circuit on both backends and analyze differences.