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

Currently does NOT recognize hollow nodes - if you have any, the vertex list
generated may not make sense and the remaining tools below may throw errors.

check
-----
Checks polygon data for consistency - makes sure that each via and buried
contact connects exactly two nodes together and that no segments are hollow.

netlist
-------
Reads in all of the layers, figures out which segments are connected to each
other (and assigns node IDs appropriately), and builds segdefs.js and
transdefs.js files for ChipSim.

By default, compiles in CMOS mode, but can be easily altered to run in NMOS
mode instead.

Usage
=====
Save each layer image as a transparent PNG file, then use "pngtrace" to
vectorize each one into a matching ".dat" file.

Once all necessary files have been generated, run "check", verify that no
errors are reported, then run "netlist".

Recognized filenames:
* 'metal2_pwr' + 'metal2_gnd' + 'metal2' + 'vias2'
* 'metal1_pwr' + 'metal1_gnd' + 'metal1' + 'vias1'
* 'metal_pwr' + 'metal_gnd' + 'metal' + 'vias'
* 'poly_pwr' + 'poly_gnd' + 'poly' + (NMOS) 'buried'
* 'diff_pwr' + 'diff_gnd' + 'diff'
* 'trans'/ 'trans_n' + 'trans_p'

At least one set of 'pwr'/'gnd' nodes must be provided.
