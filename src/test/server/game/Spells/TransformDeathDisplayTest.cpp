/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "IntegrationTestFixture.h"
#include "SpellAuras.h"
#include "SpellInfoTestHelper.h"
#include "gtest/gtest.h"

namespace
{
// 'Fiend Fury': the Skeletal Fiend transforms itself into "Skeletal Fiend (Enraged Form)"
static constexpr uint32 FIEND_FURY_SPELL_ID         = 3416;
static constexpr int32  ENRAGED_FORM_CREATURE_ID    = 2454;

// Skeletal Fiend and its own model.
static constexpr uint32 SKELETAL_FIEND_ENTRY    = 531;
static constexpr uint32 NATIVE_DISPLAY_ID       = 7550;

class TransformDeathDisplayTest : public IntegrationTestFixture
{
protected:
    void SetUp() override
    {
        IntegrationTestFixture::SetUp();

        _transformAuraInfo = SpellInfoBuilder()
            .WithId(FIEND_FURY_SPELL_ID)
            .WithEffect(0, SPELL_EFFECT_APPLY_AURA, SPELL_AURA_TRANSFORM)
            .WithEffectMiscValue(0, ENRAGED_FORM_CREATURE_ID)
            .BuildUnique();

        // Permanent aura, removed by the tests themselves
        _durationEntry = {};
        _durationEntry.Duration[0] = -1;
        _durationEntry.Duration[1] = -1;
        _durationEntry.Duration[2] = -1;
        _transformAuraInfo->DurationEntry = &_durationEntry;
    }

    std::unique_ptr<SpellInfo> _transformAuraInfo;
    SpellDurationEntry _durationEntry;
};

// A creature dying under its own transform keeps the transformed model, so the client plays the death animation
// cppcheck-suppress syntaxError
TEST_F(TransformDeathDisplayTest, CreatureKeepsSelfCastTransformDisplayOnDeath)
{
    TestCreature* creature = CreateTestCreature(100, SKELETAL_FIEND_ENTRY, TEST_FACTION_HOSTILE_TO_ALL);
    creature->SetNativeDisplayId(NATIVE_DISPLAY_ID);
    creature->SetDisplayId(NATIVE_DISPLAY_ID);

    Aura* aura = creature->AddAura(_transformAuraInfo.get(), 0x1, creature);
    ASSERT_NE(aura, nullptr);

    uint32 const transformedDisplayId = creature->GetDisplayId();
    ASSERT_NE(transformedDisplayId, NATIVE_DISPLAY_ID);

    creature->RemoveAllAurasOnDeath();
    EXPECT_EQ(creature->GetDisplayId(), transformedDisplayId);
    EXPECT_FALSE(creature->HasAura(FIEND_FURY_SPELL_ID));
    EXPECT_EQ(creature->getTransForm(), 0u);
}

// A transform cast by another unit (e.g. Polymorph, Hex) is still undone on death
// cppcheck-suppress syntaxError
TEST_F(TransformDeathDisplayTest, CreatureRestoresDisplayOnDeathWhenTransformedByOther)
{
    TestPlayer* caster = CreateTestPlayer(1, "Caster");

    TestCreature* creature = CreateTestCreature(100, SKELETAL_FIEND_ENTRY, TEST_FACTION_HOSTILE_TO_ALL);
    creature->SetNativeDisplayId(NATIVE_DISPLAY_ID);
    creature->SetDisplayId(NATIVE_DISPLAY_ID);

    Aura* aura = caster->AddAura(_transformAuraInfo.get(), 0x1, creature);
    ASSERT_NE(aura, nullptr);
    ASSERT_NE(creature->GetDisplayId(), NATIVE_DISPLAY_ID);

    creature->RemoveAllAurasOnDeath();
    EXPECT_EQ(creature->GetDisplayId(), NATIVE_DISPLAY_ID);
    EXPECT_FALSE(creature->HasAura(FIEND_FURY_SPELL_ID));
}

// Players are out of scope: a player dying under its own transform gets its native model back
// cppcheck-suppress syntaxError
TEST_F(TransformDeathDisplayTest, PlayerRestoresDisplayOnDeathWithSelfCastTransform)
{
    TestPlayer* player = CreateTestPlayer(1, "Costumed");
    player->SetNativeDisplayId(NATIVE_DISPLAY_ID);
    player->SetDisplayId(NATIVE_DISPLAY_ID);

    Aura* aura = player->AddAura(_transformAuraInfo.get(), 0x1, player);
    ASSERT_NE(aura, nullptr);
    ASSERT_NE(player->GetDisplayId(), NATIVE_DISPLAY_ID);

    player->RemoveAllAurasOnDeath();
    EXPECT_EQ(player->GetDisplayId(), NATIVE_DISPLAY_ID);
    EXPECT_FALSE(player->HasAura(FIEND_FURY_SPELL_ID));
}

// Only death keeps the model: a self-cast transform removed while alive (expiry, dispel, script) is undone
// cppcheck-suppress syntaxError
TEST_F(TransformDeathDisplayTest, CreatureRestoresDisplayWhenSelfCastTransformIsRemovedAlive)
{
    TestCreature* creature = CreateTestCreature(100, SKELETAL_FIEND_ENTRY, TEST_FACTION_HOSTILE_TO_ALL);
    creature->SetNativeDisplayId(NATIVE_DISPLAY_ID);
    creature->SetDisplayId(NATIVE_DISPLAY_ID);

    Aura* aura = creature->AddAura(_transformAuraInfo.get(), 0x1, creature);
    ASSERT_NE(aura, nullptr);
    ASSERT_NE(creature->GetDisplayId(), NATIVE_DISPLAY_ID);

    creature->RemoveAurasDueToSpell(FIEND_FURY_SPELL_ID);
    EXPECT_EQ(creature->GetDisplayId(), NATIVE_DISPLAY_ID);
    EXPECT_FALSE(creature->HasAura(FIEND_FURY_SPELL_ID));
}

} // namespace
