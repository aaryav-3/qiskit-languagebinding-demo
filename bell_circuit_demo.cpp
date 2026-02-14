/*
 * Bell Circuit Demo with Uniform Random Sampler
 * 
 * This demonstrates:
 * 1. Creating a Bell circuit (|00⟩ + |11⟩)/√2 using Qiskit C++ API
 * 2. Executing with uniform random sampler (for testing/debugging)
 * 3. Executing with real backend sampler (when available)
 */

#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <optional>

#include "circuit/quantumcircuit.hpp"
#include "primitives/backend_sampler_v2.hpp"
#include "service/qiskit_runtime_service.hpp"
#include "compiler/transpiler.hpp"

using namespace Qiskit::circuit;
using namespace Qiskit::providers;
using namespace Qiskit::primitives;
using namespace Qiskit::service;
using namespace Qiskit::compiler;

// ============================================================================
// Uniform Random Sampler (for testing without real backend)
// ============================================================================

/**
 * Generate random bitstring counts with uniform distribution
 * Useful for testing circuit logic without backend access
 * 
 * @param num_samples Number of shots to simulate
 * @param num_bits Number of qubits (bitstring length)
 * @param seed Random seed for reproducibility
 * @return Map of bitstring -> count
 */
std::unordered_map<std::string, uint64_t> generate_counts_uniform(
    int num_samples,
    int num_bits,
    std::optional<unsigned int> seed = std::nullopt
)
{
    std::mt19937 rng(seed.value_or(std::random_device{}()));
    std::bernoulli_distribution dist(0.5);

    std::unordered_map<std::string, uint64_t> counts;

    for (int i = 0; i < num_samples; ++i) {
        std::string bitstring;
        bitstring.reserve(num_bits);
        for (int j = 0; j < num_bits; ++j) {
            bitstring += dist(rng) ? '1' : '0';
        }
        counts[bitstring]++;
    }
    return counts;
}

// ============================================================================
// Bell Circuit Creation
// ============================================================================

/**
 * Create a Bell state circuit: H(q0), CNOT(q0, q1)
 * This creates the entangled state (|00⟩ + |11⟩)/√2
 * 
 * @return QuantumCircuit representing Bell state preparation
 */
QuantumCircuit create_bell_circuit()
{
    // Create 2-qubit circuit with 2 classical bits for measurement
    auto qr = QuantumRegister(2);
    auto cr = ClassicalRegister(2);
    auto circuit = QuantumCircuit(qr, cr);

    // Apply Hadamard gate to qubit 0
    circuit.h(0);

    // Apply CNOT gate with control=0, target=1
    circuit.cx(0, 1);

    // Measure both qubits
    circuit.measure(0, 0);
    circuit.measure(1, 1);

    return circuit;
}

// ============================================================================
// Display Results
// ============================================================================

/**
 * Print measurement counts in a readable format
 */
void print_counts(const std::unordered_map<std::string, uint64_t>& counts, 
                  const std::string& title)
{
    std::cout << "\n" << title << "\n";
    std::cout << std::string(50, '=') << "\n";
    
    uint64_t total = 0;
    for (const auto& [bitstring, count] : counts) {
        total += count;
    }

    for (const auto& [bitstring, count] : counts) {
        double probability = static_cast<double>(count) / total;
        std::cout << "  |" << bitstring << "⟩: " << count 
                  << " (" << (probability * 100.0) << "%)\n";
    }
    std::cout << "Total shots: " << total << "\n";
}

// ============================================================================
// Main Execution
// ============================================================================

int main(int argc, char* argv[])
{
    try {
        std::cout << "Bell Circuit Demo\n";
        std::cout << std::string(50, '=') << "\n";

        // Create Bell circuit
        auto bell_circuit = create_bell_circuit();
        std::cout << "\nBell Circuit Created:\n";
        std::cout << "  Qubits: " << bell_circuit.num_qubits() << "\n";
        std::cout << "  Classical bits: " << bell_circuit.num_clbits() << "\n";
        std::cout << "  Gates: H(q0), CNOT(q0, q1), Measure(q0->c0), Measure(q1->c1)\n";

        // ====================================================================
        // Mode 1: Uniform Random Sampler (for testing)
        // ====================================================================
        std::cout << "\n\n[Mode 1] Uniform Random Sampler Execution\n";
        std::cout << std::string(50, '-') << "\n";
        
        const int num_shots = 1000;
        const int num_qubits = 2;
        
        auto uniform_counts = generate_counts_uniform(num_shots, num_qubits, 42);
        print_counts(uniform_counts, "Uniform Random Results");
        
        std::cout << "\nNote: Uniform sampler generates random bitstrings.\n";
        std::cout << "Expected for Bell state: ~50% |00⟩ and ~50% |11⟩\n";

        // ====================================================================
        // Mode 2: Real Backend Execution (requires credentials)
        // ====================================================================
        std::cout << "\n\n[Mode 2] Real Backend Execution\n";
        std::cout << std::string(50, '-') << "\n";

        // Check if we should use real backend
        bool use_real_backend = false;
        std::string backend_name = "ibm_brisbane";
        
        // Check for environment variables or command line args
        if (argc > 1) {
            backend_name = argv[1];
            use_real_backend = true;
        }
        
        if (use_real_backend) {
            std::cout << "Attempting to use backend: " << backend_name << "\n";
            
            // Get backend from Quantum Runtime Service
            // Requires environment variables:
            //   QISKIT_IBM_TOKEN = "your API key"
            //   QISKIT_IBM_INSTANCE = "your CRN"
            auto service = QiskitRuntimeService();
            auto backend = service.backend(backend_name);

            // Transpile circuit for target backend
            auto transpiled = transpile(bell_circuit, backend);
            std::cout << "Circuit transpiled for " << backend_name << "\n";

            // Create sampler and execute
            auto sampler = BackendSamplerV2(backend, num_shots);
            auto job = sampler.run({SamplerPub(transpiled)});
            
            if (job == nullptr) {
                std::cerr << "Error: Job submission failed\n";
                return -1;
            }

            std::cout << "Job submitted, waiting for results...\n";
            auto result = job->result();
            auto pub_result = result[0];

            // Get and display counts
            auto real_counts = pub_result.data().get_counts();
            print_counts(real_counts, "Real Backend Results");
            
            std::cout << "\nFor Bell state, expect ~50% |00⟩ and ~50% |11⟩\n";
        } else {
            std::cout << "Skipping real backend execution.\n";
            std::cout << "To use real backend, run with: ./bell_circuit_demo <backend_name>\n";
            std::cout << "Example: ./bell_circuit_demo ibm_brisbane\n";
            std::cout << "\nMake sure to set environment variables:\n";
            std::cout << "  export QISKIT_IBM_TOKEN=\"your_token\"\n";
            std::cout << "  export QISKIT_IBM_INSTANCE=\"your_instance\"\n";
        }

        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "Demo completed successfully!\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
