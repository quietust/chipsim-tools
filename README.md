chipsim-tools
=============

Tools I use for converting chip layer images into netlists for Visual6502's ChipSim

convertsvg.php
--------------
Converts exported SVG files to vertex lists for the tools below.

check.cpp
---------
Checks polygon data for consistency - makes sure that each via and buried contact connects exactly two nodes together and that no metal/polysilicon/diffusion segments are hollow.

netlist.cpp
-----------
Reads in all of the layers, figures out which segments are connected to each other (and assigns node IDs appropriately), and builds segdefs.js and transdefs.js files for ChipSim.

polygon.h
---------
Used by the above two programs, keeps track of the shape and location of each segment and quickly determines whether or not two arbitrary segments intersect.

Usage
=====
Using GIMP, select all segments in each layer image (using either "Alpha to Selection" or "Select by Color"), perform "Selection to Path" with advanced settings (Corner Threshold:150, Line Reversion Threshold:0.200, Line Threshold:2.00, and optionally Corner Surround:3), export path to SVG, then run "convertsvg.php". If the SVG happened to contain any non-straight lines, it'll complain - go back into GIMP and fix the problem segments to make them more square, or recreate the Path with more strict settings. Once the SVG files are converted, run "check", verify that there are no errors, then run "netlist".

Recognized filenames are: 'metal_pwr' + 'metal_gnd' + 'metal', 'poly_pwr' + 'poly_gnd' + 'poly', 'diff_pwr' + 'diff_gnd' + 'diff', 'vias', 'buried', and 'trans'. At least one set of 'pwr'/'gnd' nodes must be provided.
