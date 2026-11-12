#include "MonteCarloPricer.hpp"

int main() {

  MertonModel model(90.0, 0.05, 0.2, 5.0, -0.1, 0.3, 40000);
  std::shared_ptr<AbstractModel> model_ptr = model.clone();

  EuropeanOption callEurope(1.0, 100.0, OptionType::Call);
  EuropeanOption putEurope(1.0, 100.0, OptionType::Put);
  std::shared_ptr<AbstractOption> callEurope_ptr = callEurope.clone();
  std::shared_ptr<AbstractOption> putEurope_ptr = putEurope.clone();

  std::vector<double> exerciseDates = {0.25, 0.5, 0.75, 1.0};

  BermudianOption callBermude(100.0, exerciseDates , OptionType::Call);
  BermudianOption putBermude(100.0, exerciseDates, OptionType::Put);
  std::shared_ptr<AbstractOption> callBermude_ptr = callBermude.clone();
  std::shared_ptr<AbstractOption> putBermude_ptr = putBermude.clone();

  MonteCarlo mcCallEurope(model_ptr, callEurope_ptr);
  MonteCarlo mcPutEurope(model_ptr, putEurope_ptr);

  LongstaffSchwartz lsCallBermude(model_ptr, callBermude_ptr);
  LongstaffSchwartz lsPutBermude(model_ptr, putBermude_ptr);

  std::cout << "Prix du Call Europeen : " << mcCallEurope.price(10000) << " €\n";
  std::cout << "Prix du Put Europeen : " << mcPutEurope.price(10000) << " €\n";

  std::cout << "Prix du Call Bermudien : " << lsCallBermude.price(10000) << " €\n";
  std::cout << "Prix du Put Bermudien : " << lsPutBermude.price(10000) << " €\n";

  model.plotSimulatedPath(1.0);

  return 0;
}