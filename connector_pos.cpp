// SPDX-License-Identifier: GPL-2.0
#include "connector_pos.hpp"

#include <cassert>

ConnectorType::ConnectorType(int n_)
	: n(n_)
{
}

ConnectorType::ConnectorType()
	: n(-5)
{
}

ConnectorType ConnectorType::input_connector(int id)
{
	assert(id >= 0);
	return id * 2 + 1;
}

ConnectorType ConnectorType::output_connector(int id)
{
	assert(id >= 0);
	return id * 2;
}

ConnectorType ConnectorType::corner(int id)
{
	assert(id >= 0 && id < 4);
	return -id - 1;
}

bool ConnectorType::is_connector() const
{
	return n >= 0;
}

bool ConnectorType::is_input_connector() const
{
	return n >= 0 && (n % 2 != 0);
}

bool ConnectorType::is_output_connector() const
{
	return n >= 0 && (n % 2 == 0);
}

bool ConnectorType::is_corner() const
{
	return n < 0;
}

int ConnectorType::input_connector_id() const
{
	assert(is_input_connector());
	return (n - 1) / 2;
}

int ConnectorType::output_connector_id() const
{
	assert(is_output_connector());
	return n / 2;
}

int ConnectorType::corner_id() const
{
	assert(is_corner());
	return -n - 1;
}

bool ConnectorType::operator==(ConnectorType t) const
{
	return n == t.n;
}

bool ConnectorType::operator!=(ConnectorType t) const
{
	return n != t.n;
}

ConnectorPos::ConnectorPos(ConnectorType type_, const QPointF &pos_)
	: type(type_)
	, pos(pos_)
{
}

ConnectorDesc::ConnectorDesc(Operator *op_, ConnectorType type_)
	: op(op_)
	, type(type_)
{
}

bool ConnectorDesc::operator==(const ConnectorDesc &c) const
{
	return op == c.op && type == c.type;
}

bool ConnectorDesc::operator!=(const ConnectorDesc &c) const
{
	return op != c.op || type != c.type;
}

