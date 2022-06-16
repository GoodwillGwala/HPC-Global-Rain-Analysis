#include <utility>
template <class In, class Out>
Scheduler<In, Out>::Scheduler(const SchedArgs& args)
: num_threads_(args.num_threads)
, step_(args.step), extra_data_(args.extra_data)
, num_iters_(args.num_iters)
{
    MPI_Comm_size(MPI_COMM_WORLD, &num_nodes_);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);

    if (rank_ == 0)
    printf("Scheduler: Initializing with %d threads and %d nodes...\n", args.num_threads, num_nodes_);

    assert(args.num_threads > 0);
    assert(args.step > 0);

    if (rank_ == 0)
    printf("Scheduler: Constructing the reduction map for all the threads...\n");

    for (int i = 0; i < num_threads_; ++i)
    {
        std::map<int, std::unique_ptr<RedObj>> loc_reduction_map;
        reduction_map_.emplace_back(std::move(loc_reduction_map));
    }

    if (rank_ == 0)
    printf("Scheduler: Reduction map for %lu threads is ready.\n", reduction_map_.size());


}


//set-up
template <class In, class Out>
void Scheduler<In, Out>::setup(const In* data, size_t total_len, size_t buf_size, Out* out, size_t out_len)
{
    assert(data != nullptr);
    data_ = data;
    assert(total_len > 0);
    total_len_ = total_len;
    assert(buf_size > 0 && buf_size <= total_len);
    buf_size_ = buf_size;
    if (out != nullptr)
    {
        assert(out_len > 0);
        out_ = out;
        out_len_ = out_len;
    }


}


//run-time sharing
template <class In, class Out>
void Scheduler<In, Out>::run_time_sharing(const In* data, size_t total_len, size_t buf_size, Out* out, size_t out_len, MAPPING_MODE_T mode)
{
  std::chrono::time_point<std::chrono::system_clock> clk_beg, clk_end;
  clk_beg = std::chrono::system_clock::now();

  // Clear both reduction and combination maps.
  reduction_map_.clear();
  combination_map_.clear();

  // Set up the scheduler.
  setup(data, total_len, buf_size, out, out_len);

    if (rank_ == 0)
    {
        dprintf("Combination map after processing extra data...\n");
        //dump_combination_map();
    }

  for (int iter = 1; iter <= num_iters_; ++iter)
  {
      //if (rank_ == 0)
      //  dprintf("Scheduler: Iteration# = %d.\n", iter);

      // Set the input data chunk.
      Chunk input(0, buf_size_);

      // Copy global combination map to each (local) combination_map_.
      if (iter > 1)
      {
          distribute_global_combination_map();

      	  dprintf("Combination map after distributing global combination map on node %d...\n", rank_);
      //dump_combination_map();
      }


      // distribution is done in parallel.
      distribute_local_combination_map();

      dprintf("Reduction map after distributing local combination map on node %d...\n", rank_);
      //dump_reduction_map();

      // Process chunks one by one.
      // perform splitting and local reduction.
      int num_bufs_to_process = total_len_ / buf_size_;
      if (total_len_ % buf_size_ != 0)
      {
      	  ++num_bufs_to_process;
      }

    for (int i = 0; i < num_bufs_to_process; ++i)
    {
      dprintf("\nScheduler: Processing the input chunk %d...\n", i + 1);

      if (i != num_bufs_to_process - 1 || total_len_ % buf_size_ == 0)
      {
           process_chunk(input, mode);
           input.start += buf_size_;
      }
      else
      {  // The last iteration does not load a full buffer.

        input.length = total_len_ % buf_size_;
        process_chunk(input, mode);
      }
    }

    dprintf("Reduction map after local reduction...\n");
    //dump_reduction_map();

    // Local combination is done sequentially (or can be at most pairwise parallelized).
    local_combine();

    dprintf("Combination map after local combination...\n");
    //dump_combination_map();


    if (glb_combine_)
      global_combine();

//    dprintf("Combination map after global combination...\n");
    //dump_combination_map();

    // Perform post-combination processing on the master node.
    if (rank_ == 0)
    {
      post_combine(combination_map_);

      dprintf("Global combination map after post-combination at iteration %d...\n", iter);
      dump_combination_map();
    }
  }

  // Ouptut (local) combination_map_ to each node's output destination.
  if (out != nullptr && out_len > 0)
  {
    output();
  }

  clk_end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = clk_end - clk_beg;
  if (num_iters_ > 1 && rank_ == 0)
  {
    dprintf("Scheduler: # of iterations = %d.\n", num_iters_);
  }

  dprintf("Scheduler: Processing time on node %d = %.2f secs.\n", rank_, elapsed_seconds.count());
}





template <class In, class Out>
void Scheduler<In, Out>::process_chunk(const Chunk& input, MAPPING_MODE_T mode)
{
  if (input.empty())
    return;

  assert(input.start >= 0 && input.start + input.length <= total_len_);
  input_ = input;

  // Splitting (or local partitioning) is done by the master thread.
  split();

#pragma omp parallel num_threads(num_threads_)
  {
    reduce(mode);
  }

  // Clear splits_ for the next run.
  splits_.clear();
}


//distribute_global combination

template <class In, class Out>
void Scheduler<In, Out>::distribute_global_combination_map()
{
  dprintf("Scheduler: Distribute global combination map to each (local) combination_map_.\n");

  int num_red_objs = 0;
  if (rank_ == 0)
  {
       num_red_objs = (int)combination_map_.size();
  }

  	   MPI_Bcast(&num_red_objs, 1, MPI_INT, 0, MPI_COMM_WORLD);
  	   dprintf("num_red_objs on node %d = %d\n", rank_, num_red_objs);

  	   // Vectorize global comination map;
  	   int keys[num_red_objs];
  	   size_t length = num_red_objs * red_obj_size_;  // Total length of the serialized reduction objects.
  	   char *red_objs = new char[length];

  if (rank_ == 0)
  {
       int i = 0;
       for (const auto& pair : combination_map_)
       {
		  keys[i] = pair.first;
		  memcpy(&red_objs[i * red_obj_size_], pair.second.get(), red_obj_size_);
		  ++i;
       }
  }

  // Broadcast vectorized combination_map_;
  MPI_Bcast(keys, num_red_objs, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(red_objs, length, MPI_BYTE, 0, MPI_COMM_WORLD);

  // Generate global combination map from received vectorized (local) combination_map_.
  if (rank_ != 0)
  {
      combination_map_.clear();

      for (int i = 0; i < num_red_objs; ++i)
      {
          deserialize(combination_map_[keys[i]], &red_objs[i * red_obj_size_]);
      }
  }

  dprintf("Scheduler: Combination map after distributing global combination map on node %d.\n", rank_);
  //dump_combination_map();

  delete [] red_objs;
}

//Local distribute combination
template <class In, class Out>
void Scheduler<In, Out>::distribute_local_combination_map()
{
    dprintf("Scheduler: Distribute combination_map_ to each local reduction map.\n");

	#pragma omp parallel num_threads(num_threads_)
  	{
		int tid = omp_get_thread_num();
		reduction_map_[tid].clear();

    	for (const auto& pair: combination_map_)
    	{
		    const auto key = pair.first;
		  	reduction_map_[tid][key].reset(combination_map_.find(key)->second->clone());
    	}
  	}
}

//split input data

template <class In, class Out>
void Scheduler<In, Out>::split()
{
    dprintf("Scheduler: Splitting the input data into %d splits...\n", num_threads_);

  	size_t split_length = input_.length / num_threads_;
  	for (int i = 0; i < num_threads_; ++i)
  	{
		Chunk c;
		c.start = input_.start + i * split_length;
		c.length = split_length;
		splits_.emplace_back(c);
  	}

  	if (input_.length % num_threads_ != 0)
  	{
    	splits_.back().length += input_.length % num_threads_;
  	}

  	// Print out each split.
  	for (int i = 0; i < num_threads_; ++i)
  	{
    	dprintf("Split for thread %d: start = %lu, length = %lu.\n", i, splits_[i].start, splits_[i].length);
  	}
}

//Retrieve next chunk

template <class In, class Out>
bool Scheduler<In, Out>::next(std::unique_ptr<Chunk>& unit_chunk, const Chunk& split) const
{
    bool is_last = false;

  	// Retrieve a non-first unit chunk.
  	if (unit_chunk != nullptr)
  	{
		 unit_chunk->start += unit_chunk->length;
	     // The length equals step_, so remain unchanged for the non-last unit chunk.

		size_t split_end = split.start + split.length;
		if (unit_chunk->start + unit_chunk->length >= split_end)
		{
		    unit_chunk->length = split_end - unit_chunk->start;
		    is_last = true;
		}
  	}
  else
  {  // Retrieve the first unit chunk.
    unit_chunk.reset(new Chunk);
    unit_chunk->start = split.start;

    if (step_ < split.length)
    {
      	unit_chunk->length = step_;
    }
    else
    {
      	unit_chunk->length = split.length;
      	is_last = true;
    }
  }

  dprintf("Current unit chunk: start = %lu, length = %lu.\n", unit_chunk->start, unit_chunk->length);
  return is_last;
}

//reduce

template <class In, class Out>
void Scheduler<In, Out>::reduce(MAPPING_MODE_T mode)
{
  int tid = omp_get_thread_num();


  bool is_last = false;
  std::unique_ptr<Chunk> chunk = nullptr;
  do {
    	is_last = next(chunk, splits_[tid]);

    	if (mode == GEN_ONE_KEY_PER_CHUNK)
    	{    // Perform reduction with gen_key.
		  	int key = gen_key(*chunk, data_, combination_map_);
		  	accumulate(*chunk, data_, reduction_map_[tid][key]);
		  	// Check if the early emission can be triggered.
		  	if (reduction_map_[tid].find(key)->second->trigger())
		  	{
		    	dprintf("Scheduler: The reduction object %s is emitted by trigger...\n", reduction_map_[tid].find(key)->second->str().c_str());

		    	assert(key >= 0 && key < static_cast<int>(out_len_));
		    	convert(*reduction_map_[tid].find(key)->second, &out_[key]);
		    	reduction_map_[tid].erase(key);
		   }
    	}
    	else
    	{ // mode == GEN_KEYS_PER_CHUNK, and perform reduction with gen_keys.
		    std::vector<int> keys;
		  	gen_keys(*chunk, data_, keys, combination_map_);

		  	for (int key : keys)
		  	{
		    	accumulate(*chunk, data_, reduction_map_[tid][key]);
		    	// Check if the early emission can be triggered.
		    	if (reduction_map_[tid].find(key)->second->trigger())
		    	{
		      		dprintf("Scheduler: The reduction object %s is emitted by trigger...\n", reduction_map_[tid].find(key)->second->str().c_str());

		      		assert(key >= 0 && key < out_len_);
		      		convert(*reduction_map_[tid].find(key)->second, &out_[key]);
		      		reduction_map_[tid].erase(key);
        		}
      		}
    	}
  	} while (!is_last);


}


//local combine

template <class In, class Out>
void Scheduler<In, Out>::local_combine()
{
  dprintf("Scheduler: Local combination on the master thread on node %d...\n", rank_);

  // Combine all the local reduction maps with the combination map.
  for (int tid = 0; tid < num_threads_; ++tid)
  {
    for (auto& pair : reduction_map_[tid])
    {
      if (combination_map_.find(pair.first) != combination_map_.end())
      {
        merge(*pair.second, combination_map_[pair.first]);
      }
      else
      {
        combination_map_[pair.first] = std::move(pair.second);
      }
    }
  }
}


//global combine

template <class In, class Out>
void Scheduler<In, Out>::global_combine()
{
  dprintf("Scheduler: Global combination...\n");

  int num_nodes;
  MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);
  int local_num_red_objs = (int)combination_map_.size();
  int global_num_red_objs[num_nodes];
  MPI_Gather(&local_num_red_objs, 1, MPI_INT, global_num_red_objs, 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (rank_ == 0)
  {
    for (int i = 1; i < num_nodes; ++i)
    {
      // Receive serialized reduciton objects from slave nodes.
      int keys[global_num_red_objs[i]];
      size_t length = red_obj_size_ * global_num_red_objs[i];  // Total length of the serialized reduction objects.
      char* red_objs = new char[red_obj_size_ * global_num_red_objs[i]];

      MPI_Status status;
      MPI_Recv(keys, global_num_red_objs[i], MPI_INT, i, i, MPI_COMM_WORLD, &status);
      MPI_Recv(red_objs, length, MPI_BYTE, i, i + num_nodes, MPI_COMM_WORLD, &status);

      // Deserialize reduction objects and add them to the global combination
      // map.
      for (int j = 0; j < global_num_red_objs[i]; ++j)
      {
          std::unique_ptr<RedObj> red_obj;
          deserialize(red_obj, &red_objs[j * red_obj_size_]);
       	  assert(red_obj != nullptr);

          if (combination_map_.find(keys[j]) != combination_map_.end())
          {
              merge(*red_obj, combination_map_[keys[j]]);
          }
          else
          {
              combination_map_[keys[j]] = std::move(red_obj);
          }
      }

      delete [] red_objs;
    }
  }
	 else
	 {
	    // Serialize reduction objects.
		int i = 0;
		int local_keys[local_num_red_objs];
		size_t length = red_obj_size_ * local_num_red_objs;  // Total length of the serialized reduction objects.
		char* local_red_objs = new char[length];

		for (const auto& pair : combination_map_)
		{
		    local_keys[i] = pair.first;
		    memcpy(&local_red_objs[i * red_obj_size_], pair.second.get(), red_obj_size_);
		  	++i;
		}

		  // Send serialized data to the master node.
		  MPI_Send(local_keys, local_num_red_objs, MPI_INT, 0, rank_, MPI_COMM_WORLD);
		  MPI_Send(local_red_objs, length, MPI_BYTE, 0, rank_ + num_nodes, MPI_COMM_WORLD);
	  }
}

//Output

template <class In, class Out>
void Scheduler<In, Out>::output()
{
  for (const auto& pair: combination_map_)
  {
    int key = pair.first;
    assert(key >= 0 && key < out_len_);

    // Convert a reduction object into a desired output element.
    convert(*pair.second.get(), &out_[key]);
  }
}


//dump reduction map

template <class In, class Out>
void Scheduler<In, Out>::dump_reduction_map() const
{
  dprintf("Reduction map on node %d:\n", rank_);
  for (int i = 0; i < num_threads_; ++i)
  {
    dprintf("\tLocal reduciton map %d:\n", i);
    for (const auto& pair : reduction_map_[i])
    {
      dprintf("\t\t(key = %d, value = %s)\n", pair.first, (pair.second != nullptr ? pair.second->str().c_str() : "NULL"));
    }
  }
}

//dump combination map

template <class In, class Out>
void Scheduler<In, Out>::dump_combination_map() const
{
  dprintf("Combination map on node %d:\n", rank_);
  for (const auto& pair : combination_map_)
  {
    dprintf("\t(key = %d, value = %s)\n", pair.first, (pair.second != nullptr ? pair.second->str().c_str() : "NULL"));
  }
}
