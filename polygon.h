#ifndef polygon_h
#define polygon_h

#include <vector>
#include <string>
using namespace std;

#ifdef _MSC_VER
typedef __int64 int64_t;
#else
#include <inttypes.h>
#endif

struct rect
{
	int xmin, ymin;
	int xmax, ymax;
};

struct vertex
{
	int x;
	int y;
	vertex (int _x, int _y)
	{
		x = _x;
		y = _y;
	}
};

bool intersect (const vertex &p1, const vertex &p2, const vertex &q1, const vertex &q2)
{
	int64_t d = (q2.y - q1.y) * (p2.x - p1.x) - (q2.x - q1.x) * (p2.y - p1.y);
	if (d == 0)
		return false;

	int64_t _ua = (q2.x - q1.x) * (p1.y - q1.y) - (q2.y - q1.y) * (p1.x - q1.x);
	int64_t _ub = (p2.x - p1.x) * (p1.y - q1.y) - (p2.y - p1.y) * (p1.x - q1.x);

	long double ua = (long double)_ua / (long double)d;
	long double ub = (long double)_ub / (long double)d;

	if (((_ua == 0 || _ua == d) && (ub >= 0 && ub <= 1)) || ((_ub == 0 || _ub == d) && (ua >= 0 && ua <= 1)))
		return false;

	if ((ua > 0) && (ua < 1) && (ub > 0) && (ub < 1))
		return true;

	return false;
}

class polygon
{
public:
	vector<vertex> vertices;
	polygon() {}
	polygon (const polygon &copy)
	{
		for (int i = 0; i < copy.vertices.size(); i++)
			add(copy.vertices[i].x, copy.vertices[i].y);
	}
	void add (const int x, const int y)
	{
		vertices.push_back(vertex(x,y));
	}
	void finish ()
	{
		vertices.push_back(vertices[0]);
	}

	bool isInside (const vertex &v) const
	{
		int winding_number = 0;
		// distant point at a slight angle
		const vertex inf(v.x + 100000, v.y + 100);

		for (int i = 1; i < vertices.size(); i++)
		{
			const vertex &q1 = vertices[i-1];
			const vertex &q2 = vertices[i];
			if (intersect(v, inf, q1, q2))
				winding_number++;
		}
		return (winding_number & 1);
	}

	bool overlaps (const polygon &other) const
	{
		for (int i = 1; i < other.vertices.size(); i++)
			if (isInside(other.vertices[i]))
				return true;

		for (int i = 1; i < vertices.size(); i++)
		{
			const vertex &p1 = vertices[i-1];
			const vertex &p2 = vertices[i];
			for (int j = 1; j < other.vertices.size(); j++)
			{
				const vertex &q1 = other.vertices[j-1];
				const vertex &q2 = other.vertices[j];
				if (intersect(p1, p2, q1, q2))
					return true;
			}
		}
		return false;
	}

	void move (const int x, const int y)
	{
		for (int i = 0; i < vertices.size(); i++)
		{
			vertices[i].x += x;
			vertices[i].y += y;
		}
	}

	void bRect (rect &bbox) const
	{
		bbox.xmin = INT_MAX;	bbox.xmax = INT_MIN;
		bbox.ymin = INT_MAX;	bbox.ymax = INT_MIN;
		for (int i = 1; i < vertices.size(); i++)
		{
			bbox.xmin = min(bbox.xmin, vertices[i].x);
			bbox.ymin = min(bbox.ymin, vertices[i].y);
			bbox.xmax = max(bbox.xmax, vertices[i].x);
			bbox.ymax = max(bbox.ymax, vertices[i].y);
		}
	}
	string toString ()
	{
		string output;
		char buf[256];
		sprintf(buf, "%i,%i", vertices[1].x, vertices[1].y);
		output += buf;
		for (int i = 2; i < vertices.size(); i++)
		{
			sprintf(buf, ",%i,%i", vertices[i].x, vertices[i].y);
			output += buf;
		}
		return output;
	}
};

struct node
{
	int id;
	char pullup;
	int layer;
	polygon poly;
	rect bbox;
	node () {}
	bool collide (node *other)
	{
		if ((bbox.xmin > other->bbox.xmax) || (other->bbox.xmin > bbox.xmax) || (bbox.ymin > other->bbox.ymax) || (other->bbox.ymin > bbox.ymax))
			return false;
		return poly.overlaps(other->poly);
	}
};

struct transistor : public node
{
	int gate;
	int c1;
	int c2;
};

#endif // polygon_h
