/*
 * Netlist Generator Helper
 * Checks validity of vias and buried contacts
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

	printf("Checking for hollow metal segments (%i-%i)\n", metal_start, metal_end - 1);
	for (i = metal_start; i < metal_end; i++)
	{
		cur = nodes[i];
		for (j = metal_start; j < metal_end; j++)
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

	printf("Checking for hollow polysilicon segments (%i-%i)\n", poly_start, poly_end - 1);
	for (i = poly_start; i < poly_end; i++)
	{
		cur = nodes[i];
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

	printf("Checking for hollow diffusion segments (%i-%i)\n", diff_start, diff_end - 1);
	for (i = diff_start; i < diff_end; i++)
	{
		cur = nodes[i];
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

	printf("Done!\n");
}
