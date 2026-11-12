#include "Option.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

/*-----------------------------------------------------------------------------
-------------------Implementation Option Europeenne----------------------------
------------------------------------------------------------------------------*/

//Constructeur-----------------------------------------------------------------

EuropeanOption::EuropeanOption(double T, double K, OptionType type):
_T(T), _K(K), _type(type){
    if (!std::isfinite(T) || T < 0.0 || !std::isfinite(K) || K < 0.0) {
        throw std::invalid_argument("Maturite et strike doivent etre finis et >= 0.");
    }
}

//Methode-----------------------------------------------------------------------

double EuropeanOption::payoff(double spot) const
{
    if(_type == OptionType::Call) return std::max(0.0, spot - _K);
    else return std::max(0.0, _K - spot);
}

double EuropeanOption::getStrike() const { return _K; }

double EuropeanOption::getMaturity() const { return _T; }

OptionType EuropeanOption::getType() const { return _type; }

std::vector<double> EuropeanOption::getExerciseTimes() const { return {_T}; }

bool EuropeanOption::isPathDependent() const { return false; }

std::unique_ptr<AbstractOption> EuropeanOption::clone() const {
    return std::make_unique<EuropeanOption>(*this);
}

/*-----------------------------------------------------------------------------
-------------------Implementation Option Bermudienne---------------------------
------------------------------------------------------------------------------*/

//Constructeur-----------------------------------------------------------------

BermudianOption::BermudianOption(double K, std::vector<double> exerciseDates, 
OptionType type):
_K(K), _exerciseDates(exerciseDates), _type(type) {
    if (!std::isfinite(K) || K < 0.0 || _exerciseDates.empty()) {
        throw std::invalid_argument("Strike invalide ou calendrier d'exercice vide.");
    }
    double previous = -1.0;
    for (double date : _exerciseDates) {
        if (!std::isfinite(date) || date < 0.0 || date <= previous) {
            throw std::invalid_argument("Les dates doivent etre finies, >= 0 et strictement croissantes.");
        }
        previous = date;
    }
}

//Methode-----------------------------------------------------------------------

double BermudianOption::payoff(double spot) const {
    if(_type == OptionType::Call) return std::max(0.0, spot - _K);
    else return std::max(0.0, _K - spot);
}

double BermudianOption::getStrike() const { return _K; }

double BermudianOption::getMaturity() const {
    return _exerciseDates.back();
}

OptionType BermudianOption::getType() const { return _type; }

bool BermudianOption::isPathDependent() const {
    return false;
}

std::vector<double> BermudianOption::getExerciseTimes() const { 
    return _exerciseDates;
}

std::unique_ptr<AbstractOption> BermudianOption::clone() const {
    return std::make_unique<BermudianOption>(*this);
}

