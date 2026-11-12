#pragma once

#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

/*-----------------------------------------------------------------------------
-------------------Declaration Modele Abstrait---------------------------------
------------------------------------------------------------------------------*/


class AbstractModel {
public:

  virtual ~AbstractModel() = default;

  virtual std::vector<double> simulatePath(double maturity) const = 0;

  virtual void plotSimulatedPath(double maturity) const = 0;
    
  virtual double getRiskFreeRate() const = 0;
  virtual double getInitialSpot() const = 0;
  virtual std::vector<double> getParameters() const = 0;
  virtual void setParameters(const std::vector<double> &params) = 0;
  virtual void setSeed(size_t seed) const = 0;
  virtual void setRandomSeed() = 0;
  virtual std::mt19937 getSeed() const = 0;
  virtual size_t getNPath() const = 0;
  virtual std::string getName() const = 0;

  virtual std::unique_ptr<AbstractModel> clone() const = 0;
  
};


/*-----------------------------------------------------------------------------
-------------------Declaration Merton-Jump-Diffusion---------------------------
------------------------------------------------------------------------------*/


class MertonModel : public AbstractModel {
public:
  // Constructeur--------------------------------------------------------------

  MertonModel(double S0, double r, double sigma, double lambda, double muJ, 
  double sigmaJ, size_t nPath);

  MertonModel(const MertonModel& model);

  // Fonctions membres----------------------------------------------------------

  std::vector<double> simulatePath(double maturity) const override; // Genere le vecteur des St

  void plotSimulatedPath(double maturity) const override; // Genre le graphique

  double getRiskFreeRate() const override; // Renvoie le taux risque neutre
  double getInitialSpot() const override;  // Renvoie S0
  std::vector<double> getParameters()const override; // Renvoie tout les paramettres du modèle dans un vecteur
  void setParameters(const std::vector<double> &params) override; // Permet de changer les paramètre du vecteur pour callibration
  void setSeed(size_t seed) const override; // Change la graine
  void setRandomSeed() override; // Change la graine de facon aleatoire
  std::mt19937 getSeed() const override; // Renvoie la graine
  size_t getNPath() const override;

  std::string getName() const override; // Renvoie le nom du modele

  std::unique_ptr<AbstractModel> clone() const override; // Methode Clone

private:
  double _S0, _r, _sigma, _lambda, _muJ, _sigmaJ, _k;
  size_t _nPath;
  mutable std::mt19937 _rng;
};
