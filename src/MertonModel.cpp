#include "MertonModel.hpp"
#include <limits>
#include <stdexcept>
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

/*-----------------------------------------------------------------------------
-----------------Implementation Merton-Jump-Diffusion--------------------------
------------------------------------------------------------------------------*/

//Constructeurs----------------------------------------------------------------

MertonModel::MertonModel(double S0, double r, double sigma, double lambda,
double muJ, double sigmaJ, size_t nPath):
_S0(S0), _r(r), _sigma(sigma), _lambda(lambda), _muJ(muJ), _sigmaJ(sigmaJ), 
_nPath(nPath), _rng(std::random_device{}()) {
  if (_nPath == 0 || _nPath == std::numeric_limits<size_t>::max()) {
    throw std::invalid_argument("Le nombre de pas doit etre strictement positif et representable.");
  }
  setParameters({S0, r, sigma, lambda, muJ, sigmaJ});
}

MertonModel::MertonModel(const MertonModel& model) = default;

//Methodes----------------------------------------------------------------------

std::vector<double> MertonModel::simulatePath(double maturity) const {
  if (!std::isfinite(maturity) || maturity < 0.0) {
    throw std::invalid_argument("La maturite doit etre finie et positive ou nulle.");
  }
  if (maturity == 0.0) return std::vector<double>(_nPath + 1, _S0);
  std::vector<double> path(_nPath + 1);
  path[0] = _S0;

  double dt = maturity / static_cast<double>(_nPath);

  std::normal_distribution<> normal(0.0, 1.0);
  // Les distributions standard exigent une intensite et un ecart-type > 0.
  const double jumpIntensity = _lambda * dt;
  std::poisson_distribution<> poisson(jumpIntensity > 0.0 ? jumpIntensity : 1.0);
  std::lognormal_distribution<> lognormal(_muJ, _sigmaJ > 0.0 ? _sigmaJ : 1.0);

  for (size_t i = 1; i <= _nPath; ++i) {
    double Z = normal(_rng);
    size_t N = jumpIntensity > 0.0 ? static_cast<size_t>(poisson(_rng)) : 0;

    double jumpMultiplier = 1.0;
    for (size_t j = 0; j < N; ++j) {
      jumpMultiplier *= _sigmaJ > 0.0 ? lognormal(_rng) : std::exp(_muJ);
    }

    double drift = (_r - _lambda * _k - 0.5 * _sigma * _sigma) * dt;
    double diffusion = _sigma * std::sqrt(dt) * Z;

    path[i] = path[i - 1] * std::exp(drift + diffusion) * jumpMultiplier;
  }

  return path;
}

void MertonModel::plotSimulatedPath(double maturity) const {
  std::vector<double> path = simulatePath(maturity);
  std::vector<double> times(path.size());
  double dt = maturity / static_cast<double>(_nPath);

  for (size_t i = 0; i < path.size(); ++i) {
    times[i] = static_cast<double>(i) * dt;
  }

  plt::figure();
  plt::plot(times, path);
  plt::xlabel("Temps");
  plt::ylabel("Prix");
  plt::title("Merton Jump-Diffusion");
  plt::grid(true);
  plt::show();
}

double MertonModel::getRiskFreeRate() const { return _r; }

double MertonModel::getInitialSpot() const { return _S0; }

std::vector<double> MertonModel::getParameters() const {
  return {_S0, _r, _sigma, _lambda, _muJ, _sigmaJ, _k};
}

void MertonModel::setParameters(const std::vector<double> &params) {
  if (params.size() != 6) {
    throw std::invalid_argument("setParameters() requires exactly 6 values: "
    "S0, r, sigma, lambda, muJ, sigmaJ");
  }
  for (double value : params) {
    if (!std::isfinite(value)) throw std::domain_error("Les parametres doivent etre finis.");
  }
  if (params[0] <= 0.0 || params[2] < 0.0 || params[3] < 0.0 || params[5] < 0.0) {
    throw std::domain_error("S0 doit etre > 0; sigma, lambda et sigmaJ >= 0.");
  }
  const double k = std::expm1(params[4] + 0.5 * params[5] * params[5]);
  if (!std::isfinite(k) || !std::isfinite(params[3] * k)) {
    throw std::domain_error("La compensation des sauts doit etre finie.");
  }

  _S0 = params[0];
  _r = params[1];
  _sigma = params[2];
  _lambda = params[3];
  _muJ = params[4];
  _sigmaJ = params[5];

  _k = k;
}

void MertonModel::setSeed(size_t seed) const {
  _rng.seed(static_cast<std::mt19937::result_type>(seed));
}

void MertonModel::setRandomSeed() {
  _rng.seed(std::random_device{}());
}

std::mt19937 MertonModel::getSeed() const{
  return _rng;
}

size_t MertonModel::getNPath() const{
  return _nPath;
}

std::string MertonModel::getName() const {
  return "MertonModel";
}

std::unique_ptr<AbstractModel> MertonModel::clone() const {
  return std::make_unique<MertonModel>(*this);
}
