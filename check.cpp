/*
 * Netlist Generator Helper
 * Checks validity of vias and buried contacts
 *
 * Copyright (c) QMT Productions
 */

#include <stdio.h>
#include "polygon.h"

int main (int argc, char **argv)
{
	vector<node *> nodes, vias;
	node *cur, *sub;
	int i, j;

	int metal_start, metal_end;
	int poly_start, poly_end;
	int diff_start, diff_end;

	metal_start = nodes.size();
	readnodes<node>("metal_vcc.dat", nodes, LAYER_METAL);
	if (nodes.size() != 1)
	{
		fprintf(stderr, "Error: metal_vcc.dat contains more than one node (found %i)!\n", nodes.size());
		return 1;
	}
	readnodes<node>("metal_gnd.dat", nodes, LAYER_METAL);
	if (nodes.size() != 2)
	{
		fprintf(stderr, "Error: metal_gnd.dat contains more than one node (found %i)!\n", nodes.size() - 1);
		return 1;
	}
	readnodes<node>("metal.dat", nodes, LAYER_METAL);
	metal_end = nodes.size();

	printf("Checking metal segments (%i-%i)\n", metal_start + 2, metal_end - 1);
	for (i = metal_start + 2; i < metal_end; i++)
	{
		cur = nodes[i];
		int area = cur->poly.area();
		if (area < 16)
			printf("Metal segment %i (%s) is unusually small (%i)!\n", i, cur->poly.toString().c_str(), area);
		for (j = metal_start + 2; j < metal_end; j++)
		{
			if (i == j)
				continue;
			sub = nodes[j];
			if (cur->collide(sub))
				printf("Metal segments %i (%s) and %i (%s) collide!\n", i, cur->poly.toString().c_str(), j, sub->poly.toString().c_str());
		}
	}

	poly_start = nodes.size();
	readnodes<node>("polysilicon.dat", nodes, LAYER_POLY);
	poly_end = nodes.size();

	printf("Checking polysilicon segments (%i-%i)\n", poly_start, poly_end - 1);
	for (i = poly_start; i < poly_end; i++)
	{
		cur = nodes[i];
		int area = cur->poly.area();
		if (area < 16)
			printf("Polysilicon segment %i (%s) is unusually small (%i)!\n", i, cur->poly.toString().c_str(), area);
		for (j = poly_start; j < poly_end; j++)
		{
			if (i == j)
				continue;
			sub = nodes[j];
			if (cur->collide(sub))
				printf("Polysilicon segments %i (%s) and %i (%s) collide!\n", i, cur->poly.toString().c_str(), j, sub->poly.toString().c_str());
		}
	}

	diff_start = nodes.size();
	readnodes<node>("diffusion.dat", nodes, LAYER_DIFF);
	diff_end = nodes.size();

	printf("Checking diffusion segments (%i-%i)\n", diff_start, diff_end - 1);
	for (i = diff_start; i < diff_end; i++)
	{
		cur = nodes[i];
		int area = cur->poly.area();
		if (area < 16)
			printf("Diffusion segment %i (%s) is unusually small (%i)!\n", i, cur->poly.toString().c_str(), area);
		for (j = diff_start; j < diff_end; j++)
		{
			if (i == j)
				continue;
			sub = nodes[j];
			if (cur->collide(sub))
				printf("Diffusion segments %i (%s) and %i (%s) collide!\n", i, cur->poly.toString().c_str(), j, sub->poly.toString().c_str());
		}
	}
	readnodes<node>("vias.dat", vias, LAYER_SPECIAL);

	printf("Checking for bad vias (%i total)\n", vias.size());
	for (i = 0; i < vias.size(); i++)
	{
		int hits = 0;
		sub = vias[i];
		int area = sub->poly.area();
		if (area < 9)
			printf("Via %i (%s) is unusually small (%i)!\n", i, sub->poly.toString().c_str(), area);
		for (j = metal_start; j < diff_end; j++)
		{
			cur = nodes[j];
			if (cur->collide(sub))
				hits++;
		}
		if (hits != 2)
			printf("Invalid number of connections %i for via %i (%s)\n", hits, i, sub->poly.toString().c_str());
	}
	vias.clear();

	readnodes<node>("buried_contacts.dat", vias, LAYER_SPECIAL);

	printf("Checking for bad buried contacts (%i total)\n", vias.size());
	for (i = 0; i < vias.size(); i++)
	{
		int hits = 0;
		sub = vias[i];
		int area = sub->poly.area();
		if (area < 16)
			printf("Buried contact %i (%s) is unusually small (%i)!\n", i, sub->poly.toString().c_str(), area);
		for (j = poly_start; j < diff_end; j++)
		{
			cur = nodes[j];
			if (cur->collide(sub))
				hits++;
		}
		if (hits != 2)
			printf("Invalid number of connections %i for buried contact %i (%s)\n", hits, i, sub->poly.toString().c_str());
	}
	vias.clear();

	readnodes<node>("transistors.dat", vias, LAYER_SPECIAL);

	printf("Checking for bad transistors (%i total)\n", vias.size());
	for (i = 0; i < vias.size(); i++)
	{
		int hits = 0;
		sub = vias[i];
		int area = sub->poly.area();
		if (area < 15)
			printf("Transistor %i (%s) is unusually small (%i)!\n", i, sub->poly.toString().c_str(), area);
		for (j = poly_start; j < poly_end; j++)
		{
			cur = nodes[j];
			if (cur->collide(sub))
				hits++;
		}
		if (hits != 1)
			printf("Transistor %i (%s) connects to wrong number of polysilicon nodes (%i)\n", i, sub->poly.toString().c_str(), hits);
	}

	printf("Done!\n");
}
