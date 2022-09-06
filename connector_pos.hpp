// SPDX-License-Identifier: GPL-2.0
// This class is misnamed
// It describes the type and position of a connector or corner of an operator
// It is used to build visibility graphs

#ifndef CONNECTOR_POS
#define CONNECTOR_POS

#include <QPointF>

class Operator;

class ConnectorType {
	// n = -1..-4: (-n-1) is the corner as defined in operator.hpp
	// n = even: output connector #n-4
	// n = odd: input connector #n/2
	int n;
	ConnectorType(int n_);
public:
	ConnectorType();
	static ConnectorType input_connector(int id);
	static ConnectorType output_connector(int id);
	static ConnectorType corner(int id);
	bool is_connector() const;
	bool is_input_connector() const;
	bool is_output_connector() const;
	bool is_corner() const;
	int input_connector_id() const;
	int output_connector_id() const;
	int corner_id() const;

	bool operator==(ConnectorType t) const;
	bool operator!=(ConnectorType t) const;
};

class ConnectorPos {
public:

	ConnectorType type;
	QPointF pos;
	ConnectorPos(ConnectorType type, const QPointF &pos);
};

class ConnectorDesc {
public:
	Operator *op;
	ConnectorType type;
	ConnectorDesc(Operator *op, ConnectorType type);

	bool operator==(const ConnectorDesc &) const;
	bool operator!=(const ConnectorDesc &) const;
};

#endif
