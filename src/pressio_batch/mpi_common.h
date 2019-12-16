#include <mpi.h>
#include <map>
#include <vector>
#include <string>

struct pressio_metrics;

static constexpr int ROOT = 0;
static constexpr int TASK_DONE = 0;

struct work_request {
  int task_id; //task_id=0 means stop, task_id!=0 means valid task
  int compressor_id;
  int dataset_id;
};

struct work_response {
  int task_id;
  int field_id;
  int done;
  double value;
};
extern MPI_Datatype work_request_t;
extern MPI_Datatype work_response_t;

void setup_types();

void free_types(); 

std::map<int, std::string> init_fieldnames(std::vector<std::string> const& fields, pressio_metrics* metrics); 
