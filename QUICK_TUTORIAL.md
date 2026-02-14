# Quick Tutorial: Create & Execute Any Qiskit Circuit

## 1. Create Your Circuit File

**Example: `my_circuit.cpp`**

```cpp
#include <iostream>
#include <random>
#include <unordered_map>
#include "circuit/quantumcircuit.hpp"
#include "primitives/backend_sampler_v2.hpp"
#include "service/qiskit_runtime_service.hpp"
#include "compiler/transpiler.hpp"

using namespace Qiskit::circuit;
using namespace Qiskit::primitives;
using namespace Qiskit::service;
using namespace Qiskit::compiler;

// Uniform random sampler for testing
std::unordered_map<std::string, uint64_t> random_sampler(int shots, int qubits) {
    std::mt19937 rng(42);
    std::bernoulli_distribution dist(0.5);
    std::unordered_map<std::string, uint64_t> counts;
    for (int i = 0; i < shots; ++i) {
        std::string bits;
        for (int j = 0; j < qubits; ++j) bits += dist(rng) ? '1' : '0';
        counts[bits]++;
    }
    return counts;
}

int main(int argc, char* argv[]) {
    // Create your circuit
    auto qr = QuantumRegister(2);
    auto cr = ClassicalRegister(2);
    auto circuit = QuantumCircuit(qr, cr);
    
    circuit.h(0);      // Your gates here
    circuit.cx(0, 1);
    
    circuit.measure(0, 0);
    circuit.measure(1, 1);

    // Option 1: Random sampler (testing)
    if (argc == 1) {
        auto counts = random_sampler(1000, 2);
        for (auto& [bits, count] : counts)
            std::cout << "|" << bits << ">: " << count << "\n";
    }
    
    // Option 2: Real backend
    else {
        auto service = QiskitRuntimeService();
        auto backend = service.backend(argv[1]);
        auto transpiled = transpile(circuit, backend);
        auto sampler = BackendSamplerV2(backend, 1000);
        auto job = sampler.run({SamplerPub(transpiled)});
        auto counts = job->result()[0].data().get_counts();
        for (auto& [bits, count] : counts)
            std::cout << "|" << bits << ">: " << count << "\n";
    }
    return 0;
}
```

## 2. Add to CMakeLists.txt

Add these lines after line 97:

```cmake
# Your circuit executable
add_executable(my_circuit my_circuit.cpp)
```

Then add linking (after line 156):

```cmake
# Link my_circuit
if(APPLE)
    target_link_directories(my_circuit PUBLIC
        ${QISKIT_ROOT}/dist/c/lib
        ${QRMI_ROOT}/target/release)
    target_link_libraries(my_circuit
        qiskit qrmi OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json ${ACCELERATE_LIBRARY})
    set_target_properties(my_circuit PROPERTIES
        BUILD_RPATH "${QISKIT_ROOT}/dist/c/lib;${QRMI_ROOT}/target/release"
        INSTALL_RPATH "${QISKIT_ROOT}/dist/c/lib;${QRMI_ROOT}/target/release")
else()
    target_link_libraries(my_circuit PUBLIC
        "-L${QISKIT_ROOT}/dist/c/lib -L${QRMI_ROOT}/target/release"
        qiskit qrmi OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json)
endif()

target_include_directories(my_circuit PRIVATE
    ${QISKIT_ROOT}/dist/c/include
    ${QRMI_ROOT}
    ${QISKIT_CPP_ROOT}/src)
```

## 3. Build & Run

```bash
# Build
cmake -B build -S .
cmake --build build --target my_circuit

# Run with random sampler
./build/my_circuit

# Run with real backend
export QISKIT_IBM_TOKEN="your_token"
export QISKIT_IBM_INSTANCE="your_instance"
./build/my_circuit ibm_brisbane
```

## Template Pattern

**Every circuit file needs:**
1. Random sampler function (copy from above)
2. Circuit creation with `QuantumCircuit`
3. Two execution modes (random vs real)
4. CMakeLists.txt entry

**Copy-paste the code above and modify the gates!**