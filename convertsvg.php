<?php
/*
 * Netlist Generator Helper
 * Converts SVG outlines of polygons into vertex lists used by other tools
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

function convert_svg ($in, $out)
{
	if (!is_file($in))
	{
		if (is_file($out))
			echo "Input file $in not found - assuming $out is up to date.\n";
		else	echo "Input file $in does not exist!\n";
		return;
	}
	if (is_file($out) && (filemtime($in) <= filemtime($out)))
	{
		echo "$out is up to date, skipping.\n";
		return;
	}
	echo "Parsing $in...\n";
	$data = '';
	$raw = file_get_contents($in);
	$xml = new XMLReader();
	$xml->XML($raw);
	while (1)
	{
		$xml->read() or die('Unable to locate image data!');
		if ($xml->name != 'path')
			continue;
		$raw = $xml->getAttribute('d');
		break;
	}
	$raw = preg_split('/[\s]+/', $raw);
	$i = 0;
	$cmd = $raw[$i++];
	while ($i < count($raw))
	{
		switch ($cmd)
		{
		case 'M':
			$lastpt = $raw[$i++];
			$cmd = $raw[$i++];
			break;
		case 'C':
			$pt1 = $raw[$i++];
			if ($pt1 == 'Z')
			{
				$data .= "-1,-1\n";
				if ($i < count($raw))
					$cmd = $raw[$i++];
			}
			else
			{
				$pt2 = $raw[$i++];
				$pt3 = $raw[$i++];
				if (($lastpt != $pt1) || ($pt2 != $pt3))
					echo "Curve detected at $pt1! Please fix and re-export.\n";
				$data .= str_replace('.00', '', $pt3)."\n";
				$lastpt = $pt3;
			}
			break;
		default:
			die("Unknown command $cmd!");
		}
	}
	file_put_contents($out, $data);
}

$layers = array(
	'metal_vcc',
	'metal_gnd',
	'metal',
	'vias',
	'polysilicon',
	'buried_contacts',
	'diffusion',
	'transistors',
);
foreach ($layers as $layer)
	convert_svg($layer .'.svg', $layer .'.dat');
?>
