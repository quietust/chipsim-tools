chipsim-tools
=============

Tools I use for converting chip layer images into netlists for ChipSim

pngtrace
--------
Reads a PNG file containing a layer image and traces all polygons encountered
inside it. On first run, it must be taught how to trace nodes - with NumLock
ON, use the number pad to tell it which direction to go, or press 'q' to exit
and save the list of learned rules (or 'x' to abort without saving). The next
time you run it, the previously stored rules will be automatically imported.

Input images can contain multiple sets of nodes in different colors - specify
the RGB value you want to trace, or specify 000000 to include all nodes.
All transparent regions are treated as black, and the image's background
color is ignored. There must not be any partially-transparent pixels.

Currently does NOT recognize hollow nodes during tracing - run with the
"--hollow" option beforehand to ensure that there are no holes (or "--invert"
to trace the interior of every hole for later analysis).

check
-----
Checks polygon data for consistency, making sure that each via and buried
contact connects exactly two nodes together and that no two nodes in the same
layer collide with one another.

netlist
-------
Reads in all of the layers, figures out which segments are connected to each
other (and assigns node IDs appropriately), and builds segdefs.js and
transdefs.js files for ChipSim.

By default, this tool compiles in NMOS mode, but it can be easily altered to
build in CMOS mode instead.

Usage
=====
Save each layer image as a PNG file, either with a black background or a
transparent background, then use "pngtrace" to vectorize each set of nodes
into ".dat" files with the following names:
* Upper Metal: 'metal2_pwr' + 'metal2_gnd' + 'metal2' + 'vias2'
* Lower Metal: 'metal1_pwr' + 'metal1_gnd' + 'metal1' + 'vias1'
* Single Metal: 'metal_pwr' + 'metal_gnd' + 'metal' + 'vias'
* Poly: 'poly_pwr' + 'poly_gnd' + 'poly' + 'buried'
* Diffusion: 'diff_pwr' + 'diff_gnd' + 'diff'
* Transistors: 'trans'/ 'trans_n' + 'trans_p'

Currently, a maximum of 2 metal layers are supported, with the upper metal
layer only forming connections to the lower metal layer. For chips with only
one metal layer, use the "Single Metal" layer files. The 'buried' layer is
only supported with NMOS chips, and the 'trans_p' layer is only supported
with CMOS chips.

At least one set of 'pwr' and 'gnd' nodes must be provided.

Once all necessary files have been generated, run "check", verify that no
errors are reported, then run "netlist".
