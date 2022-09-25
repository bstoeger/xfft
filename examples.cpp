// SPDX-License-Identifier: GPL-2.0
#include "examples.hpp"

Examples examples;

static const std::vector<Examples::Desc> descs = {
	{ "iteration",		"Iteration",		"Double application of the FT restores the original up to inversion" },
	{ "inverse",		"Inverse FT",		"The FT is bijective (for reasonable functions)" },
	{ "polygons",		"Polygons",		"The FT of polygons is the convolution of sinc functions" },
	{ "origin",		"Origin",		"An origin shift modulates the phases, but not the intensity" },
	{ "linearity",		"Linearity",		"The relative position of objects is found in the wiggles" },
	{ "quasicrystal",	"Quasicrystals",	"FT five dots to get a quasiperiodic pattern" },
	{ "phaseproblem",	"Phase problem",	"Are the phases more important than the intesities?" },
	{ "convolution",	"Convolution",		"Paint on the canvas to show the effect of the convolution" },
	{ "ADP",		"ADP",			"Displacement of atoms exponentially dampen the intensities" },
	{ "truncation",		"Truncation",		"Even though we lose information the original can be reconstructed from little data" },
	{ "lattice",		"Lattice",		"The FT of a lattice is the reciprocal lattice" },
	{ "crystal",		"Crystal",		"A crystal is the convolution of a lattice with a base" },
	{ "homometry",		"Homometry",		"Homometric pairs can be constructed with the convolution" },
};

const std::vector<Examples::Desc> Examples::get_descs() const
{
	return descs;
}
