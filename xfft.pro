# SPDX-License-Identifier: GPL-2.0
#QMAKE_CXX = clang++
#QMAKE_CXXFLAGS	+= -std=c++20 -g -march=native

QMAKE_CXX = g++
QMAKE_CXXFLAGS	+= -pedantic -std=c++20 -g -march=native

QT += widgets
QT += svg

HEADERS		= mainwindow.hpp \
		  scene.hpp \
		  mode.hpp \
		  operator.hpp \
		  operator_factory.hpp \
		  operator_adder.hpp \
		  operator_fft.hpp \
		  operator_convolution.hpp \
		  operator_split.hpp \
		  operator_merge.hpp \
		  operator_modulate.hpp \
		  operator_sum.hpp \
		  operator_mult.hpp \
		  operator_pow.hpp \
		  operator_powder.hpp \
		  operator_inversion.hpp \
		  operator_pixmap.hpp \
		  operator_polygon.hpp \
		  operator_gauss.hpp \
		  operator_lattice.hpp \
		  operator_wave.hpp \
		  operator_view.hpp \
		  operator_const.hpp \
		  operator_list.hpp \
		  selectable.hpp \
		  selection.hpp \
		  connector.hpp \
		  connector_pos.hpp \
		  edge.hpp \
		  edge_cycle.hpp \
		  view_connection.hpp \
		  topological_order.hpp \
		  document.hpp \
		  globals.hpp \
		  fft_buf.hpp \
		  fft_plan.hpp \
		  convolution_plan.hpp \
		  magnifier.hpp \
		  color.hpp \
		  extremes.hpp \
		  basis_vector.hpp \
		  svg_cache.hpp \
		  command.hpp

SOURCES		= main.cpp \
		  mainwindow.cpp \
		  scene.cpp \
		  operator.cpp \
		  operator_factory.cpp \
		  operator_adder.cpp \
		  operator_fft.cpp \
		  operator_convolution.cpp \
		  operator_split.cpp \
		  operator_merge.cpp \
		  operator_modulate.cpp \
		  operator_sum.cpp \
		  operator_mult.cpp \
		  operator_pow.cpp \
		  operator_powder.cpp \
		  operator_inversion.cpp \
		  operator_pixmap.cpp \
		  operator_polygon.cpp \
		  operator_gauss.cpp \
		  operator_lattice.cpp \
		  operator_wave.cpp \
		  operator_view.cpp \
		  operator_const.cpp \
		  operator_list.cpp \
		  selectable.cpp \
		  selection.cpp \
		  connector.cpp \
		  connector_pos.cpp \
		  edge.cpp \
		  edge_cycle.cpp \
		  view_connection.cpp \
		  topological_order.cpp \
		  document.cpp \
		  globals.cpp \
		  fft_buf.cpp \
		  fft_plan.cpp \
		  convolution_plan.cpp \
		  magnifier.cpp \
		  color.cpp \
		  extremes.cpp \
		  basis_vector.cpp \
		  svg_cache.cpp \
		  command.cpp

RESOURCES	= xfft.qrc

# For benchmarking: link with boost_timer
#LIBS		+= -lfftw3 -lboost_timer

LIBS		+= -lfftw3
