/*
 * Netlist Generator
 * Originally designed for the Visual 2A03
 *
 * Copyright 2011-2022 QMT Productions
 */

#include <stdio.h>
#include "polygon.h"

// Uncomment this to enable detection and removal of depletion pullups
//#define NMOS

#include <vector>
using std::vector;

// don't output first/last lines of segdefs.js/transdefs.js
//#define OUTPUT_PARTIAL_JS

#ifndef FIRST_SEG_ID
#define	FIRST_SEG_ID	1
#endif
#ifndef FIRST_TRANS_ID
#define	FIRST_TRANS_ID	1
#endif

bool find_hits (vector<node *> &nodes, vector<node *> &vias, int &nextNode, int pwr, int gnd, size_t outer_start, size_t outer_end, size_t inner_start, size_t inner_end)
{
	vector<node *> matched, unmatched;
	node *via, *cur, *sub;

	for (size_t i = outer_start; i < outer_end; i++)
	{
		cur = nodes[i];
		if (!cur->id)
			cur->id = nextNode++;
		for (size_t j = 0; j < vias.size(); j++)
		{
			via = vias[j];
			if (cur->collide(via))
				matched.push_back(via);
			else	unmatched.push_back(via);
		}
		while (!matched.empty())
		{
			via = matched.back();
			matched.pop_back();
			for (size_t j = inner_start; j < inner_end; j++)
			{
				sub = nodes[j];
				if (!sub->collide(via))
					continue;
				if (sub->id == 0)
					sub->id = cur->id;
				else if (sub->id != cur->id)
				{
					// merge the two nodes together, assuming the lower ID number of the two
					int oldid = std::max(cur->id, sub->id);
					int newid = std::min(cur->id, sub->id);
					if ((oldid == gnd) && (newid == pwr))
					{
						fprintf(stderr, "Error - via %zi (%s) shorts PWR to GND!\n", i, via->poly.toString().c_str());
						return false;
					}
					for (size_t k = 0; k < nodes.size(); k++)
						if (nodes[k]->id == oldid)
							nodes[k]->id = newid;
				}
				break;
			}
			delete via;
		}
		vias = unmatched;
		unmatched.clear();
	}

	if (!vias.empty())
	{
		printf("%zi vias were not matched!\n", vias.size());
		while (!vias.empty())
		{
			via = vias.back();
			vias.pop_back();
			fprintf(stderr, "Via (%s) was not hit!\n", via->poly.toString().c_str());
			delete via;
		}
	}
	return true;
}

int main (int argc, char **argv)
{
	vector<node *> nodes, vias;
	vector<transistor *> transistors;
	node *cur, *sub;

	int nextNode = FIRST_SEG_ID;
	// Note: PWR must be less than GND, or other parts of this tool will fail
	int pwr = nextNode++;
	int gnd = nextNode++;

	size_t metal2_start, metal2_end;
	size_t metal1_start, metal1_end;
	size_t poly_start, poly_end;
	size_t diff_start, diff_end;

	metal2_start = nodes.size();
	readnodes<node>("metal2_pwr.dat", nodes, LAYER_METAL, pwr);
	readnodes<node>("metal2_gnd.dat", nodes, LAYER_METAL, gnd);
	readnodes<node>("metal2.dat", nodes, LAYER_METAL);
	metal2_end = nodes.size();

	metal1_start = nodes.size();
	readnodes<node>("metal1_pwr.dat", nodes, LAYER_METAL, pwr);
	readnodes<node>("metal1_gnd.dat", nodes, LAYER_METAL, gnd);
	readnodes<node>("metal1.dat", nodes, LAYER_METAL);
	// Legacy support for NMOS chips
	readnodes<node>("metal_pwr.dat", nodes, LAYER_METAL, pwr);
	readnodes<node>("metal_gnd.dat", nodes, LAYER_METAL, gnd);
	readnodes<node>("metal.dat", nodes, LAYER_METAL);
	metal1_end = nodes.size();

	poly_start = nodes.size();
	readnodes<node>("poly_pwr.dat", nodes, LAYER_POLY, pwr);
	readnodes<node>("poly_gnd.dat", nodes, LAYER_POLY, gnd);
	readnodes<node>("poly.dat", nodes, LAYER_POLY);
	poly_end = nodes.size();

	diff_start = nodes.size();
	readnodes<node>("diff_pwr.dat", nodes, LAYER_DIFF, pwr);
	readnodes<node>("diff_gnd.dat", nodes, LAYER_DIFF, gnd);
	readnodes<node>("diff.dat", nodes, LAYER_DIFF);
	diff_end = nodes.size();

	// First, use 'vias2' to link 'metal2' to 'metal1'
	readnodes<node>("vias2.dat", vias, LAYER_SPECIAL);
	printf("Parsing metal2 nodes %zi thru %zi with %zi vias\n", metal2_start, metal2_end - 1, vias.size());
	if (!find_hits(nodes, vias, nextNode, pwr, gnd, metal2_start, metal2_end, metal1_start, metal1_end))
		return 2;

	// Next, use 'vias1' to link 'metal1' to poly/diff
	readnodes<node>("vias1.dat", vias, LAYER_SPECIAL);
	// Legacy support for NMOS chips
	readnodes<node>("vias.dat", vias, LAYER_SPECIAL);

	printf("Parsing metal1 nodes %zi thru %zi with %zi vias\n", metal1_start, metal1_end - 1, vias.size());
	if (!find_hits(nodes, vias, nextNode, pwr, gnd, metal1_start, metal1_end, poly_start, diff_end))
		return 2;

	// If we have any buried contacts, scan them
	readnodes<node>("buried.dat", vias, LAYER_SPECIAL);

	printf("Parsing polysilicon nodes %zi thru %zi with %zi buried contacts\n", poly_start, poly_end - 1, vias.size());
	if (!find_hits(nodes, vias, nextNode, pwr, gnd, poly_start, poly_end, diff_start, diff_end))
		return 2;

	printf("Parsing diffusion nodes %zi thru %zi\n", diff_start, diff_end - 1);
	for (size_t i = diff_start; i < diff_end; i++)
	{
		cur = nodes[i];
		if (!cur->id)
			cur->id = nextNode++;
		if (cur->id == pwr)
			cur->layer = LAYER_DIFF_PWR;
		if (cur->id == gnd)
			cur->layer = LAYER_DIFF_GND;
	}

	// Move all permanently powered/grounded poly nodes into the Protect layer
	for (size_t i = poly_start; i < poly_end; i++)
	{
		cur = nodes[i];
		if ((cur->id == pwr) || (cur->id == gnd))
			cur->layer = LAYER_PROTECT;
	}

	// TODO - add an option to go through all of the nodes and make the ID numbers consecutive

	size_t trans_p_start;
	readnodes<transistor>("trans_n.dat", transistors, LAYER_SPECIAL);
	readnodes<transistor>("trans.dat", transistors, LAYER_SPECIAL);
	trans_p_start = transistors.size();
	readnodes<transistor>("trans_p.dat", transistors, LAYER_SPECIAL);

	transistor *cur_t;
	nextNode = FIRST_TRANS_ID;

	vector<node *> diffs;
#ifdef NMOS
	int pullups = 0;
#endif

	node *t1 = new node;
	node *t2 = new node;
	node *t3 = new node;
	node *t4 = new node;

	printf("Parsing %zi transistors\n", transistors.size());
	for (size_t i = 0; i < transistors.size(); i++)
	{
		cur_t = transistors[i];
		cur_t->id = nextNode++;
		cur_t->area = cur_t->poly.area();
		cur_t->ptype = (i >= trans_p_start);

		for (size_t j = poly_start; j < poly_end; j++)
		{
			sub = nodes[j];
			if (sub->collide(cur_t))
			{
				cur_t->gate = sub->id;
				// Permanently disabled transistors (grounded N or powered P) get discarded at the end
				if (cur_t->gate == (cur_t->ptype ? pwr : gnd))
					nextNode--;
				break;
			}
		}
		t1->poly = polygon(cur_t->poly);
		t2->poly = polygon(cur_t->poly);
		t3->poly = polygon(cur_t->poly);
		t4->poly = polygon(cur_t->poly);
		t1->poly.move(-2, 0);
		t2->poly.move(2, 0);
		t3->poly.move(0, -2);
		t4->poly.move(0, 2);
		t1->poly.bRect(t1->bbox);
		t2->poly.bRect(t2->bbox);
		t3->poly.bRect(t3->bbox);
		t4->poly.bRect(t4->bbox);

		diffs.clear();
		// Find all diff nodes which are touching the transistor
		for (size_t j = diff_start; j < diff_end; j++)
		{
			sub = nodes[j];
			if (sub->collide(t1) || sub->collide(t2) || sub->collide(t3) || sub->collide(t4))
				diffs.push_back(sub);
		}

		cur_t->c1 = cur_t->c2 = -1;
		// Loop through the nodes we found, and assign the first two unique ones to the source and drain
		// If we don't find exactly 2 unique nodes, then we've got a problem
		bool err = false;
		for (size_t j = 0; j < diffs.size(); j++)
		{
			if (cur_t->c1 == -1)
				cur_t->c1 = diffs[j]->id;
			if (cur_t->c1 == diffs[j]->id)
				continue;
			if (cur_t->c2 == -1)
				cur_t->c2 = diffs[j]->id;
			if (cur_t->c2 == diffs[j]->id)
				continue;
			err = true;
			break;
		}

		if (err || (cur_t->c1 == -1) || (cur_t->c2 == -1))
		{
			fprintf(stderr, "Transistor %i (%s) had wrong number of terminals (%zi:%i,%i)\n", cur_t->id, cur_t->poly.toString().c_str(), diffs.size(), cur_t->c1, cur_t->c2);
			// assign dummy values
			cur_t->c1 = cur_t->c2 = cur_t->ptype ? pwr : gnd;
			continue;
		}

		// if any of the following are true, swap the terminals around
		// 1. 1st terminal is connected to PWR
		// 2. 1st terminal is connected to GND
		// 3. NMOS only: 2nd terminal is connected to gate (generally gets caught by case #1)
		if ((cur_t->c1 == pwr) || (cur_t->c1 == gnd)
#ifdef NMOS
			 || (cur_t->c2 == cur_t->gate)
#endif
		)
		{
			int tmp = cur_t->c2;
			cur_t->c2 = cur_t->c1;
			cur_t->c1 = tmp;
		}

		// calculate geometry
		int segs0 = 0, segs1 = 0, segs2 = 0;
		for (size_t j = 0; j < cur_t->poly.numVertices(); j++)
		{
			vertex v;
			int len = cur_t->poly.midpoint(j, v);
			bool found = false;
			for (size_t k = 0; k < diffs.size(); k++)
			{
				if (!diffs[k]->poly.isInside(v))
					continue;
				if (diffs[k]->id == cur_t->c1)
				{
					cur_t->width1 += len;
					segs1++;
				}
				else if (diffs[k]->id == cur_t->c2)
				{
					cur_t->width2 += len;
					segs2++;
				}
				else	fprintf(stderr, "Transistor %i (%s) diffusion node %i doesn't belong to either side (%i/%i)?\n", cur_t->id, cur_t->poly.toString().c_str(), diffs[k]->id, cur_t->c1, cur_t->c2);
				found = true;
				break;
			}
			if (!found)
			{
				cur_t->length += len;
				segs0++;
			}
		}
		if (segs1 == segs2)
			cur_t->segments = segs1;
		else
		{
			fprintf(stderr, "Transistor %i (%s) had inconsistent number of segments on each side (%i/%i)!\n", cur_t->id, cur_t->poly.toString().c_str(), segs1, segs2);
			cur_t->segments = std::max(segs1, segs2);
		}
		if (segs0)
			cur_t->length /= segs0;
		else	fprintf(stderr, "Transistor %i (%s) has zero length?\n", cur_t->id, cur_t->poly.toString().c_str());
		if (cur_t->c1 == pwr && cur_t->c2 == gnd)
			fprintf(stderr, "Transistor %i (%s) connects PWR to GND!\n", cur_t->id, cur_t->poly.toString().c_str());
#ifdef NMOS
		// if the gate is connected to one terminal and the other terminal is PWR/GND, assign pullup state to the other side
		if (cur_t->c1 == cur_t->gate)
		{
			// gate == c1, c2 == PWR -> it's a pull-up resistor
			// but don't do this if the other side is GND!
			if ((cur_t->c2 == pwr) && (cur_t->c1 != gnd))
			{
				// assign pull-up state
				for (size_t j = metal1_start; j < diff_end; j++)
				{
					if (nodes[j]->id == cur_t->gate)
						nodes[j]->pull = '+';
				}
				cur_t->depl = true;
				// This will get deleted, so reuse the node number
				nextNode--;
				pullups++;
			}
		}
#endif
	}
	diffs.clear();
#ifdef NMOS
	if (pullups > 0)
		printf("Deleted %i pullups\n", pullups);
#endif

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
	for (size_t i = 0; i < transistors.size(); i++)
	{
		cur_t = transistors[i];
		// Skip disabled transistors
		if (cur_t->gate == (cur_t->ptype ? pwr : gnd))
		{
			delete cur_t;
			continue;
		}
#ifdef NMOS
		// Skip depletion pullups
		if (cur_t->depl)
		{
			delete cur_t;
			continue;
		}
#endif
		fprintf(out, "['t%i',%i,%i,%i,[%i,%i,%i,%i],[%i,%i,%i,%i,%i],%s],\n", cur_t->id, cur_t->gate, cur_t->c1, cur_t->c2, cur_t->bbox.xmin, cur_t->bbox.xmax, cur_t->bbox.ymin, cur_t->bbox.ymax, cur_t->width1, cur_t->width2, cur_t->length, cur_t->segments, cur_t->area, cur_t->ptype ? "true" : "false");
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
	for (size_t i = 0; i < nodes.size(); i++)
	{
		cur = nodes[i];
		// skip powered/grounded metal nodes
		// CMOS does not include pullup/pulldown state in segdefs
		if (!((cur->layer == LAYER_METAL) && ((cur->id == pwr) || (cur->id == gnd))))
#ifdef NMOS
			fprintf(out, "[%i,'%c',%i,%s],\n", cur->id, cur->pull, cur->layer, cur->poly.toString().c_str());
#else
			fprintf(out, "[%i,%i,%s],\n", cur->id, cur->layer, cur->poly.toString().c_str());
#endif
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
