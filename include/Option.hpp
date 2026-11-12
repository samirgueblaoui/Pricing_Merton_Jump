#pragma once

#include <vector>
#include <memory>

enum class OptionType {
    Call,
    Put
};

/*-----------------------------------------------------------------------------
-------------------Declaration Option Abstraite--------------------------------
------------------------------------------------------------------------------*/

class AbstractOption {
public:
    
    //Methode------------------------------------------------------------------

    virtual double payoff(double spot) const = 0;
    virtual OptionType getType() const = 0;
    virtual bool isPathDependent() const = 0;
    virtual double getMaturity() const = 0;
    virtual std::vector<double> getExerciseTimes() const = 0;
    virtual double getStrike() const = 0;

    virtual std::unique_ptr<AbstractOption> clone() const = 0;

    //Destructeur--------------------------------------------------------------

    virtual ~AbstractOption() = default;
    
};

/*-----------------------------------------------------------------------------
-------------------Declaration Option Europeenne-------------------------------
------------------------------------------------------------------------------*/

class EuropeanOption : public AbstractOption {
public:

    //Constructeur-------------------------------------------------------------

    EuropeanOption(double T, double K, OptionType type);

    //Methode------------------------------------------------------------------

    double payoff(double spot) const override; 
    double getStrike() const override;
    double getMaturity() const override;
    OptionType getType() const override;
    std::vector<double> getExerciseTimes() const override;
    bool isPathDependent() const override;
    
    std::unique_ptr<AbstractOption> clone() const override;

private:
    double _T; // Maturite
    double _K; // Strike
    OptionType _type; // Call ou Put
};

/*-----------------------------------------------------------------------------
-------------------Declaration Option Bermudienne------------------------------
------------------------------------------------------------------------------*/

class BermudianOption : public AbstractOption {
public:

    //Constructeur-------------------------------------------------------------

    BermudianOption(double K, std::vector<double> exercieDates, OptionType type);

    //Methode------------------------------------------------------------------

    double payoff(double spot) const override;
    double getStrike() const override;
    double getMaturity() const override;
    OptionType getType() const override;
    bool isPathDependent() const override;
    std::vector<double> getExerciseTimes() const override;

    std::unique_ptr<AbstractOption> clone() const override;

private:
    double _K; // Strike
    std::vector<double> _exerciseDates; // Date pour lesqquelle on peut exercer
    OptionType _type; // Call ou Put
};
