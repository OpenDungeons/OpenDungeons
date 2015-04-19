#ifndef __RectLayoutManager_H__
#define __RectLayoutManager_H__

#include <list>
#include <algorithm>

// It moves the rectangles on the Y axis so they won't overlap.
class RectLayoutManager
{
public:

	enum MethodType
	{
		PLACE_ABOVE = 0,	// the overlapping rectangles are placed above the non-overlapping ones
		PLACE_BELOW			// the overlapping rectangles are placed below the non-overlapping ones
	};

	class Rect
	{
	public:
		Rect(short left, short top, short right, short bottom)
			: dy(0)
			, mBottom(bottom)
			, mTop(top)
			, mLeft(left)
			, mRight(right)
		{
			if (mBottom <= mTop)
				throw std::exception();

			if (mRight <= mLeft)
				throw std::exception();
		}

		inline const short getTop() const {return mTop + dy;}
		inline const short getBottom() const {return mBottom + dy;}
		inline const short getLeft() const {return mLeft;}
		inline const short getRight() const {return mRight;}

		// STL needs this
		inline bool operator <(const RectLayoutManager::Rect &R) const {return getBottom() < R.getBottom();}

		// displacement on Y axis
		short dy;

		// original rectangle coordinates
		short mBottom;
		short mTop;
		short mLeft;
		short mRight;
	};

	typedef std::list<RectLayoutManager::Rect> RectList;

	RectLayoutManager(	unsigned short leftBound,
						unsigned short topBound,
						unsigned short rightBound,
						unsigned short bottomBound,
						MethodType method = PLACE_ABOVE)
	: mMinDistance(1)
	, mMaxRectHeight(0)
	, mDepth(0)
	, mMethod(method)
	, mBoundTop(topBound)
	, mBoundLeft(leftBound)
	, mBoundBottom(bottomBound)
	, mBoundRight(rightBound)
	{
		if (mBoundBottom <= mBoundTop)
			throw std::exception();

		if (mBoundRight <= mBoundLeft)
			throw std::exception();
	}

	~RectLayoutManager(){clear();}

	const unsigned short getMinDistance() const {return mMinDistance;}
	const unsigned short getMaxRectHeight() const {return mMaxRectHeight;}
	const unsigned short getDepth() const {return mDepth;}
	void getBounds(	unsigned short &left,
					unsigned short &top,
					unsigned short &right,
					unsigned short &bottom)
	{
		left = mBoundLeft;
		top = mBoundTop;
		right = mBoundRight;
		bottom = mBoundBottom;
	}

	void setMinDistance(unsigned short minDistance){mMinDistance = minDistance;}
	void setDepth(unsigned short depth){mDepth = depth;}

	bool isOutOfBounds(RectLayoutManager::Rect &r)
	{
		if (r.getTop() < mBoundTop ||
			r.getBottom() > mBoundBottom ||
			r.getLeft() < mBoundLeft ||
			r.getRight() > mBoundRight)
			return true;
		else
			return false;
	}

	RectList::iterator getListBegin(){return mList.begin();}
	RectList::iterator getListEnd(){return mList.end();}

	void clear(){mList.clear();}

	RectList::iterator addData(Rect &Data)
	{
		if (isOutOfBounds(Data))
			return mList.end(); // out of bounds, error

		switch (mMethod)
		{
		case PLACE_ABOVE:
			return addDataAbove(Data);
		case PLACE_BELOW:
			return addDataBelow(Data);
		default:
			return mList.end(); // incorrect method type, error
		}
	}

protected:
	// List of orderedd rectangles
	// All items in list are assumed ordered and within established Bounds
	RectList mList;

	// The overlapping rectangles are placed at a mMinDistance from the other ones
	unsigned short mMinDistance;

	// This gets calculated "on the fly"
	unsigned short mMaxRectHeight;

	// An overlapping rectangle is moved on Y axis (Above or Below) until
	//  a place is found where it doesn't overlap any other rectangle.
	// mDepth is the number of times an overlapping rectangle will be moved
	//  in order to find a non-overlapping place.
	//
	// (mDepth = 0) - the search will go on until a place is found.
	// (mDepth > 0) - the search will go on <mDepth> times
	unsigned short mDepth;

	// Don't use these directly, use addData instead
	RectList::iterator addDataAbove(Rect &Data);
	RectList::iterator addDataBelow(Rect &Data);

	// Const variables can only be set in the constructor and certify the RectList integrity.

	// Method Type
	const MethodType mMethod;

	// Bounds
	const unsigned short mBoundTop;
	const unsigned short mBoundLeft;
	const unsigned short mBoundBottom;
	const unsigned short mBoundRight;
};

static bool _fLessBottom(const RectLayoutManager::Rect &L, const RectLayoutManager::Rect &R) {return L.getBottom() < R.getBottom();}

RectLayoutManager::RectList::iterator RectLayoutManager::addDataBelow(RectLayoutManager::Rect &Data)
{
	bool MoveIt = false;
	bool FoundIt = false;
	RectList::iterator itStart;
	RectList::iterator itLastChecked;
	RectList::iterator itCurrent;
	RectList::iterator itInsert;

	unsigned short depth = 0;
	RectLayoutManager::Rect &r = Data;
	short height = r.getBottom() - r.getTop();

	if (height > mMaxRectHeight)
		mMaxRectHeight = height;

	// find the first RECT  that has .bottom > r.top
	// first possible intersect
	r.dy -= height;
	itStart = lower_bound(mList.begin(), mList.end(), r, &_fLessBottom);
	r.dy += height;

	// it's safe to add it at the back of the list
	if (itStart == mList.end())
	{
		mList.push_back(r);
		return --(mList.end());
	}

	// insert it temporarily (we will move it at the right place, afterwords)
	itInsert = mList.insert(itStart,r);

	for (itCurrent = itStart, itLastChecked = itInsert;itCurrent != mList.end();itCurrent++)
	{
		// Can't intersect r so i skip it
		if ((*itCurrent).getRight() < r.getLeft())
			continue;

		// Can't intersect r so i skip it
		if (r.getRight() < (*itCurrent).getLeft())
			continue;

		// Can't intersect r so i skip it
		if (r.getTop() > (*itCurrent).getBottom())
			continue;

		short diff = (*itCurrent).getTop() - (*itLastChecked).getBottom();
		short diff2 = mMaxRectHeight - ((*itCurrent).getBottom() - (*itCurrent).getTop());
		if (diff > 0) // above the last checked
		{
			// If no rect overlapped r, then there is no need to move it
			if (!MoveIt && (diff > diff2))
			{
				FoundIt = true;
				itLastChecked = itStart;
				break;
			}
			else
				MoveIt = true;

			if (mDepth && (depth >= mDepth))
				break;

			// This is above r, so i check if its enought space to move r
			if (diff > height + diff2 + 2*mMinDistance)
			{
				r.dy = ((*itLastChecked).getBottom() + mMinDistance + 1) - r.getTop();
				FoundIt = true;
				break;
			}

			depth++;
		}
		else // it overlaps
			MoveIt = true;

		itLastChecked = itCurrent;
	}

	if (itCurrent == mList.end())
	{
		if (MoveIt)
			r.dy = ((*itLastChecked).getBottom() + mMinDistance + 1) - r.getTop();
		else
			itLastChecked = itStart;

		FoundIt = true;
	}

	mList.erase(itInsert);

	if (FoundIt)
	{
		if (r.getBottom() > mBoundBottom)
			return mList.end(); // out of bounds

		itInsert = lower_bound(itLastChecked, itCurrent, r);
		itInsert = mList.insert(itInsert,r);

		return itInsert;
	};

	return mList.end();
}

static bool _fGreaterTop(const RectLayoutManager::Rect &L, const RectLayoutManager::Rect &R) {return L.getTop() > R.getTop();}

RectLayoutManager::RectList::iterator RectLayoutManager::addDataAbove(RectLayoutManager::Rect &Data)
{
	bool MoveIt = false;
	bool FoundIt = false;
	RectList::iterator itStart;
	RectList::iterator itLastChecked;
	RectList::iterator itCurrent;
	RectList::iterator itInsert;

	unsigned short depth = 0;
	RectLayoutManager::Rect &r = Data;
	short height = r.getBottom() - r.getTop();

	if (height > mMaxRectHeight)
		mMaxRectHeight = height;

	// find the first RECT  that has .bottom > r.top
	// first possible intersect
	r.dy += height;
	itStart = lower_bound(mList.begin(), mList.end(), r, &_fGreaterTop);
	r.dy -= height;

	// it's safe to add it at the back of the list
	if (itStart == mList.end())
	{
		mList.push_back(r);
		return --(mList.end());
	}

	// insert it temporarily (we will move it at the right place, afterwords)
	itInsert = mList.insert(itStart,r);

	for (itCurrent = itStart, itLastChecked = itInsert;itCurrent != mList.end();itCurrent++)
	{
		// Can't intersect r so i skip it
		if ((*itCurrent).getRight() < r.getLeft())
			continue;

		// Can't intersect r so i skip it
		if (r.getRight() < (*itCurrent).getLeft())
			continue;

		// Can't intersect r so i skip it
		if (r.getBottom() < (*itCurrent).getTop())
			continue;

		short diff = (*itLastChecked).getTop() - (*itCurrent).getBottom();
		short diff2 = mMaxRectHeight - ((*itCurrent).getBottom() - (*itCurrent).getTop());
		if (diff > 0) // above the last checked
		{
			// If no rect overlapped r, then there is no need to move it
			if (!MoveIt && (diff > diff2))
			{
				FoundIt = true;
				itLastChecked = itStart;
				break;
			}
			else
				MoveIt = true;

			if (mDepth && (depth >= mDepth))
				break;

			// This is above r, so i check if its enought space to move r
			if (diff > height + diff2 + 2*mMinDistance)
			{
				r.dy = -(r.getBottom() - ((*itLastChecked).getTop() - mMinDistance - 1));
				FoundIt = true;
				break;
			}

			depth++;
		}
		else // it overlaps
			MoveIt = true;

		itLastChecked = itCurrent;
	}

	if (itCurrent == mList.end())
	{
		if (MoveIt)
			r.dy = -(r.getBottom() - ((*itLastChecked).getTop() - mMinDistance - 1));
		else
			itLastChecked = itStart;


		FoundIt = true;
	}

	mList.erase(itInsert);

	if (FoundIt)
	{
		if (r.getTop() < mBoundTop)
			return mList.end(); // out of bounds

		itInsert = lower_bound(itLastChecked, itCurrent, r, _fGreaterTop);
		itInsert = mList.insert(itInsert,r);

		return itInsert;
	};

	return mList.end();
}
#endif /* __RectLayoutManager_H__ */
