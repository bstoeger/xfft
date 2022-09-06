// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_GAUSS_HPP
#define OPERATOR_GAUSS_HPP

#include "operator.hpp"

#include <QImage>
#include <QTransform>

class OperatorGaussState final : public Operator::StateTemplate<OperatorGaussState> {
	QJsonObject to_json() const override;
	void from_json(const QJsonObject &) override;
public:
	double e1 = 0.05, e2 = 0.05;	// Eigenvalues of correlation matrix.
	double angle = 0.0;		// Rotation angle of first eigenvector.
	QPointF offset { 0.0, 0.0 };
};

class OperatorGauss : public OperatorTemplate<OperatorId::Gauss, OperatorGaussState>
{
	// Factor corresponding to 90% confidence ellipse
	static constexpr double s_factor = 1.28155;

	QImage image;

	size_t num_input() const override;
	size_t num_output() const override;
	bool input_connection_changed() override;
	void execute() override;
	void init() override;
	void placed() override;
	void state_reset() override;

	void calculate_gauss();

	class Handle : public Operator::Handle {
		void mousePressEvent(QGraphicsSceneMouseEvent *);
	public:
		enum class Type {
			first_axis,
			second_axis,
			move
		} type;
		Handle(Type type, const char *tooltip, Operator *parent);
	};

	Handle *handle1;
	Handle *handle2;
	Handle *handle3;
	Handle::Type move_type;
	QPointF clicked_pos;
	QPointF clicked_offset;
	QPointF center;
	double e1_old, e2_old, angle_old;
	double scale;
	bool dont_accumulate_undo;

	void clear();

	void place_handles();
	void show_handles();
	void hide_handles();
	void restore_handles() override;
	void drag_handle(const QPointF &, Qt::KeyboardModifiers) override;

	std::array<double,3> calculate_tensor() const;	// Returns fxx, fyy, fxy
public:
	inline static constexpr const char *icon = ":/icons/gauss.svg";
	inline static constexpr const char *tooltip = "Add Gaussian";

	using OperatorTemplate::OperatorTemplate;
	void clicked_handle(QGraphicsSceneMouseEvent *, Handle::Type type);
private:
	friend class Operator;
	template<size_t n> void calculate();
};

#endif
