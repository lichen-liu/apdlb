#include "kbm_utils.hpp"
#include "suap_pool.hpp" 

void matvecp_kernel(size_t lower,size_t upper){
ERT::SUAP_POOL __apert_ert_pool(2);
__apert_ert_pool.start();
{
  const size_t offset = 1;
  const size_t scale = 20;
  const size_t range = upper - lower;
// Input mats
  float ***mats = new float **[range];
  for (size_t iteration = 0; iteration <= range - 1; iteration += 1) {
    const size_t n = offset + (iteration + lower) * scale;
    mats[iteration] = (new float *[n]);{
std::vector<ERT::RAW_TASK> __apert_ert_tasks;
// ================ APERT ================
    for (size_t i = 0; i <= n - 1; i += 1) {
auto __apert_ert_task = [=]()
{
      mats[iteration][i] = (new float [n]);
    };
__apert_ert_tasks.emplace_back(std::move(__apert_ert_task));
}
__apert_ert_pool.execute(std::move(__apert_ert_tasks));
}
  }
// Input vecs
  float **vecs = new float *[range];{
std::vector<ERT::RAW_TASK> __apert_ert_tasks;
// ================ APERT ================
  for (size_t iteration = 0; iteration <= range - 1; iteration += 1) {
auto __apert_ert_task = [=]()
{
    const size_t n = offset + (iteration + lower) * scale;
    vecs[iteration] = (new float [n]);
  };
__apert_ert_tasks.emplace_back(std::move(__apert_ert_task));
}
__apert_ert_pool.execute(std::move(__apert_ert_tasks));
}
// Output vecs
  float **ress = new float *[range];{
std::vector<ERT::RAW_TASK> __apert_ert_tasks;
// ================ APERT ================
  for (size_t iteration = 0; iteration <= range - 1; iteration += 1) {
auto __apert_ert_task = [=]()
{
    const size_t n = offset + (iteration + lower) * scale;
    ress[iteration] = (new float [n]);
  };
__apert_ert_tasks.emplace_back(std::move(__apert_ert_task));
}
__apert_ert_pool.execute(std::move(__apert_ert_tasks));
}{
std::vector<ERT::RAW_TASK> __apert_ert_tasks;
// Main computation loop
// ================ APERT ================
  for (size_t iteration = lower; iteration <= upper - 1; iteration += 1) {
auto __apert_ert_task = [=]()
{
    const size_t n = offset + (iteration + lower) * scale;
// Generate random data
    int seed = static_cast < int  >  (iteration);
    for (size_t i = 0; i <= n - 1; i += 1) {
      for (size_t j = 0; j <= n - 1; j += 1) {
        seed = seed * 0x343fd + 0x269EC3;
// a=214013, b=2531011
        float rand_val = (seed / 65536 & 0x7FFF);
        mats[iteration][i][j] = (static_cast < float  >  (rand_val)) / (static_cast < float  >  (2147483647));
      }
    }
    for (size_t i = 0; i <= n - 1; i += 1) {
      seed = seed * 0x343fd + 0x269EC3;
// a=214013, b=2531011
      float rand_val = (seed / 65536 & 0x7FFF);
      vecs[iteration][i] = (static_cast < float  >  (rand_val)) / (static_cast < float  >  (2147483647));
    }
// Computation
    for (size_t row_idx = 0; row_idx <= n - 1; row_idx += 1) {
      float result = 0;
      for (size_t col_idx = 0; col_idx <= n - 1; col_idx += 1) {
        result += mats[iteration][row_idx][col_idx] * vecs[iteration][col_idx];
      }
      ress[iteration][row_idx] = result;
    }
  };
__apert_ert_tasks.emplace_back(std::move(__apert_ert_task));
}
__apert_ert_pool.execute(std::move(__apert_ert_tasks));
}{
std::vector<ERT::RAW_TASK> __apert_ert_tasks;
// ================ APERT ================
  for (size_t iteration = 0; iteration <= range - 1; iteration += 1) {
auto __apert_ert_task = [=]()
{
    delete []ress[iteration];
// 2nd dim
  };
__apert_ert_tasks.emplace_back(std::move(__apert_ert_task));
}
__apert_ert_pool.execute(std::move(__apert_ert_tasks));
}
  delete []ress;{
std::vector<ERT::RAW_TASK> __apert_ert_tasks;
// 1st dim
// ================ APERT ================
  for (size_t iteration = 0; iteration <= range - 1; iteration += 1) {
auto __apert_ert_task = [=]()
{
    delete []vecs[iteration];
// 2nd dim
  };
__apert_ert_tasks.emplace_back(std::move(__apert_ert_task));
}
__apert_ert_pool.execute(std::move(__apert_ert_tasks));
}
  delete []vecs;{
std::vector<ERT::RAW_TASK> __apert_ert_tasks;
// 1st dim
// ================ APERT ================
  for (size_t iteration = 0; iteration <= range - 1; iteration += 1) {
auto __apert_ert_task = [=]()
{
    const size_t n = offset + (iteration + lower) * scale;
    for (size_t i = 0; i <= n - 1; i += 1) {
      delete []mats[iteration][i];
// 3rd dim
    }
    delete []mats[iteration];
// 2nd dim
  };
__apert_ert_tasks.emplace_back(std::move(__apert_ert_task));
}
__apert_ert_pool.execute(std::move(__apert_ert_tasks));
}
  delete []mats;
// 1st dim
}
}

int main(int argc,char *argv[])
{
  const double start_time = get_time_stamp();
  matvecp_kernel(0,200);
  print_elapsed(argv[0],start_time);
}
