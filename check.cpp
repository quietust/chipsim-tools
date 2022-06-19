/*
 * Netlist Generator Helper
 * Checks validity of nodes before actually trying to generate a netlist
 *
 * Copyright (c) QMT Productions
 */

#include <stdio.h>
#include "polygon.h"

int main (int argc, char **argv)
{
	std::vector<node *> nodes, vias;
	node *cur, *sub;
	size_t i, j;

	size_t metal_start, metal_end;
	size_t poly_start, poly_end;
	size_t diff_start, diff_end;

	metal_start = nodes.size();
	readnodes<node>("metal_pwr.dat", nodes, LAYER_METAL);
	readnodes<node>("metal_gnd.dat", nodes, LAYER_METAL);
	readnodes<node>("metal.dat", nodes, LAYER_METAL);
	metal_end = nodes.size();

	printf("Checking metal segments (%zi-%zi)\n", metal_start, metal_end - 1);
	for (i = metal_start; i < metal_end; i++)
	{
		cur = nodes[i];
		int area = cur->poly.area();
		if (area < 16)
			printf("Metal segment %zi (%s) is unusually small (%i)!\n", i, cur->poly.toString().c_str(), area);
		for (j = i + 1; j < metal_end; j++)
		{
			sub = nodes[j];
			// Check collisions in both directions, in case one way fails
			if (cur->collide(sub) || sub->collide(cur))
				printf("Metal segments %zi (%s) and %zi (%s) collide!\n", i, cur->poly.toString().c_str(), j, sub->poly.toString().c_str());
		}
	}

	poly_start = nodes.size();
	readnodes<node>("poly_pwr.dat", nodes, LAYER_POLY);
	readnodes<node>("poly_gnd.dat", nodes, LAYER_POLY);
	readnodes<node>("poly.dat", nodes, LAYER_POLY);
	poly_end = nodes.size();

	printf("Checking polysilicon segments (%zi-%zi)\n", poly_start, poly_end - 1);
	for (i = poly_start; i < poly_end; i++)
	{
		cur = nodes[i];
		int area = cur->poly.area();
		if (area < 16)
			printf("Polysilicon segment %zi (%s) is unusually small (%i)!\n", i, cur->poly.toString().c_str(), area);
		for (j = i + 1; j < poly_end; j++)
		{
			sub = nodes[j];
			if (cur->collide(sub) || sub->collide(cur))
				printf("Polysilicon segments %zi (%s) and %zi (%s) collide!\n", i, cur->poly.toString().c_str(), j, sub->poly.toString().c_str());
		}
	}

	diff_start = nodes.size();
	readnodes<node>("diff_pwr.dat", nodes, LAYER_DIFF);
	readnodes<node>("diff_gnd.dat", nodes, LAYER_DIFF);
	readnodes<node>("diff.dat", nodes, LAYER_DIFF);
	diff_end = nodes.size();

	printf("Checking diffusion segments (%zi-%zi)\n", diff_start, diff_end - 1);
	for (i = diff_start; i < diff_end; i++)
	{
		cur = nodes[i];
		int area = cur->poly.area();
		if (area < 16)
			printf("Diffusion segment %zi (%s) is unusually small (%i)!\n", i, cur->poly.toString().c_str(), area);
		for (j = i + 1; j < diff_end; j++)
		{
			sub = nodes[j];
			if (cur->collide(sub) || sub->collide(cur))
				printf("Diffusion segments %zi (%s) and %zi (%s) collide!\n", i, cur->poly.toString().c_str(), j, sub->poly.toString().c_str());
		}
	}
	readnodes<node>("vias.dat", vias, LAYER_SPECIAL);

	printf("Checking for bad vias (%zi total)\n", vias.size());
	for (i = 0; i < vias.size(); i++)
	{
		int hits = 0;
		sub = vias[i];
		int area = sub->poly.area();
		if (area < 9)
			printf("Via %zi (%s) is unusually small (%i)!\n", i, sub->poly.toString().c_str(), area);
		for (j = metal_start; j < diff_end; j++)
		{
			cur = nodes[j];
			if (cur->collide(sub) || sub->collide(cur))
				hits++;
		}
		if (hits != 2)
			printf("Invalid number of connections %i for via %zi (%s)\n", hits, i, sub->poly.toString().c_str());
	}
	vias.clear();

	readnodes<node>("buried.dat", vias, LAYER_SPECIAL);

	printf("Checking for bad buried contacts (%zi total)\n", vias.size());
	for (i = 0; i < vias.size(); i++)
	{
		int hits = 0;
		sub = vias[i];
		int area = sub->poly.area();
		if (area < 16)
			printf("Buried contact %zi (%s) is unusually small (%i)!\n", i, sub->poly.toString().c_str(), area);
		for (j = poly_start; j < diff_end; j++)
		{
			cur = nodes[j];
			if (cur->collide(sub) || sub->collide(cur))
				hits++;
		}
		if (hits != 2)
			printf("Invalid number of connections %i for buried contact %zi (%s)\n", hits, i, sub->poly.toString().c_str());
	}
	vias.clear();

	readnodes<node>("trans.dat", vias, LAYER_SPECIAL);

	printf("Checking for bad transistors (%zi total)\n", vias.size());
	for (i = 0; i < vias.size(); i++)
	{
		int hits = 0;
		sub = vias[i];
		int area = sub->poly.area();
		if (area < 15)
			printf("Transistor %zi (%s) is unusually small (%i)!\n", i, sub->poly.toString().c_str(), area);
		for (j = poly_start; j < poly_end; j++)
		{
			cur = nodes[j];
			if (cur->collide(sub) || sub->collide(cur))
				hits++;
		}
		if (hits != 1)
			printf("Transistor %zi (%s) connects to wrong number of polysilicon nodes (%i)\n", i, sub->poly.toString().c_str(), hits);
	}

	printf("Done!\n");
}
