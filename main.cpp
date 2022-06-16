#include <chrono>
#include <memory>
#include <mpi.h>
#include <algorithm>
#include <utility>
#include <tuple>
#include <iterator>

#include <utility.h>

#include "Box_and_Whiskers.h"
#include "netcdf_partitioner.h"
#include "partitioner.h"
#include "scheduler.h"

#include <unistd.h>

#define STEP  1

#define FILENAME "data.nc"
#define VARNAME "precip"

#define CLUSTER 1

#define PRINT_COMBINATION_MAP 1
#define PRINT_OUTPUT 1

using namespace std;





int main(int argc, char* argv[])
{

    // MPI initialization.


  	int mpi_status = MPI_Init(&argc, &argv);
  	if (mpi_status != MPI_SUCCESS)
  	{
    	  printf("Failed to initialize MPI environment.\n");
    	  MPI_Abort(MPI_COMM_WORLD, mpi_status);
  	}

  	int rank;
  	MPI_Comm_rank(MPI_COMM_WORLD, &rank);





  	auto NUM_THREADS = omp_get_num_procs();

  	if(CLUSTER)
	NUM_THREADS = std::max(atoi(std::getenv("OMP_NUM_THREADS")), NUM_THREADS);




	omp_set_nested(1);

//	Load data
  	chrono::time_point<chrono::system_clock> clk_beg, clk_end;
  	clk_beg = chrono::system_clock::now();

  	unique_ptr<Partitioner> p(new NetCDFPartitioner(FILENAME, VARNAME, STEP));

  	p->load_partition();

  	const size_t out_len = p->get_len();
  	float* out = new float[out_len];
  	float* in  = (float*)p->get_data();

  	std::stable_partition(in, in+p->get_len(), [](const auto n){return n>0 && n<2000;});
  	auto new_last = distance( in, find_if( in, in+out_len, [](auto x) { return x == 0; }));



  	clk_end = chrono::system_clock::now();
  	chrono::duration<float> sim_seconds = clk_end - clk_beg;
//	Finish load


  	if (rank == 0)
  	{
        printf("Simulation time = %.2f secs.\n", sim_seconds.count());
      	printf("Simulation data is ready...\n");
  	}

// Execute Data
  	SchedArgs args(NUM_THREADS, STEP);
  	unique_ptr<Scheduler<float, float>> win_app(new BoxAndWhiskers<float, float>(args));
  	win_app->set_red_obj_size(sizeof(WinObj));
  	win_app->set_glb_combine(false);
  	win_app->run2(in, p->get_len(), out, out_len);
// 	Finish Data execution

  	clk_end = chrono::system_clock::now();
  	chrono::duration<float> elapsed_seconds = clk_end - clk_beg;


  	if (rank == 0)
    printf("Analytics time = %.2f secs.\n", elapsed_seconds.count() - sim_seconds.count());

  	printf("Total processing time on node %d = %.2f secs.\n", rank, elapsed_seconds.count());

//  Print combination map
  	if (PRINT_COMBINATION_MAP && rank == 0 )
  	{
    	printf("\n");
    	win_app->dump_combination_map();
  	}

//  Print out the final result on the master node if required.
	if (PRINT_OUTPUT && rank == 0)
	{
		printf("Final output on the master node:\n");

		auto [North, South] = PartitionHemispheres(out, out + p->get_len() );

		{
		auto [Q1, Q2, Q3] = Quatiles(out, North);

		std::cout<<"\n\nNorth Statistics\n\n";

		std::cout<<"North Min: "<<(Q1-1.5*(Q3-Q1))<<std::endl;
		std::cout<<"North Q1: "<<Q1<<std::endl;
		std::cout<<"North Q2: "<<Q2<<std::endl;
		std::cout<<"North Q3: "<<Q3<<std::endl;
		std::cout<<"North Max: "<<(Q3+1.5*(Q3-Q1))<<std::endl;
		}

		{
		auto [Q11, Q22, Q33] = Quatiles(North, South);

		std::cout<<"\nSouth Statistics\n\n";

		std::cout<<"South Min: "<<(Q11-1.5*(Q33-Q11))<<std::endl;
		std::cout<<"South Q1: "<<Q11<<std::endl;
		std::cout<<"South Q2: "<<Q22<<std::endl;
		std::cout<<"South Q3: "<<Q33<<std::endl;
		std::cout<<"South Max: "<<(Q33+1.5*(Q33-Q11));
		}
		std::cout<<"\n\n\n\n";




		std::cout<<"\nGlobal Statistics\n\n";
		{
		auto [Q111, Q222, Q333] = Quatiles(out, South);
		std::cout<<"Global Min: "<<(Q111-1.5*(Q333-Q111))<<std::endl;
		std::cout<<"Global Q1: "<<Q111<<std::endl;
		std::cout<<"Global Q2: "<<Q222<<std::endl;
		std::cout<<"Global Q3: "<<Q333<<std::endl;
		std::cout<<"Global Max: "<<(Q333+1.5*(Q333-Q111));
		}
		std::cout<<"\n\n\n\n";



		clk_end = chrono::system_clock::now();
  		chrono::duration<float> elapsed_seconds = clk_end - clk_beg;


  	if (rank == 0)
    printf("Analytics time = %.2f secs.\n", elapsed_seconds.count() - sim_seconds.count());

  	printf("Total processing time on node %d = %.2f secs.\n", rank, elapsed_seconds.count());

//	  	std::copy(North, South, std::ostream_iterator<float>(std::cout, " "));

	}// end if statement




//  Distribution Analysis



  delete [] out;

  MPI_Finalize();

  return 0;
}
