#include "main.h"
#include <dllentry.h>

class AbilitiesCommand : public Command {
public:
    AbilitiesCommand() { selector.setIncludeDeadPlayers(true); }

    CommandSelector<Player> selector;
    enum class ActionType { Set, Reset } action = ActionType::Set;
    AbilitiesIndex ability = AbilitiesIndex::Invalid;
    bool boolValue = true;

    constexpr const char* abilityToString(AbilitiesIndex a) {
        switch (a) {
            case AbilitiesIndex::Build: return "Build";
            case AbilitiesIndex::Mine: return "Mine";
            case AbilitiesIndex::DoorsAndSwitches: return "DoorsAndSwitches";
            case AbilitiesIndex::OpenContainers: return "OpenContainers";
            case AbilitiesIndex::AttackPlayers: return "AttackPlayers";
            case AbilitiesIndex::AttackMobs: return "AttackMobs";
            case AbilitiesIndex::OperatorCommands: return "OperatorCommands";
            case AbilitiesIndex::Teleport: return "Teleport";
            case AbilitiesIndex::Invulnerable: return "Invulnerable";
            case AbilitiesIndex::Flying: return "Flying";
            case AbilitiesIndex::MayFly: return "MayFly";
            case AbilitiesIndex::Instabuild: return "Instabuild";
            case AbilitiesIndex::Lightning: return "Lightning";
            //case AbilitiesIndex::FlySpeed: return "FlySpeed";
            //case AbilitiesIndex::WalkSpeed: return "WalkSpeed";
            case AbilitiesIndex::Muted: return "Muted";
            case AbilitiesIndex::WorldBuilder: return "WorldBuilder";
            case AbilitiesIndex::NoClip: return "NoClip";
            default: return "Unknown";
        }
    }

    void setAbility(Player *player, CommandOutput &output, std::string const& abilityStatusStr, bool sendCommandFeedback) {

        // set ability serverside
        auto& abils = player->mAbilities;
        switch (action) {
            case ActionType::Set: {
                abils.Abilities[(int32_t) ability].value.val_bool = boolValue;
                abils.mPermissionsHandler->mPlayerPermissions = PlayerPermissionLevel::Custom;
                break;
            }
            case ActionType::Reset: {
                // default values
                abils.Abilities[(int32_t) AbilitiesIndex::Build].value.val_bool            = true; // 0
                abils.Abilities[(int32_t) AbilitiesIndex::Mine].value.val_bool             = true; // 1
                abils.Abilities[(int32_t) AbilitiesIndex::DoorsAndSwitches].value.val_bool = true; // 2
                abils.Abilities[(int32_t) AbilitiesIndex::OpenContainers].value.val_bool   = true; // 3
                abils.Abilities[(int32_t) AbilitiesIndex::AttackPlayers].value.val_bool    = true; // 4
                abils.Abilities[(int32_t) AbilitiesIndex::AttackMobs].value.val_bool       = true; // 5
                abils.Abilities[(int32_t) AbilitiesIndex::Lightning].value.val_bool        = true; // 12
                abils.Abilities[(int32_t) AbilitiesIndex::Muted].value.val_bool            = false; // 15
                abils.Abilities[(int32_t) AbilitiesIndex::WorldBuilder].value.val_bool     = false; // 16

                switch (player->mPlayerGameType) {
                    case GameType::Survival:
                    case GameType::Adventure:
                        abils.Abilities[(int32_t) AbilitiesIndex::Invulnerable].value.val_bool = false; // 8
                        abils.Abilities[(int32_t) AbilitiesIndex::Flying].value.val_bool       = false; // 9
                        abils.Abilities[(int32_t) AbilitiesIndex::MayFly].value.val_bool       = false; // 10
                        abils.Abilities[(int32_t) AbilitiesIndex::Instabuild].value.val_bool   = false; // 11
                        break;

                    case GameType::Creative:
                        abils.Abilities[(int32_t) AbilitiesIndex::Invulnerable].value.val_bool = true; // 8
                        abils.Abilities[(int32_t) AbilitiesIndex::Flying].value.val_bool       = false; // 9
                        abils.Abilities[(int32_t) AbilitiesIndex::MayFly].value.val_bool       = true; // 10
                        abils.Abilities[(int32_t) AbilitiesIndex::Instabuild].value.val_bool   = true; // 11
                        break;

                    case GameType::SurvivalViewer:
                    case GameType::CreativeViewer:
                        abils.Abilities[(int32_t) AbilitiesIndex::Invulnerable].value.val_bool = true; // 8
                        abils.Abilities[(int32_t) AbilitiesIndex::Flying].value.val_bool       = true; // 9
                        abils.Abilities[(int32_t) AbilitiesIndex::MayFly].value.val_bool       = true; // 10
                        abils.Abilities[(int32_t) AbilitiesIndex::Instabuild].value.val_bool   = true; // 11
                        break;

                    default: break;
                }

                auto& perms = abils.mPermissionsHandler->mPlayerPermissions;
                if (player->isOperator()) {
                    abils.Abilities[(int32_t) AbilitiesIndex::OperatorCommands].value.val_bool = true; // 6
                    abils.Abilities[(int32_t) AbilitiesIndex::Teleport].value.val_bool         = true; // 7
                    perms = PlayerPermissionLevel::Operator;
                }
                else {
                    abils.Abilities[(int32_t) AbilitiesIndex::OperatorCommands].value.val_bool = false; // 6
                    abils.Abilities[(int32_t) AbilitiesIndex::Teleport].value.val_bool         = false; // 7
                    perms = PlayerPermissionLevel::Member;
                }
                break;
            }
            default: break;
        }

        // sync with permissions.json
        auto permFile = LocateService<ServerNetworkHandler>()->mPermissionsFile;
        if (permFile) {

            auto& db = Mod::PlayerDatabase::GetInstance();
            auto it = db.Find(player);
        
            if (it && (it->xuid > 0)) {
                CallServerClassMethod<void>(
                    "?persistPlayerPermissionsToDisk@PermissionsFile@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4PlayerPermissionLevel@@@Z",
                    permFile, std::to_string(it->xuid), player->getPlayerPermissionLevel());
            }
        }

        // resend AdventureSettingsPacket
        AdventureSettingsPacket pkt;
        CallServerClassMethod<void>(
            "??0AdventureSettingsPacket@@QEAA@AEBUAdventureSettings@@AEBVAbilities@@UActorUniqueID@@_N@Z",
            &pkt, LocateService<Level>()->getAdventureSettings(), &abils, player->getUniqueID(), false);

        LocateService<Level>()->forEachPlayer([&](Player &p) -> bool {
            p.sendNetworkPacket(pkt);
            return true;
        });

        if (sendCommandFeedback) {
            auto output = TextPacket::createTextPacket<TextPacketType::SystemMessage>(abilityStatusStr);
            player->sendNetworkPacket(output);
        }
    }

    void execute(CommandOrigin const &origin, CommandOutput &output) {
        
        auto selectedEntities = selector.results(origin);
        if (selectedEntities.empty()) {
            return output.error("No targets matched selector");
        }

        bool sendCommandFeedback = (output.type != CommandOutputType::NoFeedback);

        int32_t resultCount = selectedEntities.count();
        std::string successStr; // command output
        std::string abilityStatusStr; // individual player output
        switch (action) {
            case ActionType::Set: {
                std::string thisAbil = abilityToString(ability);
                std::string thisVal = (boolValue ? "true" : "false");
                successStr += "Set the \"" + thisAbil + "\" ability to " + thisVal;
                abilityStatusStr = "Your \"" + thisAbil + "\" ability has been set to " + thisVal;
                break;
            }
            case ActionType::Reset: {
                successStr += "Reset all abilities";
                abilityStatusStr = "Your abilities have been reset";
                break;
            }
            default: break;
        }
        successStr += " for " + std::to_string(resultCount) + std::string(resultCount == 1 ? " player" : " players");

        for (auto player : selectedEntities) {
            setAbility(player, output, abilityStatusStr, sendCommandFeedback);
        }
        
        output.success(successStr);
    }

    static void setup(CommandRegistry *registry) {
        using namespace commands;

        registry->registerCommand(
            "abilities", "Sets a player's ability flags.", CommandPermissionLevel::GameMasters, CommandFlagUsage, CommandFlagNone);

        addEnum<ActionType>(registry, "setAbilityAction", {
            { "set", ActionType::Set }
        });

        addEnum<ActionType>(registry, "resetAbilityAction", {
            { "reset", ActionType::Reset }
        });

        // command will silently fail if any enum shortnames have capitals!
        addEnum<AbilitiesIndex>(registry, "abilitiesIndex", {
            { "build", AbilitiesIndex::Build },
            { "mine", AbilitiesIndex::Mine },
            { "doorsandswitches", AbilitiesIndex::DoorsAndSwitches },
            { "opencontainers", AbilitiesIndex::OpenContainers },
            { "attackplayers", AbilitiesIndex::AttackPlayers },
            { "attackmobs", AbilitiesIndex::AttackMobs },
            { "operatorcommands", AbilitiesIndex::OperatorCommands },
            { "teleport", AbilitiesIndex::Teleport },
            { "invulnerable", AbilitiesIndex::Invulnerable },
            { "flying", AbilitiesIndex::Flying },
            { "mayfly", AbilitiesIndex::MayFly },
            { "instabuild", AbilitiesIndex::Instabuild },
            { "lightning", AbilitiesIndex::Lightning },
            //{ "flyspeed", AbilitiesIndex::FlySpeed }, - not changeable serverside
            //{ "walkspeed", AbilitiesIndex::WalkSpeed } - change this via attributes instead
            { "muted", AbilitiesIndex::Muted },
            { "worldbuilder", AbilitiesIndex::WorldBuilder },
            { "noclip", AbilitiesIndex::NoClip }
        });

        registry->registerOverload<AbilitiesCommand>("abilities",
            mandatory<CommandParameterDataType::ENUM>(&AbilitiesCommand::action, "set", "setAbilityAction"),
            mandatory(&AbilitiesCommand::selector, "player"),
            mandatory<CommandParameterDataType::ENUM>(&AbilitiesCommand::ability, "ability", "abilitiesIndex"),
            optional(&AbilitiesCommand::boolValue, "boolValue")
        );

        registry->registerOverload<AbilitiesCommand>("abilities",
            mandatory<CommandParameterDataType::ENUM>(&AbilitiesCommand::action, "reset", "resetAbilityAction"),
            optional(&AbilitiesCommand::selector, "player")
        );
    }
};

void dllenter() {}
void dllexit() {}
void PreInit() {
    Mod::CommandSupport::GetInstance().AddListener(SIG("loaded"), &AbilitiesCommand::setup);
}
void PostInit() {}