#include "SpectrumWidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPen>
#include <QColor>
#include <QFont>
#include <QtCharts/QScatterSeries>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
constexpr int   kWindow        = 512;   // FFT window (must be power of 2)
constexpr int   kRecomputeEvery = 16;   // recompute spectrum every N samples
constexpr double kMinFilled    = 0.5;   // need 50% buffer before first FFT
} // namespace

SpectrumWidget::SpectrumWidget(QWidget *parent)
  : QWidget(parent), buf_(kWindow, 0.0) {

  chart_ = new QChart();
  chart_->setTitle(
      "Live spectrum of T<sub>c,out</sub>  |  Hann window, FFT N=512");
  chart_->setTitleBrush(QColor("#2c3e50"));
  chart_->legend()->setVisible(false);
  chart_->setBackgroundBrush(QColor("#fafbfc"));
  chart_->setMargins(QMargins(6, 6, 6, 6));

  spectrum_ = new QLineSeries(this);
  spectrum_->setName("|X(f)|");
  QPen pen(QColor("#2c5784"));
  pen.setWidth(2);
  spectrum_->setPen(pen);
  chart_->addSeries(spectrum_);

  peakMarker_ = new QLineSeries(this);
  QPen markerPen(QColor("#c0392b"));
  markerPen.setWidth(2);
  markerPen.setStyle(Qt::DashLine);
  peakMarker_->setPen(markerPen);
  chart_->addSeries(peakMarker_);

  axisX_ = new QValueAxis();
  axisX_->setTitleText("Frequency f [Hz]");
  axisX_->setLabelFormat("%.3f");
  axisX_->setRange(0.0, 0.5);
  chart_->addAxis(axisX_, Qt::AlignBottom);
  spectrum_->attachAxis(axisX_);
  peakMarker_->attachAxis(axisX_);

  axisY_ = new QLogValueAxis();
  axisY_->setTitleText("Magnitude  |X(f)|  (log scale)");
  axisY_->setLabelFormat("%.2e");
  axisY_->setBase(10.0);
  axisY_->setRange(1e-4, 1e2);
  chart_->addAxis(axisY_, Qt::AlignLeft);
  spectrum_->attachAxis(axisY_);
  peakMarker_->attachAxis(axisY_);

  view_ = new QChartView(chart_, this);
  view_->setRenderHint(QPainter::Antialiasing, true);

  lblPeak_ = new QLabel(
      "Peak: — Hz   (P<sub>u</sub> ≈ — s)   buffer: 0 / " + QString::number(kWindow),
      this);
  QFont f = lblPeak_->font();
  f.setPointSize(f.pointSize() + 1);
  f.setBold(true);
  lblPeak_->setFont(f);
  lblPeak_->setStyleSheet("color:#2c3e50;padding:6px;background:#ecf0f1;border-radius:3px;");

  auto *top = new QHBoxLayout();
  top->addWidget(lblPeak_, 1);

  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(4, 4, 4, 4);
  root->addLayout(top);
  root->addWidget(view_, 1);
}

void SpectrumWidget::clear() {
  std::fill(buf_.begin(), buf_.end(), 0.0);
  writeIdx_ = 0;
  filled_ = 0;
  stepsSinceFft_ = 0;
  spectrum_->clear();
  peakMarker_->clear();
  lblPeak_->setText(
      "Peak: — Hz   (P<sub>u</sub> ≈ — s)   buffer: 0 / " + QString::number(kWindow));
}

void SpectrumWidget::addSample(double t, double Tc_out) {
  // Estimate sample interval from successive arrival times.  Use the
  // running estimate (not an average) so step-changes in dt are tracked
  // instantly.
  if (filled_ > 0) {
    const double dt = t - lastT_;
    if (dt > 1e-6) lastDt_ = dt;
  }
  lastT_ = t;

  buf_[static_cast<size_t>(writeIdx_)] = Tc_out;
  writeIdx_ = (writeIdx_ + 1) % kWindow;
  if (filled_ < kWindow) ++filled_;
  ++stepsSinceFft_;

  if (stepsSinceFft_ >= kRecomputeEvery &&
      filled_ >= static_cast<int>(kMinFilled * kWindow)) {
    recomputeSpectrum();
    stepsSinceFft_ = 0;
  }
}

void SpectrumWidget::recomputeSpectrum() {
  const int N = kWindow;

  // Build the windowed input in time order (oldest → newest).  The ring
  // buffer oldest entry sits at writeIdx_ when fully filled; before it's
  // full we just start from index 0.
  std::vector<std::complex<double>> x(static_cast<size_t>(N));

  // Compute mean over filled portion — subtracting it kills the huge DC
  // spike that would otherwise dominate the log plot.
  double mean = 0.0;
  for (int i = 0; i < filled_; ++i) mean += buf_[static_cast<size_t>(i)];
  mean /= std::max(1, filled_);

  for (int i = 0; i < N; ++i) {
    const int src = (filled_ == N)
                      ? (writeIdx_ + i) % N
                      : i;  // not yet wrapped
    const double raw = (src < filled_)
                         ? (buf_[static_cast<size_t>(src)] - mean)
                         : 0.0;    // zero-pad until full
    // Hann window: w[i] = 0.5 * (1 - cos(2π i / (N-1)))
    const double w = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (N - 1)));
    x[static_cast<size_t>(i)] = std::complex<double>(raw * w, 0.0);
  }

  fftRadix2(x);

  // Sample rate and frequency axis.
  const double fs = 1.0 / std::max(1e-6, lastDt_);
  const double df = fs / N;

  // Keep only the first half (real input → symmetric spectrum).
  const int half = N / 2;

  // Build the full point list first, then push it to the series in one
  // replace() call.  clear()+append() in a loop fires 256 pointAdded signals
  // per FFT (every 16 samples) which forces the chart to re-lay out the
  // axis ticks repeatedly — this was a major UI-thread stall at high speed.
  QList<QPointF> pts;
  pts.reserve(half);
  double maxMag = 0.0;
  int    peakBin = 1;   // skip DC
  for (int k = 0; k < half; ++k) {
    const double mag = std::abs(x[static_cast<size_t>(k)]) * 2.0 / N;
    const double f   = k * df;
    pts.append(QPointF(f, std::max(mag, 1e-6)));
    if (k >= 1 && mag > maxMag) { maxMag = mag; peakBin = k; }
  }
  spectrum_->replace(pts);

  // Adjust axis ranges to fit current data.
  axisX_->setRange(0.0, std::max(0.01, half * df));
  const double topDecade = std::pow(10.0, std::ceil(std::log10(std::max(maxMag, 1e-3))));
  axisY_->setRange(std::max(1e-6, topDecade * 1e-5), std::max(1e-3, topDecade * 2.0));

  // Drop a dashed vertical marker at the peak frequency (two-point batch).
  const double fPeak = peakBin * df;
  QList<QPointF> markerPts;
  markerPts.reserve(2);
  markerPts.append(QPointF(fPeak, axisY_->min()));
  markerPts.append(QPointF(fPeak, axisY_->max()));
  peakMarker_->replace(markerPts);

  // Update the summary label.
  const double Pu = (fPeak > 1e-9) ? 1.0 / fPeak : 0.0;
  lblPeak_->setText(QStringLiteral(
      "Peak: <b>%1 Hz</b>   (P<sub>u</sub> &asymp; <b>%2 s</b>)   "
      "magnitude: %3 K   buffer: %4 / %5   f<sub>s</sub>: %6 Hz")
        .arg(fPeak, 0, 'g', 4)
        .arg(Pu, 0, 'f', 1)
        .arg(maxMag, 0, 'f', 4)
        .arg(filled_).arg(kWindow)
        .arg(fs, 0, 'g', 3));
}

// Iterative radix-2 Cooley-Tukey FFT (in-place, N must be a power of 2).
void SpectrumWidget::fftRadix2(std::vector<std::complex<double>> &a) {
  const int n = static_cast<int>(a.size());
  if (n <= 1) return;

  // Bit-reverse permutation.
  for (int i = 1, j = 0; i < n; ++i) {
    int bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) std::swap(a[static_cast<size_t>(i)], a[static_cast<size_t>(j)]);
  }

  // Butterfly stages.
  for (int len = 2; len <= n; len <<= 1) {
    const double ang = -2.0 * M_PI / len;
    const std::complex<double> wlen(std::cos(ang), std::sin(ang));
    for (int i = 0; i < n; i += len) {
      std::complex<double> w(1.0, 0.0);
      for (int k = 0; k < len / 2; ++k) {
        const auto u = a[static_cast<size_t>(i + k)];
        const auto v = a[static_cast<size_t>(i + k + len / 2)] * w;
        a[static_cast<size_t>(i + k)]             = u + v;
        a[static_cast<size_t>(i + k + len / 2)]   = u - v;
        w *= wlen;
      }
    }
  }
}
