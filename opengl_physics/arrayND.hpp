#ifndef arrayND_h
#define arrayND_h

#include <vector>
#include <array>



namespace {
template<typename arrayT, typename innerT, unsigned int n, unsigned int newN>
struct subArrayBase {
	arrayT* theArr;
	size_t indices[n - newN];
	
	subArrayBase(arrayT* theArr): theArr(theArr) {};
};
template<typename arrayT, typename innerT, unsigned int n, unsigned int newN>
struct subArray : public subArrayBase<arrayT, innerT, n, newN> {
	using subArrayBase<arrayT, innerT, n, newN>::subArrayBase;
	
	typedef subArray<arrayT, innerT, n, newN-1> under;
	inline under operator[](size_t i) {
		under toReturn((this->theArr));
		
		for (int j = 0; j < n - newN; ++j) {
			toReturn.indices[j] = this->indices[j];
		}
		toReturn.indices[n - newN] = i;
		
		return toReturn;
	}
};
template<typename arrayT, typename innerT, unsigned int n>
struct subArray<arrayT, innerT, n, 1> : public subArrayBase<arrayT, innerT, n, 1> {
	using subArrayBase<arrayT, innerT, n, 1>::subArrayBase;
	
	typedef innerT under;
	
	inline typename arrayT::containerT::reference operator[](size_t i) {
		size_t accumulatedStride = 1;
		size_t idx = 0;
		for (int j = 0; j < n - 1; ++j) {
			idx += this->indices[j] * accumulatedStride;
			accumulatedStride *= this->theArr->sizes[j];
		}
		idx += i * accumulatedStride;
		return this->theArr->linear()[idx];
	}
};
}

template<typename innerT, unsigned int n>
class arrayND : public std::vector<innerT> {
	
public:
	typedef std::vector<innerT> containerT;
	typedef std::array<size_t, n> sizesT;
	sizesT sizes;
	explicit arrayND(sizesT sizes) : sizes(sizes) {
		containerT::resize(total());
		static_assert(sizes.size() == n, "you must provide n dimensions");
		static_assert(n >= 2, "must have at least 2 dimensions");
	};
	explicit arrayND(sizesT sizes, innerT initValue)
	: sizes(sizes) {
		containerT::resize(total(), initValue);
		static_assert(sizes.size() == n, "you must provide n dimensions");
		static_assert(n >= 2, "must have at least 2 dimensions");
	}
	
	typedef subArray<arrayND<innerT, n>, innerT, n, n-1> under;
	inline under operator[](size_t i) {
		
		under toReturn(this);
		toReturn.indices[0] = i;
		return toReturn;
	}
	size_t total() {
		size_t total = 1;
		for (int i = 0; i < n; ++i) {
			total *= sizes[i];
		}
		return total;
	}
	containerT& linear() {
		return *this;
	}
	sizesT ind2coord(size_t i) {
		sizesT coords;
		for (int j = 0; j < n; ++j) {
			coords[j] = i % this->sizes[j];
			i /= this->sizes[j];
		}
		return coords;
	}
	size_t coord2ind(sizesT coords) {
		size_t accumulatedStride = 1;
		size_t idx = 0;
		for (int j = 0; j < n; ++j) {
			if (coords[j] >= sizes[j]) throw std::out_of_range("arrayND coord2ind");
			idx += coords[j] * accumulatedStride;
			accumulatedStride *= this->sizes[j];
		}
		return idx;
	}
	inline typename containerT::reference operator[](sizesT coord) {
		return linear()[coord2ind(coord)];
	}
	bool inBounds(sizesT coord) {
		for (int i = 0; i < n; ++i) {
			if (coord[i] >= sizes[i]) return false;
		}
		return true;
	}
	bool inBounds(ssize_t i) {
		return i >= 0 && i < total();
	}
};



#endif /* arrayND_h */
