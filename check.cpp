#include <stdio.h>
#include "polygon.h"

#define	LAYER_METAL	0
#define	LAYER_DIFF	1
#define	LAYER_PROTECT	2
#define	LAYER_DIFF_GND	3
#define	LAYER_DIFF_VCC	4
#define	LAYER_POLY	5
#define	LAYER_SPECIAL	6

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
			if (layer == LAYER_SPECIAL)
				n->poly.add(x * 2 + 1, 12512 - y * 2 + 1);
			else	n->poly.add(x * 2, 12512 - y * 2);
		}
	}
	delete n;
}

int main (int argc, char **argv)
{
	vector<node *> nodes, vias;
	node *via, *cur;
	int i, j;

	int metal_start, metal_end;
	int poly_start, poly_end;
	int diff_start, diff_end;
	readnodes<node>("metal_vcc.dat", nodes, LAYER_METAL);
	if (nodes.size() != 1)
	{
		fprintf(stderr, "Error: VCC plane contains more than one node!\n");
		return 1;
	}
	readnodes<node>("metal_gnd.dat", nodes, LAYER_METAL);
	if (nodes.size() != 2)
	{
		fprintf(stderr, "Error: GND plane contains more than one node!\n");
		return 1;
	}
	metal_start = nodes.size();
	readnodes<node>("metal.dat", nodes, LAYER_METAL);
	metal_end = poly_start = nodes.size();
	readnodes<node>("polysilicon.dat", nodes, LAYER_POLY);
	poly_end = diff_start = nodes.size();
	readnodes<node>("diffusion.dat", nodes, LAYER_DIFF);
	diff_end = nodes.size();

	readnodes<node>("vias.dat", vias, LAYER_SPECIAL);

	printf("Checking for bad vias (%i total)\n", vias.size());
	for (i = 0; i < vias.size(); i++)
	{
//		printf("%i     \r", i);
		int hits = 0;
		via = vias[i];
		// not metal_start - we need to include the power planes
		for (j = 0; j < diff_end; j++)
		{
			cur = nodes[j];
			if (cur->collide(via))
				hits++;
		}
		if (hits == 2)
			continue;
		if (hits == 1)
			printf("Via %i (%s) goes to nowhere!\n", i, via->poly.toString().c_str());
		else	printf("Via %i (%s) connects to more than 2 nodes (found %i)!\n", i, via->poly.toString().c_str(), hits);
	}

	vias.clear();

	readnodes<node>("buried_contacts.dat", vias, LAYER_SPECIAL);

	printf("Checking for bad buried contacts (%i total)\n", vias.size());
	for (i = 0; i < vias.size(); i++)
	{
//		printf("%i     \r", i);
		int hits = 0;
		via = vias[i];
		for (j = poly_start; j < diff_end; j++)
		{
			cur = nodes[j];
			if (cur->collide(via))
				hits++;
		}
		if (hits == 2)
			continue;
		if (hits == 1)
			printf("Buried contact %i (%s) goes to nowhere!\n", i, via->poly.toString().c_str());
		else	printf("Buried contact %i (%s) connects to more than 2 nodes (found %i)!\n", i, via->poly.toString().c_str(), hits);
	}
	vias.clear();
	printf("Done!\n");
}
