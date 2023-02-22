#include "sxt/multiexp/multiproduct_gpu/completion.h"

#include <vector>

#include "sxt/algorithm/reduction/test_reducer.h"
#include "sxt/base/test/unit_test.h"

using namespace sxt;
using namespace sxt::mtxmpg;

TEST_CASE("we can complete a GPU multiproduct computation") {
  SECTION("we handle a single reduction with a single block") {
    block_computation_descriptor descriptors[1];
    descriptors[0].reduction_num_blocks = 1;
    uint64_t products[1];
    uint64_t block_results[] = {123};
    complete_multiproduct<algr::test_add_reducer>(products, descriptors, block_results);
    REQUIRE(products[0] == 123);
  }

  SECTION("we handle a single reduction with multiple blocks") {
    block_computation_descriptor descriptors[2];
    descriptors[0].reduction_num_blocks = 2;
    descriptors[1].reduction_num_blocks = 2;
    uint64_t products[1];
    uint64_t block_results[] = {2, 4};
    complete_multiproduct<algr::test_add_reducer>(products, descriptors, block_results);
    REQUIRE(products[0] == 6);
  }

  SECTION("we handle multiple reductions with single blocks") {
    block_computation_descriptor descriptors[2];
    descriptors[0].reduction_num_blocks = 1;
    descriptors[1].reduction_num_blocks = 1;
    uint64_t products[2];
    uint64_t block_results[] = {2, 4};
    complete_multiproduct<algr::test_add_reducer>(products, descriptors, block_results);
    REQUIRE(products[0] == 2);
    REQUIRE(products[1] == 4);
  }

  SECTION("we handle multiple reductions with varying number of blocks") {
    block_computation_descriptor descriptors[3];
    descriptors[0].reduction_num_blocks = 1;
    descriptors[1].reduction_num_blocks = 2;
    descriptors[2].reduction_num_blocks = 2;
    uint64_t products[2];
    uint64_t block_results[] = {2, 4, 10};
    complete_multiproduct<algr::test_add_reducer>(products, descriptors, block_results);
    REQUIRE(products[0] == 2);
    REQUIRE(products[1] == 14);
  }
}
