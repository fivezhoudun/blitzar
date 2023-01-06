#include "sxt/proof/inner_product/random_product_generation.h"

#include <cassert>

#include "sxt/base/memory/alloc_utility.h"
#include "sxt/base/num/ceil_log2.h"
#include "sxt/curve21/type/element_p3.h"
#include "sxt/proof/inner_product/proof_descriptor.h"
#include "sxt/ristretto/random/element.h"
#include "sxt/scalar25/random/element.h"
#include "sxt/scalar25/type/element.h"

namespace sxt::prfip {
//--------------------------------------------------------------------------------------------------
// generate_random_product
//--------------------------------------------------------------------------------------------------
void generate_random_product(proof_descriptor& descriptor, basct::cspan<s25t::element>& a_vector,
                             basn::fast_random_number_generator& rng, basm::alloc_t alloc,
                             size_t n) noexcept {
  assert(n > 0);

  // a_vector, b_vector
  auto a_vector_p = basm::allocate_array<s25t::element>(alloc, n);
  auto b_vector = basm::allocate_array<s25t::element>(alloc, n);
  for (size_t i = 0; i < n; ++i) {
    s25rn::generate_random_element(a_vector_p[i], rng);
    s25rn::generate_random_element(b_vector[i], rng);
  }
  a_vector = {a_vector_p, n};
  descriptor.b_vector = {b_vector, n};

  // q_value
  auto q_value = basm::allocate_object<c21t::element_p3>(alloc);
  rstrn::generate_random_element(*q_value, rng);
  descriptor.q_value = q_value;

  // g_vector
  auto n_lg2 = basn::ceil_log2(n);
  auto np = 1ull << n_lg2;
  auto g_vector = basm::allocate_array<c21t::element_p3>(alloc, np);
  for (size_t i = 0; i < np; ++i) {
    rstrn::generate_random_element(g_vector[i], rng);
  }
  descriptor.g_vector = {g_vector, np};
}
} // namespace sxt::prfip
