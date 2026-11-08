#include <catch2/catch_test_macros.hpp>

#include "PlayerLives.h"

using RType::PlayerLives;

TEST_CASE("A hit takes a life and makes the player invulnerable", "[lives]")
{
    PlayerLives lives(3, 2.f);

    CHECK(lives.track(7));
    CHECK_FALSE(lives.track(7));
    CHECK(lives.hit(7) == PlayerLives::Hit::Respawn);
    CHECK(lives.lives(7) == 2);

    // Still invulnerable: an enemy bullet in the same second does nothing.
    CHECK(lives.hit(7) == PlayerLives::Hit::Ignored);
    lives.update(1.f);
    CHECK(lives.hit(7) == PlayerLives::Hit::Ignored);
    CHECK(lives.lives(7) == 2);

    lives.update(1.f);
    CHECK(lives.hit(7) == PlayerLives::Hit::Respawn);
    CHECK(lives.lives(7) == 1);
}

TEST_CASE("The last life puts the player out", "[lives]")
{
    PlayerLives lives(1, 2.f);
    lives.track(3);

    CHECK(lives.hit(3) == PlayerLives::Hit::Out);
    CHECK(lives.lives(3) == 0);
    lives.update(10.f);
    CHECK(lives.hit(3) == PlayerLives::Hit::Ignored);
}

TEST_CASE("Hits on other entities are ignored", "[lives]")
{
    PlayerLives lives(3, 2.f);

    CHECK(lives.hit(1) == PlayerLives::Hit::Ignored);
    CHECK(lives.lives(1) == 0);
}

TEST_CASE("Players that left are forgotten", "[lives]")
{
    PlayerLives lives(3, 2.f);
    lives.track(1);
    lives.track(2);
    lives.hit(2);

    lives.forgetIf([](std::size_t entity) { return entity == 2; });

    CHECK(
        lives.all() == std::unordered_map<std::size_t, std::uint8_t> {{1, 3}});
    CHECK(lives.track(2));
    CHECK(lives.lives(2) == 3);
}
