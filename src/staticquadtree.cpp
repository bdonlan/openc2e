#include "staticquadtree.h"
#include <iterator>
#include <boost/format.hpp>

#define MIN_DIVISION_SIZE 25

#define IMPORT_SQT_TYPES \
		typedef StaticQuadTree_Impl::QuadRegionPair QuadRegionPair; \
		typedef StaticQuadTree_Impl::TermDivision TermDivision; \
		typedef StaticQuadTree_Impl::SubDivision SubDivision; \
		typedef StaticQuadTree_Impl::OverflowDivision OverflowDivision; \
		typedef StaticQuadTree_Impl::Division Division; \
		typedef StaticQuadTree_Impl::lookup_cb lookup_cb \

using namespace boost::lambda;

std::string QuadRegion::to_s() const {
	return boost::str(
			boost::format("(%u, %u) - (%u, %u)")
			% left % top
			% right % bottom
			);
}

class SQT_searchVisitor : public boost::static_visitor<bool> {
	protected:
		IMPORT_SQT_TYPES;

		const lookup_cb *cb;
		unsigned int x, y;

	public:
		template<typename T>
			bool visit(const T &l) const {
				for (BOOST_AUTO_TPL(it, l.begin()); it != l.end(); it++) {
					if (!it->value)
						break;
					if (!it->r.contains(x, y))
						continue;
					if ((*cb)(it->value))
						return true;
				}
				return false;
			}

		SQT_searchVisitor(const lookup_cb &cb, unsigned int x, unsigned int y) {
			this->cb = &cb;
			this->x = x;
			this->y = y;
		}
		
		bool operator()(const TermDivision &div) const {
			return visit(div.regions);
		}

		bool operator()(const OverflowDivision &div) const {
			return visit(div.regions);
		}

		bool operator()(const SubDivision &div) const {
			int xi, yi;
			if (x < div.xdiv)
				xi = 0;
			else
				xi = 1;
			if (y < div.ydiv)
				yi = 0;
			else
				yi = 1;

			return StaticQuadTree_Impl::searchDiv(div.subdiv[xi][yi], x, y, *cb);
		}
};

bool StaticQuadTree_Impl::searchDiv(Division *d, unsigned int x, unsigned int y, const lookup_cb &cb)
{
	assert(d->covered.contains(x, y));

	bool rv = boost::apply_visitor(SQT_searchVisitor(cb, x, y), d->children);
	return rv;
}

class SQT_splitVisitor : public boost::static_visitor<void> {
	protected:
		IMPORT_SQT_TYPES;
		StaticQuadTree_Impl *qt;
		Division *newDiv;
	public:
		SQT_splitVisitor(StaticQuadTree_Impl *qt_, Division *newDiv_)
			: qt(qt_), newDiv(newDiv_)
	  		{ }

		void operator()(const SubDivision &) const {
			assert(!"Attempting to split an already split node (impossible; should've asserted sooner");
			abort();
		}

		template<typename T>
			void visit(const T &l) const {
				for (BOOST_AUTO_TPL(it, l.begin()); it != l.end(); it++) {
					if (!it->value)
						break;
					qt->placeRegion(newDiv, it->r, it->value, false);
				}
			}

		void operator()(const TermDivision &td) const {
			visit(td.regions);
		}

		void operator()(const OverflowDivision &od) const {
			visit(od.regions);
		}
};

void StaticQuadTree_Impl::splitDiv(Division *d) {
	if (d->covered.width() < MIN_DIVISION_SIZE
			|| d->covered.height() < MIN_DIVISION_SIZE)
	{
		// too small, go to a fallback
		overflowDiv(d, false);
		return;
	}
	assert(!boost::get<SubDivision>(&d->children));
	OverflowDivision *od = boost::get<OverflowDivision>(&d->children);
	if (od) {
		// We only try to split an overflow div occasionally, in case
		// we have a true overlap situation
		size_t sz = od->regions.size();
		// if sz isn't a power of two, skip
		if (sz & (sz - 1) != 0) {
			return;
		}
	}
	Division oldDiv = *d;

	d->children = SubDivision();
	SubDivision *sd = boost::get<SubDivision>(&d->children);
	assert(sd);
	// TODO: better division-placement algorithm
	sd->xdiv = (d->covered.left + d->covered.right) / 2;
	sd->ydiv = (d->covered.top + d->covered.bottom) / 2;

	for (int xi = 0; xi < 2; xi++) {
		for (int yi = 0; yi < 2; yi++) {
			QuadRegion r = d->covered;
			if (xi)
				r.left = sd->xdiv;
			else
				r.right = sd->xdiv;
			if (yi)
				r.top = sd->ydiv;
			else
				r.bottom = sd->ydiv;

			Division *subDiv = divisions.construct(r);
			sd->subdiv[xi][yi] = subDiv;
		}
	}
	boost::apply_visitor(SQT_splitVisitor(this, d), oldDiv.children);
	assert(d->covered == oldDiv.covered);
}

void StaticQuadTree_Impl::overflowDiv(Division *d, bool allowSplit) {
	if (allowSplit) {
		splitDiv(d);
		return;
	}
	TermDivision *td = boost::get<TermDivision>(&d->children);
	if (!td)
		return; // already splitted
	assert(td->regions[td->regions.size()-1].value);

	std::vector<QuadRegionPair> rpl;
	std::back_insert_iterator<std::vector<QuadRegionPair> > ii(rpl);
	std::copy(td->regions.begin(), td->regions.end(), ii);

	d->children = OverflowDivision();
	boost::get<OverflowDivision>(d->children).regions.swap(rpl);		
}

class SQT_insertVisitor : public boost::static_visitor<bool> {
	protected:
		IMPORT_SQT_TYPES;
		StaticQuadTree_Impl *sqt;
		QuadRegion r;
		void *v;
		bool allowSplit;
	public:
		SQT_insertVisitor(StaticQuadTree_Impl *sqt_,
				const QuadRegion &r_, void *v_, bool allowSplit_)
			: sqt(sqt_), r(r_), v(v_), allowSplit(allowSplit_) { }

		bool operator()(TermDivision &td) const {
			for (size_t i = 0; i < td.regions.size(); i++) {
				if (td.regions[i].value)
					continue;
				td.regions[i].r = r;
				td.regions[i].value = v;
				return false;
			}
			return true;
		}

		bool operator()(OverflowDivision &od) const {
			if (allowSplit)
				return true;
			od.regions.push_back(QuadRegionPair(r, v));
			return false;
		}

		bool operator()(SubDivision &sd) const {
			for (int xi = 0; xi < 2; xi++) {
				for (int yi = 0; yi < 2; yi++) {
					QuadRegion subr = r.intersect(sd.subdiv[xi][yi]->covered);
					if (subr)
						sqt->placeRegion(sd.subdiv[xi][yi], subr, v, allowSplit);
				}
			}
			return false;
		}
};

void StaticQuadTree_Impl::placeRegion(Division *d, const QuadRegion &r,
		void *v, bool allowSplit)
{
	if (!v)
		return;
	
	bool overflowAndRetry = boost::apply_visitor(
			SQT_insertVisitor(this, r, v, allowSplit), d->children
			);
	if (overflowAndRetry) {
		overflowDiv(d, allowSplit);
		placeRegion(d, r, v, false);
	}
}

void StaticQuadTree_Impl::lookup(
		unsigned int x, unsigned int y,
		const lookup_cb &callback
		) const
{
	if (!root->covered.contains(x, y))
		return;
	searchDiv(root, x, y, callback);
}

StaticQuadTree_Impl::StaticQuadTree_Impl(
		const std::vector<std::pair<QuadRegion, void *> > &regions
		)
{
	typedef std::pair<QuadRegion, void *> VP;
	QuadRegion worldRegion;

	if (regions.size())
		worldRegion = regions[0].first;

	std::for_each(regions.begin(), regions.end(),
			( var(worldRegion) =
			  	bind(QuadRegion::minContainsBoth, var(worldRegion), bind(&VP::first, _1))
			));

	root = divisions.construct(worldRegion);

	std::for_each(regions.begin(), regions.end(),
			bind(&StaticQuadTree_Impl::placeRegion,
				this,
				root,
				bind(&VP::first, _1),
				bind(&VP::second, _1),
				true
				));
}

