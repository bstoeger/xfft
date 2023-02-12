// SPDX-License-Identifier: GPL-2.0
#include "operator_inversion.hpp"
#include "document.hpp"

bool OperatorInversion::input_connection_changed()
{
	// Empty if the input buffer is empty.
	if (input_connectors[0]->is_empty_buffer())
		return make_output_empty(0);

	FFTBuf &buf = input_connectors[0]->get_buffer();
	if (buf.is_complex())
		return make_output_complex(0);
	else
		return make_output_real(0);
}

const char *OperatorInversion::get_pixmap_name(OperatorInversionType type)
{
	switch (type) {
	default:
	case OperatorInversionType::inversion: return ":icons/inversion.svg";
	case OperatorInversionType::rot_4_plus: return ":icons/4+.svg";
	case OperatorInversionType::rot_4_minus: return ":icons/4-.svg";
	case OperatorInversionType::m_x: return ":icons/m_x.svg";
	case OperatorInversionType::m_y: return ":icons/m_y.svg";
	case OperatorInversionType::m_xy: return ":icons/m_xy.svg";
	case OperatorInversionType::m_minus_xy: return ":icons/m_-xy.svg";
	}
}

const char *OperatorInversion::get_tooltip(OperatorInversionType type)
{
	switch (type) {
	default:
	case OperatorInversionType::inversion: return "Inversion (twofold rotation)";
	case OperatorInversionType::rot_4_plus: return "Fourfold rotation ccw";
	case OperatorInversionType::rot_4_minus: return "Fourfold rotation ccw";
	case OperatorInversionType::m_x: return "Reflection at x=0";
	case OperatorInversionType::m_y: return "Reflection at y=0";
	case OperatorInversionType::m_xy: return "Reflection at x=y";
	case OperatorInversionType::m_minus_xy: return "Reflection at x=-y";
	}
}

QPixmap OperatorInversion::get_pixmap(OperatorInversionType type, int size)
{
	const char *name = get_pixmap_name(type);
	return QIcon(name).pixmap(QSize(size, size));
}

void OperatorInversion::init()
{
	setPixmap(get_pixmap(state.type, simple_size));

	menu = new MenuButton(Side::left, "Set transformation type", this);
	menu->add_entry(get_pixmap(OperatorInversionType::inversion, default_button_height),
			get_tooltip(OperatorInversionType::inversion), [this](){ set_type(OperatorInversionType::inversion); });
	menu->add_entry(get_pixmap(OperatorInversionType::rot_4_plus, default_button_height),
			get_tooltip(OperatorInversionType::rot_4_plus), [this](){ set_type(OperatorInversionType::rot_4_plus); });
	menu->add_entry(get_pixmap(OperatorInversionType::rot_4_minus, default_button_height),
			get_tooltip(OperatorInversionType::rot_4_minus), [this](){ set_type(OperatorInversionType::rot_4_minus); });
	menu->add_entry(get_pixmap(OperatorInversionType::m_x, default_button_height),
			get_tooltip(OperatorInversionType::m_x), [this](){ set_type(OperatorInversionType::m_x); });
	menu->add_entry(get_pixmap(OperatorInversionType::m_y, default_button_height),
			get_tooltip(OperatorInversionType::m_y), [this](){ set_type(OperatorInversionType::m_y); });
	menu->add_entry(get_pixmap(OperatorInversionType::m_xy, default_button_height),
			get_tooltip(OperatorInversionType::m_xy), [this](){ set_type(OperatorInversionType::m_xy); });
	menu->add_entry(get_pixmap(OperatorInversionType::m_minus_xy, default_button_height),
			get_tooltip(OperatorInversionType::m_minus_xy), [this](){ set_type(OperatorInversionType::m_minus_xy); });
	menu->set_pixmap((int)state.type);
}

Operator::InitState OperatorInversion::make_init_state(OperatorInversionType type)
{
	auto state = std::make_unique<OperatorInversionState>();
	state->type = type;
	return {
		get_pixmap_name(type),
		get_tooltip(type),
		std::move(state)
	};
}

std::vector<Operator::InitState> OperatorInversion::get_init_states()
{
	std::vector<Operator::InitState> res;
	res.push_back(make_init_state(OperatorInversionType::inversion));
	res.push_back(make_init_state(OperatorInversionType::rot_4_plus));
	res.push_back(make_init_state(OperatorInversionType::rot_4_minus));
	res.push_back(make_init_state(OperatorInversionType::m_x));
	res.push_back(make_init_state(OperatorInversionType::m_y));
	res.push_back(make_init_state(OperatorInversionType::m_xy));
	res.push_back(make_init_state(OperatorInversionType::m_minus_xy));
	return res;
}

QJsonObject OperatorInversionState::to_json() const
{
	QJsonObject res;
	res["type"] = static_cast<int>(type);
	return res;
}

void OperatorInversionState::from_json(const QJsonObject &desc)
{
	type = static_cast<OperatorInversionType>(desc["type"].toInt());
}

void OperatorInversion::state_reset()
{
	menu->set_pixmap((int)state.type);
	setPixmap(get_pixmap(state.type, simple_size));

	execute();

	// Execute children
	execute_topo();
}

void OperatorInversion::set_type(OperatorInversionType t)
{
	if (state.type == t)
		return;
	auto new_state = clone_state();
	new_state->type = t;
	place_set_state_command("Set symmetry type", std::move(new_state), false);
}

template <typename T, size_t N, int DX, int DY>
static void reflect(FFTBuf &in_buf, FFTBuf &out_buf)
{
	T *in = in_buf.get_data<T>();
	T *out = out_buf.get_data<T>();
	if (DX < 0)
		out += N - 1;
	if (DY < 0)
		out += N * (N - 1);
	for (size_t y = 0; y < N; ++y) {
		for (size_t x = 0; x < N; ++x) {
			*out = *in++;
			out += DX;
		}
		out -= N * DX;
		out += N * DY;
	}
}

template <typename T, size_t N, int DX, int DY>
static void rotate(FFTBuf &in_buf, FFTBuf &out_buf)
{
	T *in = in_buf.get_data<T>();
	T *out = out_buf.get_data<T>();
	if (DX < 0)
		out += N * (N - 1);
	if (DY < 0)
		out += N - 1;
	for (size_t y = 0; y < N; ++y) {
		for (size_t x = 0; x < N; ++x) {
			*out = *in++;
			out += N * DX;
		}
		out -= N * N * DX;
		out += DY;
	}
}

template <typename T, size_t N>
void OperatorInversion::transform(FFTBuf &in_buf, FFTBuf &out_buf)
{
	switch (state.type) {
	default:
	case OperatorInversionType::inversion: return reflect<T,N,-1,-1>(in_buf, out_buf);
	case OperatorInversionType::rot_4_plus: return rotate<T,N,1,1>(in_buf, out_buf);
	case OperatorInversionType::rot_4_minus: return rotate<T,N,-1,-1>(in_buf, out_buf);
	case OperatorInversionType::m_x: return reflect<T,N,-1,1>(in_buf, out_buf);
	case OperatorInversionType::m_y: return reflect<T,N,1,-1>(in_buf, out_buf);
	case OperatorInversionType::m_xy: return rotate<T,N,-1,1>(in_buf, out_buf);
	case OperatorInversionType::m_minus_xy: return rotate<T,N,1,-1>(in_buf, out_buf);
	}
}

template<size_t N>
void OperatorInversion::calculate()
{
	FFTBuf &buf = input_connectors[0]->get_buffer();
	FFTBuf &out = output_buffers[0];

	if (buf.is_complex())
		transform<std::complex<double>, N>(buf, out);
	else
		transform<double, N>(buf, out);

	output_buffers[0].set_extremes(buf.get_extremes());
}

void OperatorInversion::execute()
{
	if (input_connectors[0]->is_empty_buffer())
		return; // Empty -> nothing to do

	dispatch_calculate(*this);
}
