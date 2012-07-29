/*
 * Netlist Generator - Library
 * Tracks shape and location of circuit segments and detects collisions
 *
 * Copyright (c) 2011 QMT Productions
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef POLYGON_H
#define POLYGON_H

// Height of the layer images, so we can flip them vertically
// (it was simpler to hardcode it than to figure it out dynamically)
// This was the value used for the 2A03
#define	CHIP_HEIGHT	6256

#include <vector>
#include <string>
using namespace std;
#include <math.h>

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
	int x, y;
	vertex (int _x, int _y) : x(_x), y(_y) { }
	vertex () : x(0), y(0) { }
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
protected:
	vector<vertex> vertices;
public:
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
	int numVertices ()
	{
		return vertices.size() - 1;
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
		// first, check if any of the target polygon's vertices are inside me
		for (int i = 1; i < other.vertices.size(); i++)
			if (isInside(other.vertices[i]))
				return true;

		// if not, then see if any of its vertices intersect with any of mine
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

	// Get a pair of vertices on either side of a segment
	int midpoint (int idx, vertex &out, int d = 3)
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
		return sqrt((v2.y - v1.y) * (v2.y - v1.y) + (v2.x - v1.x) * (v2.x - v1.x));
	}

	int area ()
	{
		int a = 0;
		for (int i = 1; i < vertices.size(); i++)
		{
			const vertex &v1 = vertices[i - 1];
			const vertex &v2 = vertices[i];
			a += (v1.x * v2.y) - (v2.x * v1.y);
		}
		if (a < 0)
			a = -a;
		return a / 2;
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
	int width1;
	int width2;
	int length;
	int segments;
	int area;
	bool depl;
};

#define LAYER_METAL     0
#define LAYER_DIFF      1
#define LAYER_PROTECT   2
#define LAYER_DIFF_GND  3
#define LAYER_DIFF_VCC  4
#define LAYER_POLY      5
#define LAYER_SPECIAL   6

template<class T>
void readnodes (const char *filename, vector<T *> &nodes, int layer)
{
	printf("Reading file: %s\n", filename);
	FILE *in = fopen(filename, "rt");
	if (!in)
	{
		fprintf(stderr, "Failed to open file!\n");
		exit(2);
	}
	int x, y;
	int r;
	T *n = new T;
	while (1)
	{
		r = fscanf(in, "%d,%d", &x, &y);
		if (feof(in))
			break;
		if (r != 2)
		{
			fprintf(stderr, "Error reading file!\n");
			exit(2);
		}
		if ((x == -1) && (y == -1))
		{
			n->poly.finish();
			n->id = 0;
			n->pullup = '-';
			n->layer = layer;
			n->poly.bRect(n->bbox);
			nodes.push_back(n);
			n = new T;
		}
		else
		{
			// all coordinates are doubled, and "special" layers
			// (vias, buried contacts, and transistors) are offset
			// by a pixel so that the intersection checks
			// don't get confused if 2 segments coincide

			// since the ChipSim canvas is upside-down
			// (0,0 is at bottom-left instead of top-left)
			// we flip the image vertically
			if (layer == LAYER_SPECIAL)
				n->poly.add(x * 2 + 1, (CHIP_HEIGHT - y) * 2 + 1);
			else	n->poly.add(x * 2, (CHIP_HEIGHT - y) * 2);
		}
	}
	delete n;
}

#endif // POLYGON_H
