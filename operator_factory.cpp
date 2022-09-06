// SPDX-License-Identifier: GPL-2.0
#define OPERATOR_FACTORY_C

#include "operator_factory.hpp"
#include "operator_const.hpp"
#include "operator_convolution.hpp"
#include "operator_fft.hpp"
#include "operator_gauss.hpp"
#include "operator_wave.hpp"
#include "operator_inversion.hpp"
#include "operator_lattice.hpp"
#include "operator_merge.hpp"
#include "operator_modulate.hpp"
#include "operator_mult.hpp"
#include "operator_pow.hpp"
#include "operator_powder.hpp"
#include "operator_pixmap.hpp"
#include "operator_polygon.hpp"
#include "operator_split.hpp"
#include "operator_sum.hpp"
#include "operator_view.hpp"

template <typename O>
static Operator *factory_func(MainWindow &w)
{
	return new O(w);
}

template <typename O>
void OperatorFactory::add(const char *name,
			  std::vector<OperatorFactory::Entry> &funcs,
			  std::vector<OperatorFactory::Desc> &descs,
			  std::vector<OperatorFactory::NameId> &names,
			  bool add_separator)
{
	funcs.push_back({ O::id, name, &factory_func<O> });
	descs.push_back({ O::id, name, O::icon, O::tooltip, O::get_init_states(), add_separator });
	names.push_back({ O::id, name });
}

OperatorFactory::OperatorFactory()
{
	funcs.reserve(40);
	descs.reserve(40);
	names.reserve(40);

	// Since string literal template parameters are only possible with C++20 onward,
	// pass the names here. Move them down to the operators when switching to C++20.
	add<OperatorPixmap>("pixmap", funcs, descs, names, false);
	add<OperatorPolygon>("polygon", funcs, descs, names, false);
	add<OperatorGauss>("gauss", funcs, descs, names, false);
	add<OperatorLattice>("lattice", funcs, descs, names, false);
	add<OperatorWave>("wave", funcs, descs, names, false);
	add<OperatorConst>("const", funcs, descs, names, true);

	add<OperatorView>("view", funcs, descs, names, true);

	add<OperatorFFT>("ffr", funcs, descs, names, false);
	add<OperatorConvolution>("convolution", funcs, descs, names, true);

	add<OperatorSum>("sum", funcs, descs, names, false);
	add<OperatorMult>("mult", funcs, descs, names, false);
	add<OperatorPow>("pow", funcs, descs, names, false);
	add<OperatorInversion>("inversion", funcs, descs, names, false);
	add<OperatorPowder>("powder", funcs, descs, names, true);

	add<OperatorSplit>("split", funcs, descs, names, false);
	add<OperatorMerge>("merge", funcs, descs, names, false);
	add<OperatorModulate>("modulate", funcs, descs, names, false);

	std::sort(funcs.begin(), funcs.end());
	std::sort(names.begin(), names.end());
}

bool OperatorFactory::Entry::operator<(const Entry &e) const
{
	return id < e.id;
}

bool OperatorFactory::Entry::operator<(OperatorId id_) const
{
	return id < id_;
}

bool OperatorFactory::NameId::operator<(const NameId &e) const
{
	return strcmp(name, e.name) < 0;
}

bool OperatorFactory::NameId::operator<(const char *name_) const
{
	return strcmp(name, name_) < 0;
}

std::unique_ptr<Operator> OperatorFactory::make(OperatorId id, MainWindow &w) const
{
	auto it = std::lower_bound(funcs.begin(), funcs.end(), id);
	return it != funcs.end() && it->id == id ? std::unique_ptr<Operator>((it->func)(w))
						 : nullptr;
}

std::unique_ptr<Operator> OperatorFactory::make(OperatorId id, const Operator::State &state, MainWindow &w) const
{
	auto res = make(id, w);
	res->set_state(state);
	return res;
}

const std::vector<OperatorFactory::Desc> &OperatorFactory::get_descs() const
{
	return descs;
}

OperatorId OperatorFactory::string_to_id(const std::string &s) const
{
	auto it = std::lower_bound(names.begin(), names.end(), s.c_str());
	return it != names.end() && strcmp(it->name, s.c_str()) == 0 ?
		it->id : static_cast<OperatorId>(-1);
}

std::string OperatorFactory::id_to_string(OperatorId id) const
{
	auto it = std::lower_bound(funcs.begin(), funcs.end(), id);
	return it != funcs.end() && it->id == id ? std::string(it->name)
						 : std::string();
}
