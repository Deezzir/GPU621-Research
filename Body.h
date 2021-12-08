#pragma once
#include <tbb/tbb.h>
#include <tbb/parallel_reduce.h>

template<typename T, typename C>
class Body {
	T accumul_;
	const T* const in_;
	T* const out_;
	const T identity_;
	const C combine_;

public:
	Body(T* out, const T* in, T i, C c) :
		accumul_(i), identity_(i),
		in_(in), out_(out),
		combine_(c) {}

	Body(Body& src, tbb::split) : 
		accumul_(src.identity_), identity_(src.identity_),
		in_(src.in_), out_(src.out_), 
		combine_(src.combine_) {}

	template<typename Tag>
	void operator() (const tbb::blocked_range<T>& r, Tag) {
		T temp = accumul_;
		for (auto i = r.begin(); i < r.end(); i++) {
			temp = combine_(temp, in_[i]);
			if (Tag::is_final_scan())
				out_[i] = temp;
		}
		accumul_ = temp;
	}

	T get_accumul() { 
		return accumul_; 
	}

	void reverse_join(Body& src) {
		accumul_ = combine_(accumul_, src.accumul_);
	} 

	void assign(Body& src) {
		accumul_ = src.accumul_;
	}
};