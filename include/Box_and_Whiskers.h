#ifndef	_BOX_WHISKERS_H_
#define	_BOX_WHISKERS_H_

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <sstream>
#include <vector>
#include <utility>

#include "chunk.h"
#include "scheduler.h"

using namespace std;

#define RADIUS 1
#define WIN_SIZE   2*RADIUS+1



// Sliding window object.
struct WinObj : public RedObj
{
    float win[WIN_SIZE];
    size_t count = 0;
    float max = 0;
    float min = 0;
    float upper_q = 0;
    float lower_q = 0;
    float median = 0;


    string str() const override
    {
	      stringstream ss;
	      ss << "(win = [";
	      for (size_t i = 0; i < count - 1; ++i) {
		      ss << win[i] << " ";
	      }
	      ss << win[count - 1];

	      return "(win = [" + ss.str() + "], count = " + to_string(count) + ")";
    }

    // Trigger early emission when the count reaches WIN_SIZE.
    bool trigger() const override
    {
	    return count >= WIN_SIZE;
    }
};


template <class In, class Out>
class BoxAndWhiskers : public Scheduler<In, Out>
{
	public:
		using Scheduler<In, Out>::Scheduler;

		// Group elements into buckets.
		void gen_keys(const Chunk& chunk, const In* data, vector<int>& keys, const map<int, unique_ptr<RedObj>>& combination_map) const override
        {
			dprintf("For chunk[%lu], genrate key %lu from node%d...\n", chunk.start, chunk.start, this->rank_);
			keys.emplace_back(chunk.start);

			for (size_t i = chunk.start + 1; i <= min(chunk.start + RADIUS, this->total_len_ - 1); ++i)
            {
				dprintf("For chunk[%lu], genrate key %lu from node%d...\n", chunk.start, i, this->rank_);
				keys.emplace_back(i);
			}


			if (chunk.start >= RADIUS)
            {
				for (size_t i = chunk.start - RADIUS; i < chunk.start; ++i)
                {
					dprintf("%.2f For chunk[%lu], genrate key %lu from node%d...\n", chunk.start, i, this->rank_);
					keys.emplace_back(i);
				}
			}
            else
            {
				for (size_t i = 0; i < chunk.start; ++i)
                {
					dprintf("For chunk[%lu], genrate key %lu from node%d...\n", chunk.start, i, this->rank_);
					keys.emplace_back(i);
				}
			}
		}


		float data_point(const float element)
		{
			return std::max(0.f,element);

		}

		// Accumulate the window.
		void accumulate(const Chunk& chunk, const In* data, unique_ptr<RedObj>& red_obj) override
        {
			if (red_obj == nullptr)
            {
				red_obj.reset(new WinObj);
			}

			WinObj* w = static_cast<WinObj*>(red_obj.get());
			assert(w->count + chunk.length <= WIN_SIZE);


			auto element = 0.f;
			for (size_t i = 0; i < chunk.length; ++i)
            {

				element  = data_point((float)data[chunk.start + i]	);

				dprintf("Adding the element chunk[%lu] = %.3f.\n", chunk.start + i, element);
				w->win[w->count++] = element;
				w->max = std::max(w->max, element);
				w->min = std::min(w->min, element);

			}

			dprintf("After local reduction, w = %s.\n", w->str().c_str());
		}

		// Merge the two windows.
		void merge(const RedObj& red_obj, unique_ptr<RedObj>& com_obj) override
        {
			const WinObj* wr = static_cast<const WinObj*>(&red_obj);
			WinObj* wc = static_cast<WinObj*>(com_obj.get());

			assert(wr->count + wc->count <= WIN_SIZE);

			for (size_t i = 0; i < wr->count; ++i)
            {
				wc->win[wc->count++] = wr->win[i];
				wc->max = std::max(wr->max, wr->win[i]);
				wc->min = std::min(wr->min, wr->win[i]);



			}
		}

		// Deserialize reduction object.
		void deserialize(unique_ptr<RedObj>& obj, const char* data) const override
        {
			obj.reset(new WinObj);
			memcpy(obj.get(), data, sizeof(WinObj));
		}

		// Convert a reduction object into a desired output element.
		void convert(const RedObj& red_obj, Out* out) const override
        {
			const WinObj* w = static_cast<const WinObj*>(&red_obj);

			vector<float> v;
			v.assign(w->win, w->win + w->count);

			sort(v.begin(), v.end());

			if (w->count % 2 != 0)
            {
            	*out = v[w->count / 2];
			}
            else
            {
				if((v[w->count / 2 - 1] + v[w->count / 2]) / 2 > 0.000001f)
				*out = (v[w->count / 2 - 1] + v[w->count / 2]) / 2;
			}

		}
};

#endif
