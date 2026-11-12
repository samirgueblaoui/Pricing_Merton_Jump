#pragma once

#include "MertonModel.hpp"
#include "Option.hpp"

/*-----------------------------------------------------------------------------
-------------------Declaration Pricing General---------------------------------
------------------------------------------------------------------------------*/

class PricingEngine {
public:
    //Constructeur-------------------------------------------------------------

    PricingEngine(std::shared_ptr<AbstractModel> model, 
    std::shared_ptr<AbstractOption> option);

    //Destructeur-------------------------------------------------------------

    virtual ~PricingEngine() = default;

    //Methode------------------------------------------------------------------

    virtual double price(size_t steps) const = 0;
    
protected:
    std::shared_ptr<AbstractModel> _model;
    std::shared_ptr<AbstractOption> _option;

};

/*-----------------------------------------------------------------------------
-------------------Declaration Methode Monte Carlo-----------------------------
------------------------------------------------------------------------------*/

class MonteCarlo : public PricingEngine
{
public:
    //Constructeur-------------------------------------------------------------

    MonteCarlo(std::shared_ptr<AbstractModel> model, 
    std::shared_ptr<AbstractOption> option);

    //Methode------------------------------------------------------------------

    double price (size_t steps) const override;
};

/*-----------------------------------------------------------------------------
-------------------Declaration Methode Longstaff-Schwartz----------------------
------------------------------------------------------------------------------*/

class LongstaffSchwartz : public PricingEngine
{
public:
    //Constructeur-------------------------------------------------------------

    LongstaffSchwartz(std::shared_ptr<AbstractModel> model, 
    std::shared_ptr<AbstractOption> option);

    //Methode------------------------------------------------------------------

    double price(size_t nPaths) const override;

private:
    std::vector<double> basisFunctions(double spot) const;
    std::vector<double> regress(const std::vector<std::vector<double>>& X, 
    const std::vector<double>& Y) const;
};
