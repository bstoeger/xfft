// SPDX-License-Identifier: GPL-2.0
#include "operator_fft.hpp"
#include "document.hpp"

QJsonObject OperatorFFTState::to_json() const
{
	QJsonObject res;
	const char *id;
	switch (type) {
	case OperatorFFTType::FWD:
	default:
		id = "fwd";
		break;
	case OperatorFFTType::INV:
		id = "inv";
		break;
	case OperatorFFTType::NORM:
		id = "norm";
		break;
	}
	res["type"] = id;
	return res;
}

void OperatorFFTState::from_json(const QJsonObject &desc)
{
	QString s = desc["type"].toString();
	if (s == "inv")
		type = OperatorFFTType::INV;
	else if (s == "norm")
		type = OperatorFFTType::NORM;
	else
		type = OperatorFFTType::FWD;
}

static const char *get_pixmap_name(OperatorFFTType type)
{
	switch (type) {
	case OperatorFFTType::FWD:
		return ":/icons/fft.svg";
	case OperatorFFTType::INV:
		return ":/icons/fft_inv.svg";
	case OperatorFFTType::NORM:
		return ":/icons/fft_norm.svg";
	default:
		return "";
	}
}

static const char *get_tooltip(OperatorFFTType type)
{
	switch (type) {
	case OperatorFFTType::FWD: return "Fourier transform";
	case OperatorFFTType::INV: return "Inverse Fourier transform";
	case OperatorFFTType::NORM: return "Norm of Fourier transform";
	default:
		return "";
	}
}
QPixmap OperatorFFT::get_pixmap(OperatorFFTType type, int size)
{
	const char *name = get_pixmap_name(type);
	return QIcon(name).pixmap(QSize(size, size));
}

void OperatorFFT::init()
{
	setPixmap(get_pixmap(state.type, simple_size));

	menu = new MenuButton(Side::left, "Change transformation type", this);
	menu->add_entry(get_pixmap(OperatorFFTType::FWD, default_button_height), get_tooltip(OperatorFFTType::FWD), [this](){ set_type(OperatorFFTType::FWD); });
	menu->add_entry(get_pixmap(OperatorFFTType::INV, default_button_height), get_tooltip(OperatorFFTType::INV), [this](){ set_type(OperatorFFTType::INV); });
	menu->add_entry(get_pixmap(OperatorFFTType::NORM, default_button_height), get_tooltip(OperatorFFTType::NORM), [this](){ set_type(OperatorFFTType::NORM); });
}

Operator::InitState OperatorFFT::make_init_state(OperatorFFTType type)
{
	auto state = std::make_unique<OperatorFFTState>();
	state->type = type;
	return {
		get_pixmap_name(type),
		get_tooltip(type),
		std::move(state)
	};
}

std::vector<Operator::InitState> OperatorFFT::get_init_states()
{
	std::vector<Operator::InitState> res;
	res.push_back(make_init_state(OperatorFFTType::FWD));
	res.push_back(make_init_state(OperatorFFTType::INV));
	res.push_back(make_init_state(OperatorFFTType::NORM));
	return res;
}

bool OperatorFFT::update_plan()
{
	if (input_connectors[0]->is_empty_buffer()) {
		plan.reset();
		return make_output_empty(0);
	}

	bool forward = true;
	bool norm = false;
	if (state.type == OperatorFFTType::INV)
		forward = false;
	else if (state.type == OperatorFFTType::NORM)
		norm = true;

	FFTBuf &new_buf = input_connectors[0]->get_buffer();
	bool updated_output = norm ?
		make_output_real(0) : make_output_complex(0);

	plan = std::make_unique<FFTPlan>(new_buf, output_buffers[0], forward, norm);
	return updated_output;
}

bool OperatorFFT::input_connection_changed()
{
	return update_plan();
}

void OperatorFFT::set_type(OperatorFFTType type)
{
	if (state.type == type)
		return;
	auto new_state = clone_state();
	new_state->type = type;
	place_set_state_command("Set FFT type", std::move(new_state), false);
}

void OperatorFFT::state_reset()
{
	menu->set_pixmap((int)state.type);
	setPixmap(get_pixmap(state.type, simple_size));
	if (update_plan())
		output_buffer_changed();
	execute();

	// Execute children
	execute_topo();
}

void OperatorFFT::execute()
{
	if (!plan)
		return;
	plan->execute();
}
