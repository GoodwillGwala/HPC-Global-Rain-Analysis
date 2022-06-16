#include <utility>
#include <tuple>
#include <iterator>
#include <algorithm>



template <typename Iterator>
double median(Iterator begin, Iterator end)
{
	Iterator middle = begin + (end - begin) / 2;

	std::nth_element(begin, middle, end);

	if ((end - begin) % 2 != 0)
	{ // odd length
		return *middle;
	}
	else
	{ // even length

		Iterator lower_middle = std::max_element(begin, middle);
		return (*middle + *lower_middle) / 2.0;
	}
}


template <class ForwardIt>
std::tuple<ForwardIt, ForwardIt> PartitionHemispheres(ForwardIt first, ForwardIt last)
{

//	This will partition while keeping relative order,  all zero's will be at the end
	std::stable_partition(first, last, [](const auto n){return n>0;});


//  Find position of first zero
	auto new_last = std::distance( first, std::find_if( first, last, [](auto x) { return x == 0; }));
	auto lastItr = first + new_last;

//	partition the data into north and south
	auto pivot = std::next(first, std::distance(first,lastItr)/2);

	return std::make_tuple(pivot, lastItr);


}


template <class ForwardIt>
std::tuple<double, double, double> Quatiles(ForwardIt first, ForwardIt last)
{

	std::nth_element(first, first, last, std::less<double>());

	auto Q2 = median(first, last);

	auto pivot1 = std::next(first, std::distance(first,last)/2);

	auto Q1    = median(first, pivot1);

	auto pivot2 = std::next(first, std::distance( pivot1,last)/2);

	auto Q3		= median( pivot2, last);

	std::nth_element(pivot2, last, last, std::greater<double>());



	return std::make_tuple(Q1, Q2, Q3);

}
