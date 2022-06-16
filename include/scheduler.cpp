

//template <class In, class Out>
//const map<int, unique_ptr<RedObj>>& Scheduler<In,Out>::get_combination_map() const
//{
//	return combination_map_;
//}

template <class In, class Out>
int Scheduler<In,Out>::get_num_iters() const
{
	return num_iters_;
}

template <class In, class Out>
int Scheduler<In,Out>::get_num_threads() const
{
	return num_threads_;
}

template <class In, class Out>
bool Scheduler<In,Out>::get_glb_combine() const
{
    return glb_combine_;
}
