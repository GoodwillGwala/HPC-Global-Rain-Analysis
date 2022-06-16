#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <cassert>
#include <chrono>
#include <cstring>
#include <ctime>
#include <map>
#include <memory>
#include <mpi.h>
#include <omp.h>
#include <sstream>
#include <unistd.h>
#include <vector>
#include <mutex>
#include "chunk.h"



using namespace std;

/* Debug printf */
#define dprintf(...)  //printf(__VA_ARGS__)

// Base reduction object.
struct RedObj
{

    virtual void reset() {}


    virtual RedObj* clone()
    {
        return new RedObj(*this);
    }

    // Convert itself to a string.
    virtual string str() const
    {
        stringstream ss;
        ss << this;
        return ss.str();
    }

    // Trigger to emit reduction object.
    // The default setting returns false.
    virtual bool trigger() const
    {
        return false;
    }

    virtual ~RedObj() {}
};

// Scheduler arguments.
struct SchedArgs
{
    int num_threads;
    size_t step;
    const void* extra_data;
    int num_iters;

    SchedArgs(int n, size_t s, const void* e = nullptr, int n_iters = 1)
    : num_threads(n)
    , step(s)
    , extra_data(e)
    , num_iters(n_iters)
    {}
};

// Mapping mode, including 1-1 mapping and 1-n mapping (flatmap).
enum MAPPING_MODE_T
{
    GEN_ONE_KEY_PER_CHUNK,
    GEN_KEYS_PER_CHUNK,
};

template <class In, class Out>
class Scheduler
{
   public:

       typedef unique_lock<mutex> Lock;
       mutex m_guard;
       explicit Scheduler(const SchedArgs& args);


       void run_time_sharing(const In* data, size_t total_len, size_t buf_size, Out* out, size_t out_len, MAPPING_MODE_T mode);


       void run2(const In* data, size_t total_len, Out* out, size_t out_len)
       {
            run_time_sharing(data, total_len, total_len, out, out_len, GEN_KEYS_PER_CHUNK);

       }

        void run(const In* data, size_t total_len, Out* out, size_t out_len)
       {
            run_time_sharing(data, total_len, total_len, out, out_len, GEN_ONE_KEY_PER_CHUNK);

       }

      /* Getters */

      const map<int, unique_ptr<RedObj>>& get_combination_map() const
      {

      	return combination_map_;

      }


      // Retrieve num_iters_.
      int get_num_iters() const;


      // Retrieve num_threads_.
      int get_num_threads() const;


      // Retrieve glb_combine_.
      bool get_glb_combine() const;


      // Set the derived reduciton object size.
      void set_red_obj_size(size_t size)
      {
        red_obj_size_ = size;
      }

      // Set glb_combine_.
      void set_glb_combine(bool flag)
      {
        glb_combine_ = flag;
      }

      /* Debugging Functions */
      void dump_reduction_map() const;
      void dump_combination_map() const;



      // Generate an (integer) key given a chunk.
      virtual int gen_key(const Chunk& chunk, const In* data, const map<int, unique_ptr<RedObj>>& combination_map) const
      {
            return -1;
      }

      // Generate (integer) keys given a chunk.
      virtual void gen_keys(const Chunk& chunk, const In* data, vector<int>& keys, const map<int, unique_ptr<RedObj>>& combination_map) const
      {}

      // Accumulate the chunk on a reduction object.
      virtual void accumulate(const Chunk& chunk, const In* data, unique_ptr<RedObj>& red_obj) = 0;

      // Merge the first reduction object into the second reduction object, i.e.,
      // combination object.
      virtual void merge(const RedObj& red_obj, unique_ptr<RedObj>& com_obj) = 0;



      // Perform post-combination processing.
      virtual void post_combine(map<int, unique_ptr<RedObj>>& combination_map)
      {}

      // Construct a reduction object from serialized reduction object.
      virtual void deserialize(unique_ptr<RedObj>& obj, const char* data) const = 0;

      // Convert a reduction object to an output result.
      virtual void convert(const RedObj& red_obj, Out* out) const
      {}


      // Set up data processing by binding the input data.
      void setup(const In* data, size_t total_len, size_t buf_size, Out* out, size_t out_len);


      void process_chunk(const Chunk& input, MAPPING_MODE_T mode);

      // Copy global combination map to each (local) combination_map_.
      void distribute_global_combination_map();

      // Copy combination_map_ to each local reduction map.
      void distribute_local_combination_map();

      // Combine all the local reduction maps into a combination map.
      void local_combine();

      // Combine all the combination maps on slave nodes
      // into a global combination map on the master node.
      void global_combine();


      virtual void output();

      virtual ~Scheduler()
      {

      }

   protected:

      virtual void split();

     // Retrieve the next unit chunk in the local split.
     // Return true if the last unit chunk in the local split has been retrieved.
     virtual bool next(unique_ptr<Chunk>& unit_chunk, const Chunk& split) const;

     // Perform the local reduction.
     void reduce(MAPPING_MODE_T mode);

     int num_threads_;
     int num_nodes_;
     int rank_;

     /* Input Array Data */
     const In* data_ = nullptr;
     size_t total_len_ = 0;
     Chunk input_; // Input layout on the data source provided by data_.
     size_t buf_size_ = 0;  // Input data buffer

     /* Output Array Data */
     Out* out_ = nullptr;
     size_t out_len_ = 0;

     /* Scheduler Shared Data */
     size_t step_;  // The size of unit chunk for each single read, which groups a bunch of elements for mapping and reducing.
     const void* extra_data_;  // Extra input data
     int num_iters_;  // The # of iterations.
     size_t red_obj_size_;  // The size of the derived reduction object, used for reduction object serialization and message passing.


     vector<Chunk> splits_;  // The length equals num_threads_.
     vector<map<int, unique_ptr<RedObj>>> reduction_map_;  // The vector length equals the number of threads, and each map is a local reduction map.
     map<int, unique_ptr<RedObj>> combination_map_;  // (Local) combination map which holds the local combination results, after global combination, combination_map_ on the master node is global combination map.
     bool glb_combine_ = true;  // Enable/disable global combination. (Enable by default.)



};
#include "scheduler.inl"
#include "scheduler.cpp"
#endif
