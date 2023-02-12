// SPDX-License-Identifier: GPL-2.0
#include "operator_pow.hpp"
#include "document.hpp"

bool OperatorPow::input_connection_changed()
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

static const char *get_pixmap_name(int exponent)
{
	switch (exponent) {
	case -3: return ":icons/pow_-3.svg";
	case -2: return ":icons/pow_-2.svg";
	case -1: return ":icons/pow_-1.svg";
	case 2: return ":icons/pow_2.svg";
	case 3: return ":icons/pow_3.svg";
	default: return "";
	}
}

static int get_pixmap_id(int exponent)
{
	switch (exponent) {
	default:
	case -3: return 0;
	case -2: return 1;
	case -1: return -1;
	case 2: return 2;
	case 3: return 3;
	}
}

static double get_exponent(int exponent)
{
	switch (exponent) {
	case -3: return 1.0 / 3.0;
	case -2: return 1.0 / 2.0;
	case -1: return -1;
	case 2: return 2.0;
	case 3: return 3.0;
	default: return 1.0;
	};
}

QPixmap OperatorPow::get_pixmap(int exponent, int size)
{
	const char *name = get_pixmap_name(exponent);
	return QIcon(name).pixmap(QSize(size, size));
}

const char *OperatorPow::get_tooltip(int exponent)
{
	switch (exponent) {
	default:
	case -3: return "cube root";
	case -2: return "square root";
	case -1: return "inverse";
	case 2: return "square";
	case 3: return "cube";
	}
}

void OperatorPow::init()
{
	setPixmap(get_pixmap(state.exponent, simple_size));

	menu = new MenuButton(Side::left, "Set exponent", this);
	menu->add_entry(get_pixmap(-3, default_button_height), get_tooltip(-3), [this](){ set_exponent(-3); });
	menu->add_entry(get_pixmap(-2, default_button_height), get_tooltip(-2), [this](){ set_exponent(-2); });
	menu->add_entry(get_pixmap(2, default_button_height), get_tooltip(2), [this](){ set_exponent(2); });
	menu->add_entry(get_pixmap(3, default_button_height), get_tooltip(3), [this](){ set_exponent(3); });
	menu->add_entry(get_pixmap(-1, default_button_height), get_tooltip(-1), [this](){ set_exponent(-1); });
}

QJsonObject OperatorPowState::to_json() const
{
	QJsonObject res;
	res["exponent"] = exponent;
	return res;
}

void OperatorPowState::from_json(const QJsonObject &desc)
{
	exponent = desc["exponent"].toInt();
}

void OperatorPow::state_reset()
{
	menu->set_pixmap(get_pixmap_id(state.exponent));
	setPixmap(get_pixmap(state.exponent, simple_size));
	execute();

	// Execute children
	execute_topo();
}

Operator::InitState OperatorPow::make_init_state(int exponent)
{
	auto state = std::make_unique<OperatorPowState>();
	state->exponent = exponent;
	return {
		get_pixmap_name(exponent),
		get_tooltip(exponent),
		std::move(state)
	};
}

std::vector<Operator::InitState> OperatorPow::get_init_states()
{
	std::vector<Operator::InitState> res;
	res.push_back(make_init_state(-3));
	res.push_back(make_init_state(-2));
	res.push_back(make_init_state(2));
	res.push_back(make_init_state(3));
	res.push_back(make_init_state(-1));
	return res;
}

void OperatorPow::set_exponent(int v)
{
	if (state.exponent == v)
		return;
	auto new_state = clone_state();
	new_state->exponent = v;
	place_set_state_command("Set power function", std::move(new_state), false);
}

static const constexpr double inverse_min = 0.000001;

template <typename T, size_t N>
static double inverse_doit(FFTBuf &in_buf, FFTBuf &out_buf)
{
	T *in = in_buf.get_data<T>();
	T *out = out_buf.get_data<T>();
	double max_norm = 0.0;
	for (size_t i = 0; i < N*N; ++i) {
		T x = *in++;
		x = std::abs(x) < inverse_min ? 1.0 / inverse_min : 1.0 / x;
		double norm = std::norm(x);
		if (norm > max_norm)
			max_norm = norm;
		*out++ = x;
	}
	return max_norm;
}

template <typename T, size_t N>
static void pow_doit(FFTBuf &in_buf, FFTBuf &out_buf, double exponent)
{
	T *in = in_buf.get_data<T>();
	T *out = out_buf.get_data<T>();
	for (size_t i = 0; i < N*N; ++i)
		*out++ = pow(*in++, exponent);
}

template<size_t N>
void OperatorPow::calculate()
{
	FFTBuf &buf = input_connectors[0]->get_buffer();
	FFTBuf &out = output_buffers[0];

	// Hardcode the inverse instead of using the pow() function
	double max_norm;
	if (state.exponent == -1) {
		if (buf.is_complex())
			max_norm = inverse_doit<std::complex<double>, N>(buf, out);
		else
			max_norm = inverse_doit<double, N>(buf, out);
	} else {
		double exponent = get_exponent(state.exponent);
		if (buf.is_complex())
			pow_doit<std::complex<double>, N>(buf, out, exponent);
		else
			pow_doit<double, N>(buf, out, exponent);

		max_norm = pow(buf.get_extremes().get_max_norm(), exponent);
	}
	output_buffers[0].set_extremes(Extremes(max_norm));

}

void OperatorPow::execute()
{
	if (input_connectors[0]->is_empty_buffer())
		return; // Empty -> nothing to do

	dispatch_calculate(*this);
}
