// SPDX-License-Identifier: GPL-2.0
// A set of example files
#ifndef EXAMPLES_HPP
#define EXAMPLES_HPP

#include <vector>

class Examples {
public:
	struct Desc {
		const char *id;
		const char *name;
		const char *description;
	};

	const std::vector<Desc> get_descs() const;
};

extern Examples examples;

#endif
