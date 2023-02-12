// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_POLYGON_HPP
#define OPERATOR_POLYGON_HPP

#include "operator.hpp"
#include "svg_cache.hpp"

#include <QImage>
#include <QTransform>
#include <QSvgRenderer>
#include <QGraphicsSvgItem>

enum class OperatorPolygonDrawMode {
	fill,
	dots,
	line
};

class OperatorPolygonState final : public Operator::StateTemplate<OperatorPolygonState> {
	QJsonObject to_json() const override;
	void from_json(const QJsonObject &) override;
public:
	size_t mode = 4;	// Number of vertices, 0 = circle
	OperatorPolygonDrawMode draw_mode = OperatorPolygonDrawMode::fill;
	int width = 0;
	int height = 0;
	QPoint offset;
	double rotation = 0.0;
};

class OperatorPolygon : public OperatorTemplate<OperatorId::Polygon, OperatorPolygonState, 0, 1>
{
	QImage image;

	void init() override;

	QPolygonF poly;
	QTransform trans;

	class Arrow : public QGraphicsSvgItem {
		void hoverEnterEvent(QGraphicsSceneHoverEvent *) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *) override;
		void mousePressEvent(QGraphicsSceneMouseEvent *) override;

		OperatorPolygon &op;
		QTransform translation_base;
		QPointF pos;
	public:
		enum class Type {
			width,
			height,
			rotation,
			move
		} type;
		double base_angle;
		QPointF offset;

		QSvgRenderer &svg;
		QSvgRenderer &svg_highlighted;

		Arrow(SVGCache::Id svg_id,
		      double angle,
		      double offset_x,
		      double offset_y,
		      const QPointF &pos,
		      Type type,
		      OperatorPolygon *parent);
		void set_transformation(const QTransform &transform, double angle);
	};

	// The 8 arrows:
	// 0) top 1) bottom 2) left 3) right 4) top_right 5) bottom_right 6) bottom_left 7) top_left 8) move
	std::array<Arrow *, 9> arrows;

	MenuButton *draw_mode_menu;

	// Note: place_arrows() must be called after paint_polygon(),
	// because the former calculates the transformation matrix
	void make_polygon();
	void paint_polygon();
	void place_arrows();
	void show_arrows();
	void hide_arrows();

	// Turn scene coordinates into local coordinates
	QPointF scene_to_local(const QPointF &) const;
	double scene_to_angle(const QPointF &) const;
	double scene_to_horizontal_dist(const QPointF &) const;
	double scene_to_vertical_dist(const QPointF &) const;
	QPoint scene_to_pos(const QPointF &p) const;

	Arrow::Type move_type;
	double original_value;
	double original_value_alt;	// height if width is changed and vice-versa
	double clicked_value;
	QPoint original_offset;
	QPoint clicked_offset;
	bool dont_accumulate_undo;

	// Update FFT-buffer and children
	void update_buffer();
	void placed() override;
	void state_reset() override;

	void set_mode(size_t mode);
	void set_draw_mode(OperatorPolygonDrawMode draw_mode);
	void clear();
	void invert();

	void restore_handles() override;
	void drag_handle(const QPointF &, Qt::KeyboardModifiers) override;
public:
	inline static constexpr const char *icon = ":/icons/poly_4.svg";
	inline static constexpr const char *tooltip = "Add Polygon";

	using OperatorTemplate::OperatorTemplate;
	void clicked_arrow(QGraphicsSceneMouseEvent *, Arrow::Type);
private:
	friend class Operator;
	friend Arrow;
	template<size_t N> void calculate();
};

#endif
