#include "MonteCarloPricer.hpp"
#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>

/*-----------------------------------------------------------------------------
-------------------Implementation Methode Pricing------------------------------
------------------------------------------------------------------------------*/

//Constructeur-----------------------------------------------------------------

PricingEngine::PricingEngine(std::shared_ptr<AbstractModel> model, 
std::shared_ptr<AbstractOption> option) :
_model(model), _option(option) {
    if (!_model || !_option) {
        throw std::invalid_argument("Le modele et l'option sont obligatoires.");
    }
}

/*-----------------------------------------------------------------------------
-------------------Implementation Methode Monte Carlo--------------------------
------------------------------------------------------------------------------*/

//Constructeur-----------------------------------------------------------------

MonteCarlo::MonteCarlo(std::shared_ptr<AbstractModel> model, 
std::shared_ptr<AbstractOption> option):
PricingEngine(model, option) {}

//Methode-----------------------------------------------------------------------

double MonteCarlo::price (size_t steps) const {
    if (steps == 0) throw std::invalid_argument("Le nombre de trajectoires doit etre > 0.");
    double sum = 0.0;
    // Le modele contient un generateur mutable : consommation sequentielle,
    // sans reinitialisation, pour respecter setSeed() et eviter une data race.
    for(size_t i = 0; i < steps; i++){
        sum += _option->payoff(_model->simulatePath(_option->getMaturity()).back());
    }
    return  (std::exp(- _model->getRiskFreeRate() * _option->getMaturity())) * (sum / static_cast<double>(steps));
}

/*-----------------------------------------------------------------------------
-------------------Implementation Methode Longstaff-Schwartz-------------------
------------------------------------------------------------------------------*/

//Constructeur-----------------------------------------------------------------



LongstaffSchwartz::LongstaffSchwartz(std::shared_ptr<AbstractModel> model, 
std::shared_ptr<AbstractOption> option):
PricingEngine(model, option) {}

//Methode-----------------------------------------------------------------------

double LongstaffSchwartz::price(size_t nPaths) const
{
    if (nPaths == 0) throw std::invalid_argument("Le nombre de trajectoires doit etre > 0.");
    const auto exerciseTimes = _option->getExerciseTimes();
    if (exerciseTimes.empty()) throw std::invalid_argument("Le calendrier d'exercice est vide.");
    double previous = -1.0;
    for (double date : exerciseTimes) {
        if (!std::isfinite(date) || date < 0.0 || date <= previous) {
            throw std::invalid_argument("Calendrier d'exercice invalide.");
        }
        previous = date;
    }
    const double r = _model->getRiskFreeRate();
    const size_t nSteps = exerciseTimes.size();
    const double maturity = exerciseTimes.back();
    if (maturity == 0.0) return _option->payoff(_model->getInitialSpot());
    const size_t nPath = _model->getNPath();
    if (nPath == 0) throw std::invalid_argument("Le nombre de pas doit etre > 0.");

    // On calcule les indices qui correspondent aux exerciseTimes
    std::vector<size_t> exerciseIndices;
    for (double t : exerciseTimes) {
        size_t idx = static_cast<size_t>(std::round(t / maturity * static_cast<double>(nPath)));
        exerciseIndices.push_back(idx);
    }

    // On prépare la matrice des spots
    std::vector<std::vector<double>> spots(nPaths, std::vector<double>(nSteps));

    // Maintenant on simule et on remplit spots[i][j]
    for (size_t i = 0; i < nPaths; ++i) {
        auto path = _model->simulatePath(maturity);
        for (size_t j = 0; j < nSteps; ++j) {
            spots[i][j] = path.at(exerciseIndices[j]);
        }
    }

    // At final time: payoff
    std::vector<double> cashflows(nPaths);
    for (size_t i = 0; i < nPaths; ++i) {
        cashflows[i] = _option->payoff(spots[i].back());
    }

    // Backward induction
    for (size_t next = nSteps - 1; next > 0; --next) {
        const size_t t = next - 1;
        double dt = exerciseTimes[t + 1] - exerciseTimes[t];
        double discount = std::exp(-r * dt);

        // Select in-the-money paths
        std::vector<size_t> itmIndices;
        for (size_t i = 0; i < nPaths; ++i) {
            if (_option->payoff(spots[i][t]) > 0.0) {
                itmIndices.push_back(i);
            }
        }

        if (itmIndices.empty()) {
            /*  ➜  Aucun chemin ITM : on actualise
                TOUS les cashflows et on continue   */
            for (size_t i = 0; i < nPaths; ++i)
                cashflows[i] *= discount;
            continue;
        }

        // Prepare regression
        std::vector<std::vector<double>> X;
        std::vector<double> Y;

        for (auto i : itmIndices) {
            X.push_back(basisFunctions(spots[i][t]));
            Y.push_back(cashflows[i] * discount);
        }

        auto beta = regress(X, Y);

        // Update cashflows: exercise or continue
        for (auto i : itmIndices) {
            auto bf = basisFunctions(spots[i][t]);
            double continuation = std::inner_product(beta.begin(), beta.end(), bf.begin(), 0.0);
            double immediate = _option->payoff(spots[i][t]);

            if (immediate >= continuation) {
                cashflows[i] = immediate;
            } else {
                cashflows[i] *= discount;
            }
        }
        for (size_t i = 0; i < nPaths; ++i) {
            // si i n’est pas dans itmIndices
            if (!std::binary_search(itmIndices.begin(), itmIndices.end(), i))
                cashflows[i] *= discount;
        }
    }

    double finalDiscount = std::exp(-r * exerciseTimes[0]);
    return finalDiscount * std::accumulate(cashflows.begin(), cashflows.end(), 0.0) / static_cast<double>(nPaths);
}

std::vector<double> LongstaffSchwartz::basisFunctions(double S) const {
    const double x = S / _model->getInitialSpot() - 1.0;
    return {1.0, x, x*x, x*x*x, x*x*x*x, x*x*x*x*x};
}

std::vector<double> LongstaffSchwartz::regress(const std::vector<std::vector<double>>& X, 
const std::vector<double>& Y) const
{
    const size_t columns = X.front().size();
    const size_t m = std::min(columns, X.size());
    std::vector<std::vector<double>> Q;
    std::vector<std::vector<double>> R(m, std::vector<double>(m, 0.0));
    std::vector<double> projectedY(m, 0.0);

    // QR avec reorthogonalisation : evite de former X^T X et reduit le
    // degre du polynome si les observations ne permettent pas de l'identifier.
    const double tolerance = 100.0 * std::numeric_limits<double>::epsilon();
    for (size_t j = 0; j < m; ++j) {
        std::vector<double> v(X.size());
        double originalNorm = 0.0;
        for (size_t i = 0; i < X.size(); ++i) {
            v[i] = X[i][j];
            originalNorm = std::hypot(originalNorm, v[i]);
        }
        for (int pass = 0; pass < 2; ++pass) {
            for (size_t k = 0; k < Q.size(); ++k) {
                const double projection = std::inner_product(Q[k].begin(), Q[k].end(), v.begin(), 0.0);
                R[k][j] += projection;
                for (size_t i = 0; i < v.size(); ++i) v[i] -= projection * Q[k][i];
            }
        }
        double norm = 0.0;
        for (double value : v) norm = std::hypot(norm, value);
        if (!std::isfinite(norm) || !std::isfinite(originalNorm) ||
            norm <= tolerance * std::max(1.0, originalNorm)) break;
        R[j][j] = norm;
        for (double& value : v) value /= norm;
        projectedY[j] = std::inner_product(v.begin(), v.end(), Y.begin(), 0.0);
        Q.push_back(std::move(v));
    }

    std::vector<double> beta(columns, 0.0);
    for (size_t remaining = Q.size(); remaining > 0; --remaining) {
        const size_t i = remaining - 1;
        double value = projectedY[i];
        for (size_t j = i + 1; j < Q.size(); ++j) value -= R[i][j] * beta[j];
        beta[i] = value / R[i][i];
    }
    return beta;
}
