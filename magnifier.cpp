// SPDX-License-Identifier: GPL-2.0
#include "magnifier.hpp"
#include "scene.hpp"

#include <QPainter>

Magnifier::Magnifier(QWidget *parent)
	: QLabel(parent)
	, stamp(total_size, total_size)
{
	stamp.fill(Qt::transparent);
	QPainter painter(&stamp);
	painter.setBrush(Qt::SolidPattern);
	painter.drawEllipse(0, 0, total_size, total_size);

	setAttribute(Qt::WA_TransparentForMouseEvents, true);
	setFixedSize(QSize(total_size, total_size));
	setPixmap(QPixmap(total_size, total_size));
}

void Magnifier::go(Scene *scene, const QPointF &scene_pos)
{
	QGraphicsView *view = scene->get_view();

	// Generate zoomed bitmap
	QPoint relative_pos = view->mapFromScene(scene_pos);
	QRect unzoomed_rect(relative_pos - QPoint(size / 2, size / 2), QSize(size, size));
	QPixmap unzoomed = view->grab(unzoomed_rect);
	QPixmap zoomed = unzoomed.scaled(QSize(total_size, total_size), Qt::IgnoreAspectRatio, Qt::FastTransformation);

	// Stamp out sphere
	QPixmap stamped = stamp;
	QPainter painter(&stamped);
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	painter.drawPixmap(0, 0, zoomed, 0, 0, total_size, total_size);

	// Calculate position in parent widget
	relative_pos -= QPoint(half_size, half_size);
	QPoint window_pos = view->mapToParent(relative_pos);
	setPixmap(stamped);
	move(window_pos);
	setVisible(true);
}
