#include <math.h>
#include <cstring>
#include <cstdio>
#include <complex>
#include "cplxfft.hpp"
#include "spectrum.h"

#define SPECTRUM_AVERAGE 4
#define SOFTWARE_GAIN 30

using std::complex;

typedef complex<double> Complex;

namespace {
	struct ComplexArray {
		Complex** array_;
		int h_;
		int w_;
		ComplexArray(Complex** array, int h, int w) : array_(array), h_(h), w_(w) {}
	};

	ComplexArray* createArray(int h, int w) {
		Complex** array = new Complex*[w];
		for (int i = 0; i < w; i++) {
			array[i] = new Complex[h];
		}
		return new ComplexArray(array, h, w);
	}

	void freeArray(ComplexArray* ca) {
		for (int i = 0; i < ca->w_; i++) {
			delete[] ca->array_[i];
		}
		delete[] ca->array_;
		delete ca;
	}
}

class Spectrum {
public:
	Spectrum(int N) 
		: calculator_(N), complexArray_(::createArray(N, SPECTRUM_AVERAGE))  {
	}

	virtual ~Spectrum() {
		::freeArray(complexArray_);
	}

	ComplexArray* getComplexArray() { return complexArray_; }

	void fft(Complex* a) { calculator_.fft(a); }

	int length() { calculator_.length(); }

private:
	cfft<Complex> calculator_;
	ComplexArray* complexArray_;

};


EXTERNC spectrum_handle init_spectrum(int N) {
	return new Spectrum(N);
}

EXTERNC int calculate_spectrum(spectrum_handle handle, cmplx* src, int src_len, double* dst, int dst_len) {
	Spectrum* p = reinterpret_cast<Spectrum*>(handle);
	int fft_points = p->length();
	int point_compression = fft_points / dst_len;
	ComplexArray* spectrum = p->getComplexArray();
	int idx = 0;
	int fft_idx = 0;
	for (int i = 0; i < SPECTRUM_AVERAGE; i++) {
		Complex* c = spectrum->array_[i];
		for (fft_idx = 0; (idx < src_len) && (fft_idx < fft_points); idx++, fft_idx++) {
			c[fft_idx].real((((double)src[idx].re) - 128));
			c[fft_idx].imag((((double)src[idx].im) - 128));
		}
		p->fft(c);
	}
	for (idx = 0; idx < p->length(); idx+=point_compression) {
		double magnitude = 0;
		for (int i = 0; i < SPECTRUM_AVERAGE; i++) {
			for (int j = 0; j < point_compression; j++) {
				int spectrum_idx = (idx + j + fft_points/2) % fft_points;
				spectrum_idx = spectrum_idx == 0 ? 1 : spectrum_idx;
				magnitude += 20*log10(abs(spectrum->array_[i][spectrum_idx]));
			}
		}
		magnitude /= (SPECTRUM_AVERAGE*point_compression);
		magnitude += SOFTWARE_GAIN;
		dst[idx/point_compression] = magnitude;
	}
	return 0;
}

EXTERNC void free_spectrum(spectrum_handle handle) {
	delete reinterpret_cast<Spectrum*>(handle);
}
