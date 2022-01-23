#pragma once

#include <hook.h>
#include <base/base.h>
#include <base/log.h>
#include <base/playerdb.h>
#include <mods/CommandSupport.h>
#include <Actor/Player.h>
#include <Level/Level.h>
#include <Level/AdventureSettings.h>
#include <Level/Abilities.h>
#include <Level/AbilitiesIndex.h>
#include <Level/PlayerPermissionLevel.h>
#include <Net/ServerNetworkHandler.h>
#include <Core/PermissionsFile.h>
#include <Packet/TextPacket.h>
#include <Packet/AdventureSettingsPacket.h>

DEF_LOGGER("AbilitiesCommand");