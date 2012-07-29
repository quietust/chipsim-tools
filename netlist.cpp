/*
 * Netlist Generator
 * Originally designed for the Visual 2A03
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

#include <stdio.h>
#include "polygon.h"

#include <vector>
using namespace std;

// with the 2A03, enabling this causes some components to stop working
//#define DELETE_PULLUPS	// delete depletion-load pullups instead of just flagging them as "weak"

// don't output first/last lines of segdefs.js/transdefs.js
//#define OUTPUT_PARTIAL_JS

#ifndef FIRST_SEG_ID
#define	FIRST_SEG_ID	1
#endif
#ifndef FIRST_TRANS_ID
#define	FIRST_TRANS_ID	1
#endif

int main (int argc, char **argv)
{
	vector<node *> nodes;
	vector<transistor *> transistors;

	int metal_start, metal_end;
	int poly_start, poly_end;
	int diff_start, diff_end;
	readnodes<node>("metal_vcc.dat", nodes, LAYER_METAL);
	if (nodes.size() != 1)
	{
		fprintf(stderr, "Error: metal_vcc.dat contains more than one node!\n");
		return 1;
	}
	readnodes<node>("metal_gnd.dat", nodes, LAYER_METAL);
	if (nodes.size() != 2)
	{
		fprintf(stderr, "Error: metal_gnd.dat contains more than one node!\n");
		return 1;
	}
	// metal_start begins after VCC/GND, since those are handled separately
	metal_start = nodes.size();
	readnodes<node>("metal.dat", nodes, LAYER_METAL);
	metal_end = poly_start = nodes.size();
	readnodes<node>("polysilicon.dat", nodes, LAYER_POLY);
	poly_end = diff_start = nodes.size();
	readnodes<node>("diffusion.dat", nodes, LAYER_DIFF);
	diff_end = nodes.size();

	int nextNode = FIRST_SEG_ID;
	int vcc = nextNode++;
	int gnd = nextNode++;

	vector<node *> vias, matched, unmatched;
	node *via, *cur, *sub;
	int i, j, k;

	readnodes<node>("vias.dat", vias, LAYER_SPECIAL);

	// VCC and GND are the two biggest nodes, accounting for most of the vias
	// so report progress on them before proceeding to the rest of the chip
	printf("Parsing VCC node - %i vias remaining\n", vias.size());
	{
		cur = nodes[0];
		cur->id = vcc;
		for (i = 0; i < vias.size(); i++)
		{
			via = vias[i];
			if (cur->collide(via))
				matched.push_back(via);
			else	unmatched.push_back(via);
		}
		while (!matched.empty())
		{
//			printf("%i    \r", matched.size());
			via = matched.back();
			matched.pop_back();
			for (i = poly_start; i < diff_end; i++)
			{
				sub = nodes[i];
				if (sub->collide(via))
				{
					delete via;
					via = NULL;
					sub->id = vcc;
					break;
				}
			}
			if (via)
				delete via; //unmatched.push_back(via);
		}
		vias = unmatched;
		unmatched.clear();
	}

	printf("Parsing GND node - %i vias remaining\n", vias.size());
	{
		cur = nodes[1];
		cur->id = gnd;
		for (i = 0; i < vias.size(); i++)
		{
			via = vias[i];
			if (cur->collide(via))
				matched.push_back(via);
			else	unmatched.push_back(via);
		}
		while (!matched.empty())
		{
//			printf("%i    \r", matched.size());
			via = matched.back();
			matched.pop_back();
			for (i = poly_start; i < diff_end; i++)
			{
				sub = nodes[i];
				if (sub->collide(via))
				{
					delete via;
					via = NULL;
					if (sub->id == vcc)
					{
						fprintf(stderr, "Error - node %i shorts VCC to GND!\n", i);
						return 2;
					}
					sub->id = gnd;
					break;
				}
			}
			if (via)
				delete via; //unmatched.push_back(via);
		}
		vias = unmatched;
		unmatched.clear();
	}

	printf("Parsing metal nodes %i thru %i - %i vias remaining\n", metal_start, metal_end - 1, vias.size());
	for (i = metal_start; i < metal_end; i++)
	{
//		printf("%i        \r", i);
		cur = nodes[i];
		if (!cur->id)
			cur->id = nextNode++;
		for (j = 0; j < vias.size(); j++)
		{
			via = vias[j];
			if (cur->collide(via))
				matched.push_back(via);
			else	unmatched.push_back(via);
		}
		while (!matched.empty())
		{
//			printf("%i.%i\r", i, matched.size());
			via = matched.back();
			matched.pop_back();
			for (j = poly_start; j < diff_end; j++)
			{
				sub = nodes[j];
				if (sub->collide(via))
				{
					delete via;
					via = NULL;
					if (sub->id == 0)
						sub->id = cur->id;
					else if (sub->id != cur->id)
					{
						int oldid = cur->id;
						int newid = sub->id;
						for (k = 0; k < nodes.size(); k++)
							if (nodes[k]->id == oldid)
								nodes[k]->id = newid;
					}
					break;
				}
			}
			if (via)
				delete via; //unmatched.push_back(via);
		}
		vias = unmatched;
		unmatched.clear();
	}
	if (!vias.empty())
	{
		printf("%i vias were not matched!\n", vias.size());
		while (!vias.empty())
		{
			via = vias.back();
			vias.pop_back();
			fprintf(stderr, "Via (%s) was not hit!\n", via->poly.toString().c_str());
			delete via;
		}
	}

	readnodes<node>("buried_contacts.dat", vias, LAYER_SPECIAL);

	printf("Parsing polysilicon nodes %i thru %i - %i buried contacts remaining\n", poly_start, poly_end - 1, vias.size());
	for (i = poly_start; i < poly_end; i++)
	{
//		printf("%i        \r", i);
		cur = nodes[i];
		if (!cur->id)
			cur->id = nextNode++;
		for (j = 0; j < vias.size(); j++)
		{
			if (cur->collide(vias[j]))
				matched.push_back(vias[j]);
			else	unmatched.push_back(vias[j]);
		}
		while (!matched.empty())
		{
//			printf("%i.%i\r", i, matched.size());
			via = matched.back();
			matched.pop_back();
			for (j = diff_start; j < diff_end; j++)
			{
				sub = nodes[j];
				if (sub->collide(via))
				{
					delete via;
					via = NULL;
					if (sub->id == 0)
						sub->id = cur->id;
					else if (sub->id != cur->id)
					{
						int oldid = cur->id;
						int newid = sub->id;
						for (k = 0; k < nodes.size(); k++)
							if (nodes[k]->id == oldid)
								nodes[k]->id = newid;
					}
					break;
				}
			}
			if (via)
				delete via; //unmatched.push_back(via);
		}
		vias = unmatched;
		unmatched.clear();
		// Not strictly correct, but it handles the input protection diodes
		if (cur->id == gnd)
			cur->layer = LAYER_PROTECT;
	}
	if (!vias.empty())
	{
		printf("%i buried contacts were not matched!\n", vias.size());
		while (!vias.empty())
		{
			via = vias.back();
			vias.pop_back();
			fprintf(stderr, "Buried contact (%s) was not hit!\n", via->poly.toString().c_str());
			delete via;
		}
	}

	printf("Parsing diffusion nodes %i thru %i\n", diff_start, diff_end - 1);
	for (i = diff_start; i < diff_end; i++)
	{
		cur = nodes[i];
		if (!cur->id)
			cur->id = nextNode++;
		if (cur->id == vcc)
			cur->layer = LAYER_DIFF_VCC;
		if (cur->id == gnd)
			cur->layer = LAYER_DIFF_GND;
	}

	// TODO - add an option to go through all of the nodes and make the ID numbers consecutive

	readnodes<transistor>("transistors.dat", transistors, LAYER_SPECIAL);
	transistor *cur_t;
	nextNode = FIRST_TRANS_ID;

	vector<node *> diffs;
	int pullups = 0;

	node *t1 = new node;
	node *t2 = new node;
	node *t3 = new node;
	node *t4 = new node;

	printf("Parsing %i transistors\n", transistors.size());
	for (i = 0; i < transistors.size(); i++)
	{
//		printf("%i     \r", i);
		cur_t = transistors[i];
		cur_t->id = nextNode++;
		cur_t->depl = false;
		cur_t->width1 = 0;
		cur_t->width2 = 0;
		cur_t->length = 0;
		cur_t->segments = 0;
		cur_t->area = cur_t->poly.area();

		for (j = poly_start; j < poly_end; j++)
		{
			sub = nodes[j];
			if (sub->collide(cur_t))
			{
				cur_t->gate = sub->id;
				break;
			}
		}
		t1->poly = polygon(cur_t->poly);
		t2->poly = polygon(cur_t->poly);
		t3->poly = polygon(cur_t->poly);
		t4->poly = polygon(cur_t->poly);
		t1->poly.move(-3, 0);
		t2->poly.move(3, 0);
		t3->poly.move(0, -3);
		t4->poly.move(0, 3);
		t1->poly.bRect(t1->bbox);
		t2->poly.bRect(t2->bbox);
		t3->poly.bRect(t3->bbox);
		t4->poly.bRect(t4->bbox);
		diffs.clear();
		for (j = diff_start; j < diff_end; j++)
		{
			sub = nodes[j];
			if (sub->collide(t1) || sub->collide(t2) || sub->collide(t3) || sub->collide(t4))
				diffs.push_back(sub);
		}

		if (diffs.size() != 2)
		{
			// assign dummy values
			cur_t->c1 = gnd;
			cur_t->c2 = gnd;
			fprintf(stderr, "\rTransistor %i had wrong number of terminals (%i)\n", cur_t->id, diffs.size());
			continue;
		}

		// if any of the following are true, swap the terminals around
		// 1. 1st terminal is connected to VCC
		// 2. 1st terminal is connected to GND
		// 3. 2nd terminal is connected to gate (generally gets caught by case #1)
		if ((diffs[0]->id == vcc) || (diffs[0]->id == gnd) || (diffs[1]->id == cur_t->gate))
		{
			sub = diffs.front();
			diffs.erase(diffs.begin());
			diffs.push_back(sub);
		}

		cur_t->c1 = diffs[0]->id;
		cur_t->c2 = diffs[1]->id;

		// calculate geometry
		int segs0 = 0, segs1 = 0, segs2 = 0;
		for (j = 0; j < cur_t->poly.numVertices(); j++)
		{
			vertex v;
			int len = cur_t->poly.midpoint(j, v);
			if (diffs[0]->poly.isInside(v))
			{
				cur_t->width1 += len;
				segs1++;
			}
			else if (diffs[1]->poly.isInside(v))
			{
				cur_t->width2 += len;
				segs2++;
			}
			else
			{
				cur_t->length += len;
				segs0++;
			}
		}
		if (segs1 == segs2)
			cur_t->segments = segs1;
		else
		{
			fprintf(stderr, "\rTransistor %i had inconsistent number of segments on each side (%i/%i)!\n", cur_t->id, segs1, segs2);
			cur_t->segments = max(segs1, segs2);
		}
		if (segs0)
			cur_t->length /= segs0;
		else	fprintf(stderr, "\rTransistor %i has zero length?\n", cur_t->id);

		// if the gate is connected to one terminal and the other terminal is VCC/GND, assign pullup state to the other side
		if (cur_t->c1 == cur_t->gate)
		{
			// gate == c1, c2 == VCC -> it's a pull-up resistor
			// but don't do this if the other side is GND!
			if ((cur_t->c2 == vcc) && (cur_t->c1 != gnd))
			{
				// assign pull-up state
				for (j = metal_start; j < diff_end; j++)
				{
					if (nodes[j]->id == cur_t->gate)
						nodes[j]->pullup = '+';
				}
				cur_t->depl = true;
#ifdef	DELETE_PULLUPS
				// if we're not including depletion-mode transistors
				// then reuse the ID number
				nextNode--;
#endif
				pullups++;
			}
		}
	}
	diffs.clear();
	printf("%i transistors turned out to be pull-up resistors\n", pullups);

	delete t1;
	delete t2;
	delete t3;
	delete t4;

	FILE *out;

	printf("Writing transdefs.js\n");
	out = fopen("transdefs.js", "wt");
	if (!out)
	{
		fprintf(stderr, "Unable to create transdefs.js!\n");
		return 1;
	}
#ifndef	OUTPUT_PARTIAL_JS
	fprintf(out, "var transdefs = [\n");
#endif
	for (i = 0; i < transistors.size(); i++)
	{
		cur_t = transistors[i];
#ifdef	DELETE_PULLUPS
		if (cur_t->depl)
		{
			delete cur_t;
			continue;
		}
#endif
		fprintf(out, "['t%i',%i,%i,%i,[%i,%i,%i,%i],[%i,%i,%i,%i,%i],%s],\n", cur_t->id, cur_t->gate, cur_t->c1, cur_t->c2, cur_t->bbox.xmin, cur_t->bbox.xmax, cur_t->bbox.ymin, cur_t->bbox.ymax, cur_t->width1, cur_t->width2, cur_t->length, cur_t->segments, cur_t->area, cur_t->depl ? "true" : "false");
		delete cur_t;
	}
#ifndef	OUTPUT_PARTIAL_JS
	fprintf(out, "]\n");
#endif
	fclose(out);
	transistors.clear();

	printf("Writing segdefs.js\n");
	out = fopen("segdefs.js", "wt");
	if (!out)
	{
		fprintf(stderr, "Unable to create segdefs.js!\n");
		return 1;
	}
#ifndef	OUTPUT_PARTIAL_JS
	fprintf(out, "var segdefs = [\n");
#endif
	// skip the main VCC and GND nodes, since those are huge and we don't really need them
	for (i = 2; i < nodes.size(); i++)
	{
		cur = nodes[i];
		fprintf(out, "[%i,'%c',%i,%s],\n", cur->id, cur->pullup, cur->layer, cur->poly.toString().c_str());
		delete cur;
	}
#ifndef	OUTPUT_PARTIAL_JS
	fprintf(out, "]\n");
#endif
	fclose(out);
	nodes.clear();

	printf("All done!\n");
	return 0;
}
