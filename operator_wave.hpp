// SPDX-License-Identifier: GPL-2.0
#ifndef OPERATOR_WAVE_HPP
#define OPERATOR_WAVE_HPP

#include "operator.hpp"

#include <QImage>

class BasisVector;

enum class OperatorWaveMode {
	mag_phase,		// Magnitude/Phase
	long_trans		// Longitudinal/Transversal
};

class OperatorWaveState final : public Operator::StateTemplate<OperatorWaveState> {
	QJsonObject to_json() const override;
	void from_json(const QJsonObject &) override;
public:
	OperatorWaveMode mode = OperatorWaveMode::mag_phase;
	QPoint h { 10, 0 };		// Wave vector h
	double amplitude_mag = 1.0;	// Amplitude of magnitudes
	double amplitude_phase = 0.0;	// Amplitude of phase
};

class OperatorWave : public OperatorTemplate<OperatorId::Wave, OperatorWaveState, 0, 1>
{
	AlignedBuf<uint32_t> imagebuf;

	class Handle : public Operator::Handle {
		void mousePressEvent(QGraphicsSceneMouseEvent *);
	public:
		Handle(const char *tooltip, Operator *parent);
	};

	Handle *handle;
	QPointF clicked_pos;
	QPointF clicked_old_pos;
	bool dont_accumulate_undo;

	BasisVector *basis;

	// Update FFT-buffer and children
	void update_buffer();
	void placed() override;
	void state_reset() override;

	void clear();

	void paint_wave();
	void paint_basis();

	void place_handle();
	void show_handle();
	void hide_handle();
	void restore_handles() override;
	void drag_handle(const QPointF &, Qt::KeyboardModifiers) override;

	template <size_t N>
	void paint_quadrant_mag_phase(uint32_t *out, std::complex<double> *data, int start_x, int start_y,
				      double max_mag, double max_phase, double max);
	template <size_t N>
	void paint_quadrant_long_trans(uint32_t *out, std::complex<double> *data, int start_x, int start_y,
				       double max_re, double max_im, double max);

	// Switch between modulation modes
	MenuButton *mode_menu;
	void switch_mode(OperatorWaveMode mode);

	// Set amplitudes
	Scroller *scroller_mag;
	Scroller *scroller_phase;
	void set_amplitude_mag(double v);
	void set_amplitude_phase(double v);
	void set_scrollers();

	static constexpr double max_amplitude = 20.0;

public:
	inline static constexpr const char *icon = ":/icons/wave.svg";
	inline static constexpr const char *tooltip = "Add plane wave";

	using OperatorTemplate::OperatorTemplate;

	void clicked_handle(QGraphicsSceneMouseEvent *);
	void init() override;
private:
	friend class Operator;
	template <size_t N> void calculate();
};

#endif
