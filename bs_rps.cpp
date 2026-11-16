/*
************************************************************************************************
** Module	: bs_rps.cpp
** Version	: 1.0.1
** Author	: skutte (SkyChatz1234@gmail.com)
** Release	: 11th July, 2025
** Update	: 16th November, 2026
************************************************************************************************
** <Description>
**
**		This module is designed for ppl who wanted to play rps games
**
**		It is recommended to use latest Anope version 
**
** </Description>
**
************************************************************************************************
** Tested: 2.1.27, Unreal6+
**
** Providing Command:
**
** /msg BotServ HELP RPS
** /msg BotServ RPS #channel START <rounds>
** /msg BotServ RPS #channel rock | paper | scissors 
** or
** fantasy
** !rps rock | paper | scissors  
**
** Contact me 
** Email : SkyChatz1234@hotmail.com
** IRC : /s irc.skychatz.org:7000 #modules
**
** You may use this module under the terms of docs/LICENSE, in the Anope source directory
**
** Configuration:
** module { name = "bs_rps"; }
** command { name = "RPS"; service = "BotServ"; command = "botserv/rps"; }
** fantasy { name = "RPS"; command = "botserv/rps"; prepend_channel = true; }
**
************************************************************************************************
** Changelog:
**
** 1.0.1 - 16th November, 2026
**         * Anope 2.1.27 compatibility fixes:
**           - Replaced Anope::printf() with Anope::Format() (Anope::printf
**             is not available in 2.1)
**           - Removed source.GetChannel() usage (not present on
**             CommandSource in Anope 2.1); channel is now taken from
**             params[0] supplied by prepend_channel
**         * Fixed invalid move handling: unknown moves (e.g. "lizard")
**           were silently reported as a tie; now rejected with a clear
**           error message
**         * Fixed tie detection: explicit action == botMove comparison
**           instead of relying on the final else branch
**         * Bot now announces its own choice in the result message
**           (e.g. "I chose Paper, you chose Rock. Paper wraps Rock.")
**         * Replaced the 6-branch if/else outcome chain with a
**           table-driven std::map lookup
**         * Added per-nick rate limiting (2 second cooldown) to prevent
**           channel spam
**         * Localized all user-facing strings with _() for translation
**         * Made move/responses containers static const and RNG
**           thread_local for safety
**         * Improved OnHelp output with usage examples
**         * Corrected version gate message to "Requires Anope 2.1 or
**           newer"
**
** 1.0.0 - 11th July, 2025
**         * Initial release
**
************************************************************************************************
 */

#include "module.h"
#include <random>
#include <map>
#include <utility>
#include <chrono>

// ---------------------------------------------------------------------------
// Outcome table: (player_move, bot_move) -> result info
// ---------------------------------------------------------------------------
struct RpsOutcome
{
    bool playerWins;
    const char* message;
};

class CommandBSRps : public Command
{
    // Cooldown tracking: nick -> last play time
    std::map<Anope::string, std::chrono::steady_clock::time_point> lastPlay;
    static constexpr int COOLDOWN_SECONDS = 2;

    // Return canonical move name or empty if invalid
    static Anope::string NormalizeMove(const Anope::string& move)
    {
        if (move.equals_ci("ROCK"))     return "Rock";
        if (move.equals_ci("PAPER"))    return "Paper";
        if (move.equals_ci("SCISSORS")) return "Scissors";
        return "";
    }

    static const std::map<std::pair<Anope::string, Anope::string>, RpsOutcome>& Outcomes()
    {
        static const std::map<std::pair<Anope::string, Anope::string>, RpsOutcome> table = {
            // Player wins
            {{"Rock",     "Scissors"}, {true,  "Rock smashes Scissors"}},
            {{"Paper",    "Rock"},     {true,  "Paper wraps Rock"}},
            {{"Scissors", "Paper"},    {true,  "Scissors cut Paper"}},
            // Bot wins
            {{"Rock",     "Paper"},    {false, "Paper wraps Rock"}},
            {{"Paper",    "Scissors"}, {false, "Scissors cut Paper"}},
            {{"Scissors", "Rock"},     {false, "Rock smashes Scissors"}},
        };
        return table;
    }

    bool CheckCooldown(CommandSource& source)
    {
        const Anope::string& nick = source.GetNick();
        auto now = std::chrono::steady_clock::now();
        auto it = lastPlay.find(nick);
        if (it != lastPlay.end())
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
            if (elapsed < COOLDOWN_SECONDS)
            {
                source.Reply(_("Please wait %d second(s) before playing again."),
                             COOLDOWN_SECONDS - static_cast<int>(elapsed));
                return false;
            }
        }
        lastPlay[nick] = now;
        return true;
    }

 public:
    CommandBSRps(Module* creator, const Anope::string& sname = "botserv/rps")
        : Command(creator, sname, 1, 2)
    {
        this->SetDesc(_("Play Rock-Paper-Scissors against the channel bot"));
        this->SetSyntax("\037#channel\037 \037Rock|Paper|Scissors\037");
        this->AllowUnregistered(true);
    }

    void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
    {
        Anope::string chanName;
        Anope::string rawAction;

        if (!params.empty() && params[0].find('#') == 0)
        {
            chanName = params[0];
            if (params.size() > 1)
                rawAction = params[1];
        }
        else
        {
            source.Reply(_("Please specify a channel: RPS #channel <Rock|Paper|Scissors>"));
            return;
        }

        ChannelInfo* ci = ChannelInfo::Find(chanName);
        if (!ci)
        {
            source.Reply(CHAN_X_NOT_REGISTERED, chanName.c_str());
            return;
        }

        if (!ci->bi)
        {
            source.Reply(BOT_NOT_ASSIGNED);
            return;
        }

        if (!ci->c || !ci->c->FindUser(ci->bi))
        {
            source.Reply(BOT_NOT_ON_CHANNEL, ci->name.c_str());
            return;
        }

        if (rawAction.empty())
        {
            source.Reply(_("Please choose ROCK, PAPER, or SCISSORS."));
            return;
        }

        Anope::string action = NormalizeMove(rawAction);
        if (action.empty())
        {
            source.Reply(_("Invalid move '\037%s\037. Choose ROCK, PAPER, or SCISSORS."),
                         rawAction.c_str());
            return;
        }

        if (!CheckCooldown(source))
            return;

        static thread_local std::mt19937 rng(std::random_device{}());
        static const std::vector<Anope::string> moves = { "Rock", "Paper", "Scissors" };
        std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
        const Anope::string& botMove = moves[dist(rng)];

        Anope::string result;
        if (action == botMove)
        {
            result = _("Tie. Play again to win the game.");
        }
        else
        {
            const auto& table = Outcomes();
            auto it = table.find({action, botMove});
            if (it != table.end())
            {
                const RpsOutcome& o = it->second;
                if (o.playerWins)
                    result = Anope::Format(_("You win! %s."), o.message);
                else
                    result = Anope::Format(_("%s wins! %s."),
                                           ci->bi->nick.c_str(), o.message);
            }
            else
            {
                result = _("Tie. Play again to win the game.");
            }
        }

        // Announce bot's choice + result
        IRCD->SendPrivmsg(*ci->bi, ci->name,
            Anope::Format(_("I chose \037%s\037, you chose \037%s\037. %s"),
                          botMove.c_str(), action.c_str(), result.c_str()));
    }

    bool OnHelp(CommandSource& source, const Anope::string& subcommand) override
    {
        this->SendSyntax(source);
        source.Reply(" ");
        source.Reply(_("Rock-Paper-Scissors is a game played to settle disputes between two people.\n"
                       "Thought to be a game of chance that depends on random luck similar to\n"
                       "flipping coins or drawing straws."));
        source.Reply(" ");
        source.Reply(_("Examples:"));
        source.Reply(_("  /msg BotServ RPS #channel Rock"));
        source.Reply(_("  !rps Paper        (in a channel)"));
        return true;
    }
};

class BSRps : public Module
{
    CommandBSRps commandbsrps;

 public:
    BSRps(const Anope::string& modname, const Anope::string& creator)
        : Module(modname, creator, THIRD)
        , commandbsrps(this)
    {
        if (Anope::VersionMajor() < 2 ||
            (Anope::VersionMajor() == 2 && Anope::VersionMinor() < 1))
        {
            throw ModuleException("Requires Anope 2.1 or newer.");
        }
        this->SetAuthor("skutte");
        this->SetVersion("1.0.1");
    }
};

MODULE_INIT(BSRps)
