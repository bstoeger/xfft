// SPDX-License-Identifier: GPL-2.0
#ifndef MAGNIFIER_HPP
#define MAGNIFIER_HPP

#include <QPixmap>
#include <QLabel>

class Scene;
class Magnifier : public QLabel {
	static constexpr size_t	size = 50;
	static constexpr size_t factor = 4;
	static constexpr size_t total_size = size * factor;
	static constexpr size_t half_size = total_size / 2;

	QPixmap stamp;
public:
	Magnifier(QWidget *parent);
	void go(Scene *scene, const QPointF &scene_pos);
};

#endif
