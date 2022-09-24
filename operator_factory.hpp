// SPDX-License-Identifier: GPL-2.0
// This class generates operators from ids.
// It also defines an order in which operators are added to the main menu
// and the toolbar.

#ifndef OPERATOR_FACTORY_HPP
#define OPERATOR_FACTORY_HPP

#include "operator.hpp"

#include <vector>
#include <memory>

class OperatorFactory {
public:
	struct Desc {
		OperatorId id;
		const char *name; // For saving
		const char *icon;
		const char *tooltip;
		std::vector<Operator::InitState> init_states;
		bool add_separator;
	};
private:
	struct Entry {
		OperatorId id;
		const char *name;			// TODO: Remove and implement at operator level with C++20.
		Operator *(*func)(MainWindow &);
		bool operator<(const Entry &) const;
		bool operator<(OperatorId) const;
	};
	struct NameId {
		OperatorId id;
		const char *name;
		bool operator<(const NameId &) const;
		bool operator<(const char *) const;
	};

	template <typename O>
	static void add(const char *name,
			std::vector<OperatorFactory::Entry> &funcs,
			std::vector<OperatorFactory::Desc> &descs,
			std::vector<OperatorFactory::NameId> &names,
			bool add_separator);

	// Vector of factory functions ordered by id
	std::vector<Entry> funcs;

	// Vector of ids, in the order of the main menu / toolbar.
	std::vector<Desc> descs;

	// To lookup by name
	std::vector<NameId> names;
public:
	OperatorFactory();

	// Generate operator. Returns nullptr for unknown id!
	std::unique_ptr<Operator> make(OperatorId id, MainWindow &) const;
	std::unique_ptr<Operator> make(OperatorId id, const Operator::State &, MainWindow &) const;
	const std::vector<Desc> &get_descs() const;
	OperatorId string_to_id(const std::string &s) const;
	std::string id_to_string(OperatorId id) const; // TODO: Implement at operator level, with template, just as id.
};

extern OperatorFactory operator_factory;

#endif
