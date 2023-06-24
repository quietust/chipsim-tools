/*
 * Netlist Generator - Library
 * Tracks shape and location of circuit segments and detects collisions
 *
 * Copyright (c) QMT Productions
 */

#ifndef POLYGON_H
#define POLYGON_H

// Height of the layer images, so we can flip them vertically
// (it was simpler to hardcode it than to figure it out dynamically)
// The 2A03 used a height of 6256 and scale 2
// Leave undefined in order to disable vertical flipping entirely
//#define	CHIP_HEIGHT	10000
#ifndef SCALE
#define	SCALE	1
#endif

#include <vector>
#include <string>
#include <algorithm>

#ifdef _MSC_VER
typedef __int64 int64_t;
#else
#include <inttypes.h>
#include <climits>
#include <math.h>
#endif

struct rect
{
	int xmin, ymin;
	int xmax, ymax;
};

struct vertex
{
	int x, y;
	vertex (int _x, int _y) : x(_x), y(_y) { }
	vertex () : x(0), y(0) { }
};

// Checks if two segments intersect
// Second segment is offset by (0.5,0.5) to ensure that the segments can never overlap
bool intersect (const vertex &p1, const vertex &p2, const vertex &q1, const vertex &q2)
{
	int64_t d = 2 * ((q2.y - q1.y) * (p2.x - p1.x) - (q2.x - q1.x) * (p2.y - p1.y));
	if (d == 0)
		return false;

	int64_t _ua = (q2.x - q1.x) * (2 * (p1.y - q1.y) - 1) - (q2.y - q1.y) * (2 * (p1.x - q1.x) - 1);
	int64_t _ub = (p2.x - p1.x) * (2 * (p1.y - q1.y) - 1) - (p2.y - p1.y) * (2 * (p1.x - q1.x) - 1);

	long double ua = (long double)_ua / (long double)d;
	long double ub = (long double)_ub / (long double)d;

	// the two segments overlap - we should probably issue a warning if this happens, since it'll mess things up
	if (((_ua == 0 || _ua == d) && (ub >= 0 && ub <= 1)) || ((_ub == 0 || _ub == d) && (ua >= 0 && ua <= 1)))
		return false;

	if ((ua > 0) && (ua < 1) && (ub > 0) && (ub < 1))
		return true;

	return false;
}

class polygon
{
protected:
	std::vector<vertex> vertices;
public:
	polygon() {}
	polygon (const polygon &copy)
	{
		for (int i = 0; i < copy.vertices.size(); i++)
			add(copy.vertices[i].x, copy.vertices[i].y);
	}
	// Add a vertex to the polygon
	void add (const int x, const int y)
	{
		vertices.push_back(vertex(x,y));
	}
	// Copy the first vertex to the end - makes it easier to iterate across them
	void finish ()
	{
		vertices.push_back(vertices[0]);
	}
	int numVertices () const
	{
		return vertices.size() - 1;
	}

	// Check if a particular point is located inside the polygon
	bool isInside (const vertex &q1) const
	{
		int winding_number = 0;
		// distant point at a slight angle
		const vertex q2(q1.x + 32768, q1.y + 128);

		for (int i = 0; i < numVertices(); i++)
		{
			const vertex &p1 = vertices[i];
			const vertex &p2 = vertices[i + 1];
			if (intersect(p1, p2, q1, q2))
				winding_number++;
		}
		return (winding_number & 1);
	}

	// Check if the second polygon intersects with the first one
	// The second polygon should always be the smaller one
	bool overlaps (const polygon &other) const
	{
		// first, check if any of the target polygon's vertices are inside me
		for (int i = 0; i < other.numVertices(); i++)
			if (isInside(other.vertices[i]))
				return true;

		// if not, then see if any of its segments intersect with any of mine
		for (int i = 0; i < numVertices(); i++)
		{
			const vertex &p1 = vertices[i];
			const vertex &p2 = vertices[i + 1];
			for (int j = 0; j < other.numVertices(); j++)
			{
				const vertex &q1 = other.vertices[j];
				const vertex &q2 = other.vertices[j + 1];
				if (intersect(p1, p2, q1, q2))
					return true;
			}
		}
		return false;
	}

	// Move the polygon
	void move (const int x, const int y)
	{
		// Using vertices.size() instead of numVertices()
		// because need to hit the duplicate vertex at the end
		for (int i = 0; i < vertices.size(); i++)
		{
			vertices[i].x += x;
			vertices[i].y += y;
		}
	}

	// Calculate the polygon's bounding box
	void bRect (rect &bbox) const
	{
		bbox.xmin = INT_MAX;	bbox.xmax = INT_MIN;
		bbox.ymin = INT_MAX;	bbox.ymax = INT_MIN;
		for (int i = 0; i < numVertices(); i++)
		{
			bbox.xmin = std::min(bbox.xmin, vertices[i].x);
			bbox.ymin = std::min(bbox.ymin, vertices[i].y);
			bbox.xmax = std::max(bbox.xmax, vertices[i].x);
			bbox.ymax = std::max(bbox.ymax, vertices[i].y);
		}
	}

	// Find a vertex on the segment's perpendicular bisector
	// Vertex is always "d" pixels from the outside edge of the polygon
	// Also return the length of the segment, just because it's useful
	int midpoint (int idx, vertex &out, int d = 2) const
	{
		const vertex &v1 = vertices[idx];
		const vertex &v2 = vertices[idx + 1];
		vertex o0, o1, o2;
		o0.x = o1.x = o2.x = (v1.x + v2.x) / 2;
		o0.y = o1.y = o2.y = (v1.y + v2.y) / 2;
		if (v1.x == v2.x)
		{
			// vertical
			o0.x += 1;
			o1.x += d;
			o2.x -= d;
		}
		else if (v1.y == v2.y)
		{
			// horizontal
			o0.y += 1;
			o1.y += d;
			o2.y -= d;
		}
		else if (((v2.y - v1.y) / (double)(v2.x - v1.x)) > 0)
		{
			// positive slope
			o0.x -= 1; o0.y += 1;
			o1.x -= d; o1.y += d;
			o2.x += d; o2.y -= d;
		}
		else
		{
			// negative slope
			o0.x -= 1; o0.y -= 1;
			o1.x -= d; o1.y -= d;
			o2.x += d; o2.y += d;
		}
		if (isInside(o0))
			out = o2;
		else	out = o1;
		return sqrt((long double)((v2.y - v1.y) * (v2.y - v1.y) + (v2.x - v1.x) * (v2.x - v1.x)));
	}

	// Calculate the area of the polygon
	int area () const
	{
		int a = 0;
		for (int i = 0; i < numVertices(); i++)
		{
			const vertex &v1 = vertices[i];
			const vertex &v2 = vertices[i + 1];
			a += (v1.x * v2.y) - (v2.x * v1.y);
		}
		if (a < 0)
			a = -a;
		return a / 2;
	}

	// Generate a string containing a list of the polygon's vertex coordinates
	std::string toString () const
	{
		std::string output;
		char buf[48];
		sprintf(buf, "%i,%i", vertices[0].x, vertices[0].y);
		output += buf;
		for (int i = 1; i < numVertices(); i++)
		{
			sprintf(buf, ",%i,%i", vertices[i].x, vertices[i].y);
			output += buf;
		}
		return output;
	}
};

// Layer numbers, as used by ChipSim Visualizer
#define LAYER_METAL     0
#define LAYER_DIFF      1
#define LAYER_PROTECT   2
#define LAYER_DIFF_GND  3
#define LAYER_DIFF_PWR  4
#define LAYER_POLY      5
#define LAYER_SPECIAL   6

// Circuit node, corresponds to segdefs.js
struct node
{
	int id;
	char pull;
	int layer;
	polygon poly;
	rect bbox;
	node ()
	{
		id = 0;
		pull = '-';
		layer = -1;
	}
	bool collide (node *other)
	{
		// Do bounding box check before performing complicated polygon overlap check
		if ((bbox.xmin > other->bbox.xmax) || (other->bbox.xmin > bbox.xmax) || (bbox.ymin > other->bbox.ymax) || (other->bbox.ymin > bbox.ymax))
			return false;
		return poly.overlaps(other->poly);
	}
};

// Transistor definition, corresponds to transdefs.js
struct transistor : public node
{
	int gate;
	int c1;
	int c2;
	int width1;
	int width2;
	int length;
	int segments;
	int area;
	bool depl;
	bool ptype;
	transistor ()
	{
		gate = c1 = c2 = 0;
		width1 = width2 = 0;
		length = 0;
		segments = 0;
		area = 0;
		depl = ptype = false;
	}
};

// Read vertex list for a particular layer and generate node definitions
template<class T>
bool readnodes (const char *filename, std::vector<T *> &nodes, int layer, int force_id = -1)
{
	printf("Reading file: %s\n", filename);
	FILE *in = fopen(filename, "rt");
	if (!in)
	{
		fprintf(stderr, "Failed to open file '%s'!\n", filename);
		return false;
	}
	int x, y;
	int r;
	int line = 0;
	T *n = new T;
	while (1)
	{
		line++;
		r = fscanf(in, "%d,%d", &x, &y);
		if (feof(in))
			break;
		if (r != 2)
		{
			fprintf(stderr, "Error reading from file '%s' at line %i!\n", filename, line);
			delete n;
			return false;
		}
		if ((x == -1) && (y == -1))
		{
			n->poly.finish();
			n->layer = layer;
			if (force_id != -1)
				n->id = force_id;
			n->poly.bRect(n->bbox);
			nodes.push_back(n);
			n = new T;
		}
		else
		{
			// since the ChipSim canvas is upside-down
			// (0,0 is at bottom-left instead of top-left)
			// we flip the image vertically
#ifdef	CHIP_HEIGHT
			n->poly.add(x * SCALE, (CHIP_HEIGHT - y) * SCALE);
#else
			n->poly.add(x * SCALE, y * SCALE);
#endif
		}
	}
	delete n;
	return true;
}

#endif // POLYGON_H
