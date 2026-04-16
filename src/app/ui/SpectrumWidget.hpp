#pragma once

#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLogValueAxis>
#include <QLabel>
#include <complex>
#include <vector>

/**
 * \brief Live spectral analysis tab.
 *
 * Maintains a ring buffer of the most recent N samples of T_c,out and on
 * every Kth incoming sample computes a Hann-windowed DFT (radix-2 FFT).
 * The resulting magnitude spectrum is drawn as a line chart in Hz, with a
 * ruby marker placed at the dominant non-DC peak — the period shown in
 * the corner label is 1/f_peak.  This pairs naturally with the relay
 * auto-tuner: when the relay experiment is running, the peak of the
 * spectrum IS the ultimate period P_u.
 */
class SpectrumWidget : public QWidget {
  Q_OBJECT
 public:
  explicit SpectrumWidget(QWidget *parent = nullptr);

  /** Push the latest T_c,out sample.  Time is used to estimate dt. */
  void addSample(double t, double Tc_out);

  /** Throw away the buffer and blank the chart. */
  void clear();

  /** Number of samples held in the sliding window (must be a power of two). */
  int  windowSize() const { return static_cast<int>(buf_.size()); }

 private:
  void recomputeSpectrum();
  static void fftRadix2(std::vector<std::complex<double>> &a);

  // Ring buffer of the most recent N samples.
  std::vector<double> buf_;
  int writeIdx_{0};
  int filled_{0};
  double lastT_{0.0};
  double lastDt_{1.0};  // latest estimate of the sampling interval
  int stepsSinceFft_{0};

  QChart *chart_{};
  QChartView *view_{};
  QLineSeries *spectrum_{};
  QLineSeries *peakMarker_{};
  QValueAxis *axisX_{};
  QLogValueAxis *axisY_{};
  QLabel *lblPeak_{};
};
