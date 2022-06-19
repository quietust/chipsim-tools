/*
 * Netlist Generator
 * Originally designed for the Visual 2A03
 *
 * Copyright 2011-2022 QMT Productions
 */

#include <stdio.h>
#include "polygon.h"

#include <vector>
using namespace std;

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

	int nextNode = FIRST_SEG_ID;
	// Note: PWR must be less than GND, or other parts of this tool will fail
	int pwr = nextNode++;
	int gnd = nextNode++;

	size_t metal_start, metal_end;
	size_t poly_start, poly_end;
	size_t diff_start, diff_end;

	metal_start = nodes.size();
	readnodes<node>("metal_pwr.dat", nodes, LAYER_METAL, pwr);
	readnodes<node>("metal_gnd.dat", nodes, LAYER_METAL, gnd);
	readnodes<node>("metal.dat", nodes, LAYER_METAL);
	metal_end = nodes.size();

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

	vector<node *> vias, matched, unmatched;
	node *via, *cur, *sub;
	size_t i, j, k;

	readnodes<node>("vias.dat", vias, LAYER_SPECIAL);

	// PWR and GND are the two biggest metal nodes, accounting for most of the vias
	// so process them all first so the rest of the nodes have fewer vias to check
	printf("Parsing metal PWR nodes - %zi vias remaining\n", vias.size());
	for (i = metal_start; i < metal_end; i++)
	{
		cur = nodes[i];
		if (cur->id != pwr)
			continue;
		for (j = 0; j < vias.size(); j++)
		{
			via = vias[j];
			if (cur->collide(via))
				matched.push_back(via);
			else	unmatched.push_back(via);
		}
		while (!matched.empty())
		{
//			printf("%zi    \r", matched.size());
			via = matched.back();
			matched.pop_back();
			for (j = poly_start; j < diff_end; j++)
			{
				if (i == j)
					continue;
				sub = nodes[j];
				if (sub->collide(via))
				{
					sub->id = pwr;
					break;
				}
			}
			delete via;
		}
		vias = unmatched;
		unmatched.clear();
	}

	printf("Parsing metal GND nodes - %zi vias remaining\n", vias.size());
	for (i = metal_start; i < metal_end; i++)
	{
		cur = nodes[i];
		if (cur->id != gnd)
			continue;
		for (j = 0; j < vias.size(); j++)
		{
			via = vias[j];
			if (cur->collide(via))
				matched.push_back(via);
			else	unmatched.push_back(via);
		}
		while (!matched.empty())
		{
//			printf("%zi    \r", matched.size());
			via = matched.back();
			matched.pop_back();
			for (j = poly_start; j < diff_end; j++)
			{
				if (i == j)
					continue;
				sub = nodes[j];
				if (sub->collide(via))
				{
					if (sub->id == pwr)
					{
						fprintf(stderr, "Error - via %zi (%s) shorts PWR to GND!\n", i, via->poly.toString().c_str());
						return 2;
					}
					sub->id = gnd;
					break;
				}
			}
			delete via;
		}
		vias = unmatched;
		unmatched.clear();
	}

	printf("Parsing metal nodes %zi thru %zi - %zi vias remaining\n", metal_start, metal_end - 1, vias.size());
	for (i = metal_start; i < metal_end; i++)
	{
//		printf("%zi        \r", i);
		cur = nodes[i];
		// skip powered/grounded metal nodes, since we already handled them above
		if ((cur->id == pwr) || (cur->id == gnd))
			continue;
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
//			printf("%zi.%zi\r", i, matched.size());
			via = matched.back();
			matched.pop_back();
			for (j = poly_start; j < diff_end; j++)
			{
				sub = nodes[j];
				if (sub->collide(via))
				{
					if (sub->id == 0)
						sub->id = cur->id;
					else if (sub->id != cur->id)
					{
						// merge the two nodes together, assuming the lower ID number of the two
						int oldid = max(cur->id, sub->id);
						int newid = min(cur->id, sub->id);
						if ((oldid == gnd) && (newid == pwr))
						{
							fprintf(stderr, "Error - via %zi (%s) shorts PWR to GND!\n", i, via->poly.toString().c_str());
							return 2;
						}
						for (k = 0; k < nodes.size(); k++)
							if (nodes[k]->id == oldid)
								nodes[k]->id = newid;
					}
					break;
				}
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

	readnodes<node>("buried.dat", vias, LAYER_SPECIAL);

	printf("Parsing polysilicon nodes %zi thru %zi - %zi buried contacts remaining\n", poly_start, poly_end - 1, vias.size());
	for (i = poly_start; i < poly_end; i++)
	{
//		printf("%zi        \r", i);
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
//			printf("%zi.%zi\r", i, matched.size());
			via = matched.back();
			matched.pop_back();
			for (j = diff_start; j < diff_end; j++)
			{
				sub = nodes[j];
				if (sub->collide(via))
				{
					if (sub->id == 0)
						sub->id = cur->id;
					else if (sub->id != cur->id)
					{
						// merge the two nodes together, assuming the lower ID number of the two
						int oldid = max(cur->id, sub->id);
						int newid = min(cur->id, sub->id);
						if ((oldid == gnd) && (newid == pwr))
						{
							fprintf(stderr, "Error - buried contact %zi (%s) shorts PWR to GND!\n", i, via->poly.toString().c_str());
							return 2;
						}
						for (k = 0; k < nodes.size(); k++)
							if (nodes[k]->id == oldid)
								nodes[k]->id = newid;
					}
					break;
				}
			}
			delete via;
		}
		vias = unmatched;
		unmatched.clear();
		// Not strictly correct, but it handles the input protection diodes
		if (cur->id == gnd)
			cur->layer = LAYER_PROTECT;
		// alt usage: polysilicon hardwired to PWR doesn't show up highlighted in ChipSim,
		// so this can be used to make it highlighted
	}
	if (!vias.empty())
	{
		printf("%zi buried contacts were not matched!\n", vias.size());
		while (!vias.empty())
		{
			via = vias.back();
			vias.pop_back();
			fprintf(stderr, "Buried contact (%s) was not hit!\n", via->poly.toString().c_str());
			delete via;
		}
	}

	printf("Parsing diffusion nodes %zi thru %zi\n", diff_start, diff_end - 1);
	for (i = diff_start; i < diff_end; i++)
	{
		cur = nodes[i];
		if (!cur->id)
			cur->id = nextNode++;
		if (cur->id == pwr)
			cur->layer = LAYER_DIFF_PWR;
		if (cur->id == gnd)
			cur->layer = LAYER_DIFF_GND;
	}

	// TODO - add an option to go through all of the nodes and make the ID numbers consecutive

	readnodes<transistor>("trans.dat", transistors, LAYER_SPECIAL);
	transistor *cur_t;
	nextNode = FIRST_TRANS_ID;

	vector<node *> diffs;
	int pullups = 0;

	node *t1 = new node;
	node *t2 = new node;
	node *t3 = new node;
	node *t4 = new node;

	printf("Parsing %zi transistors\n", transistors.size());
	for (i = 0; i < transistors.size(); i++)
	{
//		printf("%zi     \r", i);
		cur_t = transistors[i];
		cur_t->id = nextNode++;
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

		cur_t->c1 = cur_t->c2 = -1;
		for (j = 0; j < diffs.size(); j++)
		{
			if (cur_t->c1 == -1)
				cur_t->c1 = diffs[j]->id;
			else
			{
				if (cur_t->c1 == diffs[j]->id)
					continue;
				if (cur_t->c2 == -1)
					cur_t->c2 = diffs[j]->id;
				else
				{
					if (cur_t->c2 == diffs[j]->id)
						continue;
					else	break;
				}
			}
		}

		if ((j != diffs.size()) || (cur_t->c1 == -1) || (cur_t->c2 == -1))
		{
			// assign dummy values
			cur_t->c1 = gnd;
			cur_t->c2 = gnd;
			fprintf(stderr, "Transistor %i had wrong number of terminals (%zi)\n", cur_t->id, diffs.size());
			continue;
		}

		// if any of the following are true, swap the terminals around
		// 1. 1st terminal is connected to PWR
		// 2. 1st terminal is connected to GND
		// 3. 2nd terminal is connected to gate (generally gets caught by case #1)
		if ((cur_t->c1 == pwr) || (cur_t->c1 == gnd) || (cur_t->c2 == cur_t->gate))
		{
			j = cur_t->c2;
			cur_t->c2 = cur_t->c1;
			cur_t->c1 = j;
		}

		// calculate geometry
		int segs0 = 0, segs1 = 0, segs2 = 0;
		for (j = 0; j < cur_t->poly.numVertices(); j++)
		{
			vertex v;
			int len = cur_t->poly.midpoint(j, v);
			for (k = 0; k < diffs.size(); k++)
			{
				if (diffs[k]->poly.isInside(v))
				{
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
					else	fprintf(stderr, "Transistor %i diffusion node %i doesn't belong to either side (%i/%i)?\n", cur_t->id, diffs[k]->id, cur_t->c1, cur_t->c2);
					break;
				}
			}
			if (k == diffs.size())
			{
				cur_t->length += len;
				segs0++;
			}
		}
		if (segs1 == segs2)
			cur_t->segments = segs1;
		else
		{
			fprintf(stderr, "Transistor %i had inconsistent number of segments on each side (%i/%i)!\n", cur_t->id, segs1, segs2);
			cur_t->segments = max(segs1, segs2);
		}
		if (segs0)
			cur_t->length /= segs0;
		else	fprintf(stderr, "Transistor %i has zero length?\n", cur_t->id);
		if (cur_t->c1 == pwr && cur_t->c2 == gnd)
			fprintf(stderr, "Transistor %i (%s) connects PWR to GND!\n", cur_t->id, cur_t->poly.toString().c_str());

		// if the gate is connected to one terminal and the other terminal is PWR/GND, assign pullup state to the other side
		if (cur_t->c1 == cur_t->gate)
		{
			// gate == c1, c2 == PWR -> it's a pull-up resistor
			// but don't do this if the other side is GND!
			if ((cur_t->c2 == pwr) && (cur_t->c1 != gnd))
			{
				// assign pull-up state
				for (j = metal_start; j < diff_end; j++)
				{
					if (nodes[j]->id == cur_t->gate)
						nodes[j]->pullup = '+';
				}
				cur_t->depl = true;
				// reuse the ID number
				nextNode--;
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
		if (cur_t->depl)
		{
			delete cur_t;
			continue;
		}
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
	for (i = 0; i < nodes.size(); i++)
	{
		cur = nodes[i];
		// skip powered/grounded metal nodes
		if (!((cur->layer == LAYER_METAL) && ((cur->id == pwr) || (cur->id == gnd))))
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
