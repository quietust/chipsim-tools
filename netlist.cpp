/*
 * Netlist Generator
 * Originally designed for the Visual 2A03
 *
 * Copyright 2011-2012 QMT Productions
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

	int metal_start, metal_end;
	int poly_start, poly_end;
	int diff_start, diff_end;
	readnodes<node>("metal_pwr.dat", nodes, LAYER_METAL);
	if (nodes.size() != 1)
	{
		fprintf(stderr, "Error: metal_pwr.dat contains more than one node!\n");
		return 1;
	}
	readnodes<node>("metal_gnd.dat", nodes, LAYER_METAL);
	if (nodes.size() != 2)
	{
		fprintf(stderr, "Error: metal_gnd.dat contains more than one node!\n");
		return 1;
	}
	// metal_start begins after PWR/GND, since those are handled separately
	metal_start = nodes.size();
	readnodes<node>("metal.dat", nodes, LAYER_METAL);
	metal_end = poly_start = nodes.size();
	readnodes<node>("polysilicon.dat", nodes, LAYER_POLY);
	poly_end = diff_start = nodes.size();
	readnodes<node>("diffusion.dat", nodes, LAYER_DIFF);
	diff_end = nodes.size();

	int nextNode = FIRST_SEG_ID;
	int pwr = nextNode++;
	int gnd = nextNode++;

	vector<node *> vias, matched, unmatched;
	node *via, *cur, *sub;
	int i, j, k;

	readnodes<node>("vias.dat", vias, LAYER_SPECIAL);

	// PWR and GND are the two biggest nodes, accounting for most of the vias
	// so report progress on them before proceeding to the rest of the chip
	printf("Parsing PWR node - %i vias remaining\n", vias.size());
	{
		cur = nodes[0];
		cur->id = pwr;
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
					sub->id = pwr;
					break;
				}
			}
			delete via;
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
					if (sub->id == pwr)
					{
						fprintf(stderr, "Error - via %i (%s) shorts PWR to GND!\n", i, via->poly.toString().c_str());
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
					if (sub->id == 0)
						sub->id = cur->id;
					else if (sub->id != cur->id)
					{
						// merge the two nodes together, assuming the lower ID number of the two
						int oldid = max(cur->id, sub->id);
						int newid = min(cur->id, sub->id);
						if ((oldid == 2) && (newid == 1))
						{
							fprintf(stderr, "Error - via %i (%s) shorts PWR to GND!\n", i, via->poly.toString().c_str());
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
					if (sub->id == 0)
						sub->id = cur->id;
					else if (sub->id != cur->id)
					{
						// merge the two nodes together, assuming the lower ID number of the two
						int oldid = max(cur->id, sub->id);
						int newid = min(cur->id, sub->id);
						if ((oldid == 2) && (newid == 1))
						{
							fprintf(stderr, "Error - buried contact %i (%s) shorts PWR to GND!\n", i, via->poly.toString().c_str());
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
		// alt usage: polysilicon hardwired to PWR doesn't show up highlighted in ChipSim
		// if (cur->id == pwr)
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
		if (cur->id == pwr)
			cur->layer = LAYER_DIFF_PWR;
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
			fprintf(stderr, "Transistor %i had wrong number of terminals (%i)\n", cur_t->id, diffs.size());
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
	// skip the main PWR and GND nodes, since those are huge and we don't really need them
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
