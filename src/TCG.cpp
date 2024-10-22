#include <array>
#include <cassert>

#include <map>
#include <random>

#pragma warning(push)
#pragma warning(disable : 4127)
#include <csv.hpp>
#pragma warning(pop)

enum class CardNumber
{
    OneToThird,
    Fourth,
    Fifth
};

enum class Deck
{
    All,
    Charizard,
    Mewtwo,
    Pikachu
};

enum class Rarity
{
    Diamond_1,
    Diamond_2,
    Diamond_3,
    Diamond_4,
    Star_1,
    Star_2,
    Star_3,
    Crown
};

using RateByRarity = std::map<Rarity, double>;
using RateByCardNumberAndRarity = std::map<CardNumber, RateByRarity>;

double parsePercentage(std::string percentStr)
{
    percentStr.erase(std::remove(percentStr.begin(), percentStr.end(), '%'), percentStr.end());
    return std::stod(percentStr);
}

RateByCardNumberAndRarity readRateByCardNumberAndRarity()
{
    RateByCardNumberAndRarity rateByCardNumberAndRarity;
    csv::CSVReader reader("Rate by rarity.csv", csv::CSVFormat().delimiter('\t'));

    for (Rarity rarity : { Rarity::Diamond_1, Rarity::Diamond_2, Rarity::Diamond_3, Rarity::Diamond_4, Rarity::Star_1, Rarity::Star_2, Rarity::Star_3, Rarity::Crown })
    {
        csv::CSVRow row;
        reader.read_row(row);

        assert(row.size() == 3); // the CSV is badly formatted

        rateByCardNumberAndRarity[CardNumber::OneToThird][rarity] = parsePercentage(row[0].get<std::string>());
        rateByCardNumberAndRarity[CardNumber::Fourth][rarity] = parsePercentage(row[1].get<std::string>());
        rateByCardNumberAndRarity[CardNumber::Fifth][rarity] = parsePercentage(row[2].get<std::string>());
    }

    return rateByCardNumberAndRarity;
}

using CardCountByRarity = std::map<Rarity, uint16_t>;
using CardCountByDeckAndRarity = std::map<Deck, CardCountByRarity>;

CardCountByDeckAndRarity readCardCountByDeckAndRarity()
{
    CardCountByDeckAndRarity cardCountByDeckAndRarity{};
    std::ifstream file("Repartition by deck and rarity.csv");
    for (Rarity rarity : { Rarity::Diamond_1, Rarity::Diamond_2, Rarity::Diamond_3, Rarity::Diamond_4, Rarity::Star_1, Rarity::Star_2, Rarity::Star_3, Rarity::Crown })
    {
        for (Deck deck : {Deck::Mewtwo, Deck::Pikachu, Deck::Charizard, Deck::All})
        {
            uint16_t cardCount{};
            file >> cardCount;
            cardCountByDeckAndRarity[deck][rarity] = cardCount;
        }
    }
    return cardCountByDeckAndRarity;
}

struct Card
{
    Rarity rarity;
    uint16_t number;
};

using Booster = std::array<Card, 5>;

class RandomCardGenerator
{
public:
    RandomCardGenerator(const RateByCardNumberAndRarity& rateByCardNumberAndRarity, const CardCountByDeckAndRarity& cardCountByDeckAndRarity)
        : m_rateByCardNumberAndRarity(rateByCardNumberAndRarity)
        , m_cardCountByDeckAndRarity(cardCountByDeckAndRarity)
        , m_randomEngine(m_randomDevice())
    {
        for (CardNumber cardNumber : {CardNumber::OneToThird, CardNumber::Fourth, CardNumber::Fifth})
        {
            const RateByRarity& rateByRarity = m_rateByCardNumberAndRarity.at(cardNumber);
            double diamond_1 = rateByRarity.at(Rarity::Diamond_1);
            double diamond_2 = rateByRarity.at(Rarity::Diamond_2);
            double diamond_3 = rateByRarity.at(Rarity::Diamond_3);
            double diamond_4 = rateByRarity.at(Rarity::Diamond_4);
            double star_1 = rateByRarity.at(Rarity::Star_1);
            double star_2 = rateByRarity.at(Rarity::Star_2);
            double star_3 = rateByRarity.at(Rarity::Star_3);
            double crown = rateByRarity.at(Rarity::Crown);
            assert(diamond_1 + diamond_2 + diamond_3 + diamond_4 + star_1 + star_2 + star_3 + crown == 100.000);  // The provided rates by rarity are incorrect, as they do not add up to 100%

            DistributionForRarity distribution{ diamond_1, diamond_2, diamond_3, diamond_4, star_1, star_2, star_3, crown };
            auto it = m_distributionForRarityByCardNumber.insert(std::make_pair(cardNumber, distribution));
            assert(it.second); // Failed to insert
        }

        for (Deck deck : {Deck::Charizard, Deck::Mewtwo, Deck::Pikachu})
        {
            auto it = m_distributionByDeckAndRarity.insert(std::make_pair(deck, DistributionByRarity{}));
            assert(it.second); // Failed to insert

            DistributionByRarity& distributionByRarity = it.first->second;
            for (Rarity rarity : { Rarity::Diamond_1, Rarity::Diamond_2, Rarity::Diamond_3, Rarity::Diamond_4, Rarity::Star_1, Rarity::Star_2, Rarity::Star_3, Rarity::Crown })
            {
                uint16_t max = cardCountByDeckAndRarity.at(deck).at(rarity) + cardCountByDeckAndRarity.at(Deck::All).at(rarity) - 1;
                distributionByRarity.insert(std::make_pair(rarity, std::uniform_int_distribution<uint16_t>{0, max}));
            }
        }
    }

    Booster getRandomBooster(Deck deck)
    {
        return
        {
            getRandomCard(CardNumber::OneToThird, deck),
            getRandomCard(CardNumber::OneToThird, deck),
            getRandomCard(CardNumber::OneToThird, deck),
            getRandomCard(CardNumber::Fourth, deck),
            getRandomCard(CardNumber::Fifth, deck)
        };
    }

private:
    Card getRandomCard(CardNumber cardNumber, Deck deck)
    {
        Rarity rarity = getRandomRarity(cardNumber);
        uint16_t number = getRandomNumber(deck, rarity);
        return { rarity, number };
    }

    Rarity getRandomRarity(CardNumber cardNumber)
    {
        return static_cast<Rarity>(m_distributionForRarityByCardNumber.at(cardNumber)(m_randomEngine));
    }

    uint16_t getRandomNumber(Deck deck, Rarity rarity)
    {
        return m_distributionByDeckAndRarity.at(deck).at(rarity)(m_randomEngine);
    }
    
    const RateByCardNumberAndRarity& m_rateByCardNumberAndRarity;
    const CardCountByDeckAndRarity& m_cardCountByDeckAndRarity;

    std::random_device m_randomDevice{};
    std::default_random_engine m_randomEngine{};

    using DistributionForRarity = std::discrete_distribution<uint16_t>;
    using DistributionForRarityByCardNumber = std::map<CardNumber, DistributionForRarity>;
    DistributionForRarityByCardNumber m_distributionForRarityByCardNumber{};

    using DistributionByRarity = std::map<Rarity, std::uniform_int_distribution<uint16_t>>;
    using DistributionByDeckAndRarity = std::map<Deck, DistributionByRarity>;
    DistributionByDeckAndRarity m_distributionByDeckAndRarity{};
};

#define DEBUG 1
int main()
{
    RateByCardNumberAndRarity rateByCardNumberAndRarity = readRateByCardNumberAndRarity();
    CardCountByDeckAndRarity cardCountByDeckAndRarity = readCardCountByDeckAndRarity();
    RandomCardGenerator randomCardGenerator(rateByCardNumberAndRarity, cardCountByDeckAndRarity);
    std::vector<Booster> boosters{};
    for (size_t i = 0; i < 1000000; ++i)
    {
        boosters.push_back(randomCardGenerator.getRandomBooster(Deck::Mewtwo));
    }

#if DEBUG
    {
        std::map<Rarity, uint32_t> computedProbabilities{};
        for (const Booster& booster : boosters)
        {
            computedProbabilities[booster[0].rarity]++;
            computedProbabilities[booster[1].rarity]++;
            computedProbabilities[booster[2].rarity]++;
        }
    }

    {
        std::map<Rarity, uint32_t> computedProbabilities{};
        for (const Booster& booster : boosters)
        {
            computedProbabilities[booster[3].rarity]++;
        }
    }

    {
        std::map<Rarity, uint32_t> computedProbabilities{};
        for (const Booster& booster : boosters)
        {
            computedProbabilities[booster[4].rarity]++;
        }
    }
#endif

    return 0;
}