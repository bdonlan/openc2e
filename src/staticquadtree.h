#ifndef STATICQUADTREE
#define STATICQUADTREE

#include <algorithm>
#include <boost/function.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/variant.hpp>
#include <boost/array.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>
#include <boost/typeof/typeof.hpp> // BOOST_AUTO_TPL
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/casts.hpp>

#ifdef _WIN32
#if defined(min) || defined(max)

// get our std::min and std::max back
#undef min
#undef max

#endif
#endif

#define QUADTREE_FANOUT 16

struct QuadRegion {
	unsigned int left, right;
	unsigned int top, bottom;

	QuadRegion(unsigned int l, unsigned int r, unsigned int t, unsigned int b)
		: left(l), right(r), top(t), bottom(b)
	{
		assert(l <= r && b >= t);
	}

	bool operator==(const QuadRegion &r) const {
		return left==r.left && right==r.right && top==r.top && bottom == r.bottom;
	}

	QuadRegion() : left(0), right(0), top(0), bottom(0) { }

	unsigned int width() const { return right - left; }
	unsigned int height() const { return bottom - top; }

	operator bool() const {
		return width() && height();
	}
	QuadRegion intersect(const QuadRegion &r) const {
		if (left > r.right || r.left > right
				|| top > r.bottom || r.top > bottom)
		{
			return QuadRegion();
		}
		return QuadRegion(
				std::max(left, r.left), std::min(right, r.right),
				std::max(top, r.top), std::min(bottom, r.bottom)
				);
	}
	bool inside(const QuadRegion &r) const {
		return r.left <= left && r.right >= right
			&& r.top <= top && r.bottom >= bottom;
	}
	bool contains(unsigned int x, unsigned int y) const {
		if (x > right || x < left)
			return false;
		if (y > bottom || y < top)
			return false;
		return true;
	}
	static QuadRegion minContainsBoth(const QuadRegion &r1, const QuadRegion &r2) {
		return QuadRegion(
				std::min(r1.left, r2.left),
				std::max(r1.right, r2.right),
				std::min(r1.top, r2.top),
				std::max(r1.bottom, r2.bottom)
				);
	}

	std::string to_s() const;
};

class StaticQuadTree_Impl {
	public:
		struct lookup_cb {
			virtual bool operator()(void *) const = 0;
		};
	protected:
		friend class SQT_searchVisitor;
		friend class SQT_splitVisitor;
		friend class SQT_insertVisitor;
		struct Division;
		struct NullValue { };
		struct QuadRegionPair {
			QuadRegion r;
			void *value;
			QuadRegionPair() : r(QuadRegion()), value(NULL) { }
			QuadRegionPair(const QuadRegion &rv, void *v) : r(rv), value(v) { }
		};
		struct SubDivision {
			unsigned int xdiv, ydiv;
			Division *subdiv[2][2];
		};
		struct TermDivision {
			boost::array<QuadRegionPair, QUADTREE_FANOUT> regions;
		};
		struct OverflowDivision {
			std::vector<QuadRegionPair> regions;
		};
		struct Division {
			boost::variant<
				struct TermDivision, // must be first
				struct SubDivision, 
				struct OverflowDivision
			> children;
			QuadRegion covered;

			Division(const QuadRegion &r) : covered(r) { }
		};

		boost::object_pool<Division> divisions;
		Division *root;

		static bool searchDiv(Division *d, unsigned int x, unsigned int y, const lookup_cb &it);

		void splitDiv(Division *d);
		void overflowDiv(Division *d, bool allowSplit);
		void placeRegion(Division *d, const QuadRegion &r, void *v, bool allowSplit);
	public:
		StaticQuadTree_Impl(const std::vector<std::pair<QuadRegion, void *> > &regions);

		void lookup(
				unsigned int x, unsigned int y,
				const lookup_cb &it
				) const;
};

template<class V>
class StaticQuadTree {
	protected:
		boost::scoped_ptr<StaticQuadTree_Impl> impl;
		boost::object_pool<V> value_pool;
		template<typename F>
		struct lookup_cb_imp : public StaticQuadTree_Impl::lookup_cb
		{
			F &callback;
			lookup_cb_imp(F &cb) : callback(cb) { }

			virtual bool operator()(void *v) const {
				return callback(*static_cast<V *>(v));
			}
		};
	public:
		StaticQuadTree(const std::vector<std::pair<QuadRegion, V> > &values) {
			std::vector<std::pair<QuadRegion, void *> > values_vp;
			values_vp.reserve(values.size());
			for(BOOST_AUTO_TPL(it, values.begin()); it != values.end(); it++) {
				V *vp = value_pool.construct(it->second);
				values_vp.push_back(std::pair<QuadRegion, void *>(it->first, (void *)vp));
			}
			impl.reset(new StaticQuadTree_Impl(values_vp));
		}

		template<typename F>
		void lookup(
				unsigned int x, unsigned int y,
				F &callback
			) const
		{
			lookup_cb_imp<F> cb(callback);

			impl->lookup(x, y, cb);
		}

		std::vector<V> lookup(unsigned int x, unsigned y) {
			using namespace boost::lambda;
			std::vector<V> tmp;
			lookup(x, y,
					(bind(&std::vector<V>::push_back, &tmp, _1), /* return */ false)
				  );
			return tmp;
		}
};

#endif
