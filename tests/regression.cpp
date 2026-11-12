#include "MonteCarloPricer.hpp"
#include <algorithm>
#include <functional>
#include <limits>
#include <stdexcept>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void near(double actual, double expected, double tolerance, const char* message) {
    if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance) {
        std::cerr << message << ": actual=" << actual << ", expected=" << expected << '\n';
        throw std::runtime_error(message);
    }
}

void rejects(const std::function<void()>& action) {
    bool rejected = false;
    try { action(); } catch (const std::logic_error&) { rejected = true; }
    check(rejected, "Invalid input was accepted");
}

// Trajectoires imposees pour verifier les decisions et l'actualisation LSM.
class FixedPaths : public MertonModel {
public:
    explicit FixedPaths(std::vector<std::vector<double>> paths)
        : MertonModel(100, 0.1, 0, 0, 0, 0, 2), paths_(std::move(paths)) {}
    std::vector<double> simulatePath(double) const override {
        return paths_.at(next_++ % paths_.size());
    }
private:
    std::vector<std::vector<double>> paths_;
    mutable size_t next_ = 0;
};

void testClone() {
    MertonModel original(90, 0.05, 0.2, 5, -0.1, 0.3, 40);
    original.setSeed(123);
    original.simulatePath(1);
    auto copy = original.clone();
    check(original.getParameters() == copy->getParameters(), "Clone lost model parameters");
    check(original.getSeed() == copy->getSeed(), "Clone lost RNG state");
    check(original.simulatePath(1) == copy->simulatePath(1), "Clone changed simulation");
}

void testEuropean() {
    auto model = std::make_shared<MertonModel>(100, 0.05, 0.2, 0, 0, 0, 1);
    auto call = std::make_shared<EuropeanOption>(1, 100, OptionType::Call);
    auto put = std::make_shared<EuropeanOption>(1, 100, OptionType::Put);
    MonteCarlo mc(model, call);
    model->setSeed(42);
    const double callPrice = mc.price(200000);
    near(callPrice, 10.450583572185565, 0.15, "Black-Scholes call reference");
    model->setSeed(42);
    near(mc.price(200000), callPrice, 0, "Monte Carlo seed reproducibility");
    model->setSeed(42);
    near(MonteCarlo(model, put).price(200000), 5.573526022256971, 0.10,
         "Black-Scholes put reference");

    // Une seule date : LSM doit coincider exactement avec Monte Carlo.
    auto terminal = std::make_shared<BermudianOption>(100, std::vector<double>{1}, OptionType::Call);
    model->setSeed(10);
    const double european = mc.price(1000);
    model->setSeed(10);
    near(LongstaffSchwartz(model, terminal).price(1000), european, 1e-12,
         "Single-date Bermudian differs from European");

    auto expired = std::make_shared<BermudianOption>(90, std::vector<double>{0}, OptionType::Call);
    near(LongstaffSchwartz(model, expired).price(1), 10, 0, "Expired Bermudian");
}

void testJumps() {
    auto model = std::make_shared<MertonModel>(90, 0.05, 0.2, 5, -0.1, 0.3, 4);
    model->setSeed(888);
    double sum = 0, sumSquares = 0;
    const size_t samples = 100000;
    for (size_t i = 0; i < samples; ++i) {
        const double terminal = model->simulatePath(1).back();
        sum += terminal;
        sumSquares += terminal * terminal;
    }
    const double mean = sum / static_cast<double>(samples);
    const double variance = sumSquares / static_cast<double>(samples) - mean * mean;
    near(mean, 90 * std::exp(0.05), 6 * std::sqrt(variance / static_cast<double>(samples)),
         "Merton risk-neutral mean");

    model->setSeed(456);
    auto call = std::make_shared<EuropeanOption>(1, 100, OptionType::Call);
    const double callPrice = MonteCarlo(model, call).price(10000);
    auto singleDate = std::make_shared<BermudianOption>(100, std::vector<double>{1}, OptionType::Call);
    model->setSeed(456);
    near(LongstaffSchwartz(model, singleDate).price(10000), callPrice, 1e-12,
         "Single-date Merton pricing");
    auto bermudian = std::make_shared<BermudianOption>(100, std::vector<double>{0.25, 0.5, 0.75, 1}, OptionType::Put);
    const double price = LongstaffSchwartz(model, bermudian).price(10000);
    check(std::isfinite(price) && price >= 0 && price <= 100, "Invalid Merton Bermudian price");

    // Sauts de taille constante : sigmaJ=0 ne doit pas violer les distributions.
    MertonModel constantJumps(100, 0, 0, 2, 0, 0, 4);
    near(constantJumps.simulatePath(1).back(), 100, 0, "Constant unit jumps");
}

void testRegressionAndDiscounting() {
    auto put = std::make_shared<BermudianOption>(100, std::vector<double>{0.5, 1}, OptionType::Put);
    auto mixed = std::make_shared<FixedPaths>(std::vector<std::vector<double>>{
        {100, 80, 70}, {100, 120, 80}, {100, 80, 90}, {100, 120, 120}});
    const double discount = std::exp(-0.05);
    near(LongstaffSchwartz(mixed, put).price(4), (40 + 20 * discount) * discount / 4,
         1e-12, "Rank-deficient regression and mixed exercise");

    auto noITM = std::make_shared<FixedPaths>(std::vector<std::vector<double>>{{100, 120, 80}});
    near(LongstaffSchwartz(noITM, put).price(3), 20 * std::exp(-0.1), 1e-12,
         "Discounting without early ITM paths");
    auto onePath = std::make_shared<FixedPaths>(std::vector<std::vector<double>>{{100, 80, 70}});
    near(LongstaffSchwartz(onePath, put).price(1), 30 * std::exp(-0.1), 1e-12,
         "Regression with one observation");

    // Cas deterministe : le prix est le meilleur payoff actualise du calendrier.
    auto model = std::make_shared<MertonModel>(80, 0.05, 0, 0, 0, 0, 4);
    auto earlyPut = std::make_shared<BermudianOption>(100, std::vector<double>{0, 0.25, 0.5, 1}, OptionType::Put);
    near(LongstaffSchwartz(model, earlyPut).price(10), 20, 1e-10, "Deterministic immediate exercise");
}

void testInvalidInputs() {
    auto model = std::make_shared<MertonModel>(100, 0.05, 0.2, 0, 0, 0, 4);
    auto option = std::make_shared<EuropeanOption>(1, 100, OptionType::Call);
    rejects([&] { MonteCarlo(model, option).price(0); });
    rejects([&] { LongstaffSchwartz(model, option).price(0); });
    rejects([&] { MonteCarlo(nullptr, option); });
    rejects([&] { MonteCarlo(model, nullptr); });
    rejects([] { BermudianOption(100, {}, OptionType::Put); });
    rejects([] { BermudianOption(100, {1, 0.5}, OptionType::Put); });
    rejects([] { BermudianOption(100, {0.5, 0.5}, OptionType::Put); });
    rejects([] { BermudianOption(100, {-0.5, 1}, OptionType::Put); });
    rejects([] { EuropeanOption(-1, 100, OptionType::Call); });
    rejects([] { MertonModel(100, 0.05, 0.2, 0, 0, 0, 0); });
    rejects([] { MertonModel(100, 0.05, 0.2, -1, 0, 0, 4); });
    rejects([] { MertonModel(100, 0.05, 0.2, 1, 0, -0.1, 4); });
    rejects([&] { model->simulatePath(-1); });
    rejects([&] { model->simulatePath(std::numeric_limits<double>::quiet_NaN()); });
    rejects([&] { model->setParameters({100, 0.05, 0.2, 1, 0}); });
    const auto before = model->getParameters();
    rejects([&] { model->setParameters({100, 0.05, 0.2, 1, 1000, 1}); });
    check(model->getParameters() == before, "Failed parameter update mutated model");
}
}

int main() {
    try {
        testClone();
        testEuropean();
        testJumps();
        testRegressionAndDiscounting();
        testInvalidInputs();
        std::cout << "All regression checks passed.\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
