// miscellaneous functions
//
// Copyright (C) Martin R. Jones 1995
//

#ifndef __MISC_H__
#define __MISC_H__

#include <string.h>
#include <qlist.h>

char *StrDup(const char *src);
char *StrUpperDup(const char *src);
const char *StrUpperStr(const char *haystack, const char *needle);


template<class T> class cHistory
{
public:
	cHistory(unsigned _maxLen)
	{
		maxLen = _maxLen;
		list.setAutoDelete(TRUE);
	}

	T *Back()	{	return list.prev(); }
	T *Forward()	{	return list.next(); }
	T *Current()	{	return list.current(); }

	int IsBack()	{	return (list.current() != list.getFirst()); }
	int IsForward()	{	return (list.current() != list.getLast()); }

	void Add( T *item )
	{
		T *stop = list.current();
		if (stop)
			while (list.last() != stop) list.removeLast();
		if (list.count() > maxLen) list.removeFirst();
		list.append( item );
	}

private:
	unsigned maxLen;
	QList<T> list;
};

#endif

