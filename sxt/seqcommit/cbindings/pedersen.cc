#include "sxt/seqcommit/cbindings/pedersen.h"

#include <memory>
#include <iostream>
#include <cuda_runtime.h>

#include "sxt/base/container/span.h"
#include "sxt/seqcommit/base/commitment.h"
#include "sxt/multiexp/base/exponent_sequence.h"
#include "sxt/memory/management/managed_array.h"
#include "sxt/seqcommit/cbindings/pedersen_backend.h"
#include "sxt/seqcommit/cbindings/pedersen_cpu_backend.h"
#include "sxt/seqcommit/cbindings/pedersen_gpu_backend.h"

using namespace sxt;

//--------------------------------------------------------------------------------------------------
// backend
//--------------------------------------------------------------------------------------------------
static sqccb::pedersen_backend* backend = nullptr;

//--------------------------------------------------------------------------------------------------
// get_num_devices
//--------------------------------------------------------------------------------------------------
static int get_num_devices() {
  int num_devices;

  auto rcode = cudaGetDeviceCount(&num_devices);

  if (rcode != cudaSuccess) {
    num_devices = 0; 

    std::cout << "cudaGetDeviceCount failed: " << cudaGetErrorString(rcode)
              << "\n";
  }

  return num_devices;
}

//--------------------------------------------------------------------------------------------------
// sxt_init
//--------------------------------------------------------------------------------------------------
int sxt_init(const sxt_config* config) {
  if (config == nullptr) exit(1);
  if (backend != nullptr) exit(1);

  if (config->backend == SXT_BACKEND_CPU) {
    backend = sqccb::get_pedersen_cpu_backend();
    return 0;
  } else if (config->backend == SXT_BACKEND_GPU) {
    int num_devices = get_num_devices();

    if (num_devices > 0) {
      backend = sqccb::get_pedersen_gpu_backend();
    } else {
      backend = sqccb::get_pedersen_cpu_backend();

      std::cout << "WARN: Using 'compute_commitments_cpu'. " << std::endl;
    }

    return 0;
  }

  return 1;
}

//--------------------------------------------------------------------------------------------------
// validate_descriptor
//--------------------------------------------------------------------------------------------------
static int validate_descriptor(uint64_t &longest_sequence, const sxt_sequence_descriptor *descriptor) {
  // verify if data pointers are validate
  if (descriptor->n > 0 && descriptor->data == nullptr) return 1;

  // verify if word size is inside the correct range (1 to 32)
  if (descriptor->element_nbytes == 0 || descriptor->element_nbytes > 32) return 1;

  longest_sequence = std::max(longest_sequence, descriptor->n);

  return 0;
}

//--------------------------------------------------------------------------------------------------
// validate_sequence_descriptor
//--------------------------------------------------------------------------------------------------
static int validate_sequence_descriptor(uint64_t &longest_sequence,
  uint32_t num_sequences, const sxt_sequence_descriptor* descriptors) {

  longest_sequence = 0;

  // invalid pointers
  if (descriptors == nullptr) return 1;

  // verify if each descriptor is inside the ranges
  for (uint32_t commit_index = 0; commit_index < num_sequences; ++commit_index) {
        if (validate_descriptor(longest_sequence, descriptors + commit_index)) return 1;
  }

  return 0;
}

//--------------------------------------------------------------------------------------------------
// process_compute_pedersen_commitments
//--------------------------------------------------------------------------------------------------
static int process_compute_pedersen_commitments(
    struct sxt_ristretto_element* commitments,
    uint32_t num_sequences,
    const struct sxt_sequence_descriptor* descriptors,
    struct sxt_ristretto_element* generators) {

  if (num_sequences == 0) return 0;

  // backend not initialized (sxt_init not called correctly)
  if (backend == nullptr) return 1;

  uint64_t longest_sequence = 0;
  
  // verify if input is validate
  if (commitments == nullptr
        || validate_sequence_descriptor(longest_sequence, num_sequences, descriptors)) return 1;

  static_assert(sizeof(sqcb::commitment) == 
      sizeof(sxt_ristretto_element), "types must be ABI compatible");

  basct::span<sqcb::commitment> commitments_result(
            reinterpret_cast<sqcb::commitment *>(commitments), num_sequences);

  memmg::managed_array<mtxb::exponent_sequence> sequences(num_sequences);

  static_assert(sizeof(mtxb::exponent_sequence) == 
      sizeof(sxt_sequence_descriptor), "types must be ABI compatible");

  // populating sequence object
  for (uint64_t i = 0; i < num_sequences; ++i) {
    sequences[i] = *(reinterpret_cast<const mtxb::exponent_sequence *>(&descriptors[i]));
  }

  basct::cspan<mtxb::exponent_sequence> value_sequences(sequences.data(),
                                                        num_sequences);
  
  if (generators == nullptr) longest_sequence = 0;

  basct::span<sqcb::commitment> generators_span(
       reinterpret_cast<sqcb::commitment *>(generators), longest_sequence);

  backend->compute_commitments(commitments_result, value_sequences, generators_span);

  return 0;
}

//--------------------------------------------------------------------------------------------------
// sxt_compute_pedersen_commitments
//--------------------------------------------------------------------------------------------------
int sxt_compute_pedersen_commitments(
    sxt_ristretto_element* commitments, uint32_t num_sequences,
    const sxt_sequence_descriptor* descriptors) {

  int ret = process_compute_pedersen_commitments(
    commitments, num_sequences, descriptors, nullptr
  );

  return ret;
}

//--------------------------------------------------------------------------------------------------
// sxt_compute_pedersen_commitments
//--------------------------------------------------------------------------------------------------
int sxt_compute_pedersen_commitments_with_generators(
    struct sxt_ristretto_element* commitments,
    uint32_t num_sequences,
    const struct sxt_sequence_descriptor* descriptors,
    struct sxt_ristretto_element* generators) {
  
  // we only verify if generators is null, but we
  // expect it to have size equal to the longest
  // row in sequence
  if (generators == nullptr) return 1;

  int ret = process_compute_pedersen_commitments(
    commitments, num_sequences, descriptors, generators
  );

  return ret;
}

//--------------------------------------------------------------------------------------------------
// sxt_get_generators
//--------------------------------------------------------------------------------------------------
int sxt_get_generators(
    struct sxt_ristretto_element* generators,
    uint64_t num_generators,
    uint64_t offset_generators) {

  // if no generator specified, then ignore the function call
  if (num_generators == 0) return 0;

  // at least one generator to be computed, but null array pointer
  if (num_generators > 0 && generators == nullptr) return 1;

  // backend not initialized (sxt_init not called correctly)
  if (backend == nullptr) return 1;

  basct::span<sqcb::commitment> generators_result(
    reinterpret_cast<sqcb::commitment *>(generators), num_generators
  );

  backend->get_generators(generators_result, offset_generators);

  return 0;
}
