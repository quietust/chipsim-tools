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

	size_t metal2_start, metal2_end;
	size_t metal1_start, metal1_end;
	size_t poly_start, poly_end;
	size_t diff_start, diff_end;

	metal2_start = nodes.size();
	readnodes<node>("metal2_pwr.dat", nodes, LAYER_METAL);
	readnodes<node>("metal2_gnd.dat", nodes, LAYER_METAL);
	readnodes<node>("metal2.dat", nodes, LAYER_METAL);
	metal2_end = nodes.size();

	metal1_start = nodes.size();
	readnodes<node>("metal1_pwr.dat", nodes, LAYER_METAL);
	readnodes<node>("metal1_gnd.dat", nodes, LAYER_METAL);
	readnodes<node>("metal1.dat", nodes, LAYER_METAL);
	// Single-metal NMOS compat
	readnodes<node>("metal_pwr.dat", nodes, LAYER_METAL);
	readnodes<node>("metal_gnd.dat", nodes, LAYER_METAL);
	readnodes<node>("metal.dat", nodes, LAYER_METAL);
	metal1_end = nodes.size();

	poly_start = nodes.size();
	readnodes<node>("poly_pwr.dat", nodes, LAYER_POLY);
	readnodes<node>("poly_gnd.dat", nodes, LAYER_POLY);
	readnodes<node>("poly.dat", nodes, LAYER_POLY);
	poly_end = nodes.size();

	diff_start = nodes.size();
	readnodes<node>("diff_pwr.dat", nodes, LAYER_DIFF);
	readnodes<node>("diff_gnd.dat", nodes, LAYER_DIFF);
	readnodes<node>("diff.dat", nodes, LAYER_DIFF);
	diff_end = nodes.size();

	printf("Checking metal2 segments (%zi-%zi)\n", metal2_start, metal2_end - 1);
	for (i = metal2_start; i < metal2_end; i++)
	{
		cur = nodes[i];
		int area = cur->poly.area();
		if (area < 16)
			printf("Metal2 segment %zi (%s) is unusually small (%i)!\n", i, cur->poly.toString().c_str(), area);
		for (j = i + 1; j < metal2_end; j++)
		{
			sub = nodes[j];
			// Check collisions in both directions, in case one way fails
			if (cur->collide(sub) || sub->collide(cur))
				printf("Metal2 segments %zi (%s) and %zi (%s) collide!\n", i, cur->poly.toString().c_str(), j, sub->poly.toString().c_str());
		}
	}

	printf("Checking metal1 segments (%zi-%zi)\n", metal1_start, metal1_end - 1);
	for (i = metal1_start; i < metal1_end; i++)
	{
		cur = nodes[i];
		int area = cur->poly.area();
		if (area < 16)
			printf("Metal1 segment %zi (%s) is unusually small (%i)!\n", i, cur->poly.toString().c_str(), area);
		for (j = i + 1; j < metal1_end; j++)
		{
			sub = nodes[j];
			// Check collisions in both directions, in case one way fails
			if (cur->collide(sub) || sub->collide(cur))
				printf("Metal1 segments %zi (%s) and %zi (%s) collide!\n", i, cur->poly.toString().c_str(), j, sub->poly.toString().c_str());
		}
	}

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

	readnodes<node>("vias2.dat", vias, LAYER_SPECIAL);
	printf("Checking for bad vias2 (%zi total)\n", vias.size());
	for (i = 0; i < vias.size(); i++)
	{
		int hits = 0;
		sub = vias[i];
		int area = sub->poly.area();
		if (area < 9)
			printf("Via2 %zi (%s) is unusually small (%i)!\n", i, sub->poly.toString().c_str(), area);
		for (j = metal2_start; j < metal1_end; j++)
		{
			cur = nodes[j];
			if (cur->collide(sub) || sub->collide(cur))
				hits++;
		}
		if (hits != 2)
			printf("Invalid number of connections %i for via2 %zi (%s)\n", hits, i, sub->poly.toString().c_str());
	}
	vias.clear();

	readnodes<node>("vias1.dat", vias, LAYER_SPECIAL);
	// Single-metal NMOS compat
	readnodes<node>("vias.dat", vias, LAYER_SPECIAL);
	printf("Checking for bad vias1 (%zi total)\n", vias.size());
	for (i = 0; i < vias.size(); i++)
	{
		int hits = 0;
		sub = vias[i];
		int area = sub->poly.area();
		if (area < 9)
			printf("Via1 %zi (%s) is unusually small (%i)!\n", i, sub->poly.toString().c_str(), area);
		for (j = metal1_start; j < diff_end; j++)
		{
			cur = nodes[j];
			if (cur->collide(sub) || sub->collide(cur))
				hits++;
		}
		if (hits != 2)
			printf("Invalid number of connections %i for via1 %zi (%s)\n", hits, i, sub->poly.toString().c_str());
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

	readnodes<node>("trans_n.dat", vias, LAYER_SPECIAL);
	readnodes<node>("trans_p.dat", vias, LAYER_SPECIAL);
	// Single-metal NMOS compat
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
