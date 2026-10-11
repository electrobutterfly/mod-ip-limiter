/*
 * This file is part of the IP Limiter Module subsystem for AzerothCore.
 *
 * Copyright (C) 2026 Klaus Simon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "Config.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Timer.h"
#include "WorldSession.h"
#include "WorldSessionMgr.h"

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class IPLimiter_WorldScript : public WorldScript
{
public:
    IPLimiter_WorldScript()
        : WorldScript("IPLimiter_WorldScript",
                      { WORLDHOOK_ON_UPDATE, WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED, WORLDHOOK_ON_AFTER_CONFIG_LOAD })
    { }

    void OnBeforeWorldInitialized() override
    {
        LoadWhitelist();
        LOG_INFO("server.loading",
            "IP Limit module loaded. Enabled: {}, Max players per IP: {}, Check interval: {} ms",
            sConfigMgr->GetOption<bool>("IPLimit.Enabled", false),
            sConfigMgr->GetOption<uint32>("IPLimit.MaxPlayersPerIP", 5),
            sConfigMgr->GetOption<uint32>("IPLimit.CheckInterval", 30000));
    }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        LoadWhitelist();
    }

    void OnUpdate(uint32 diff) override
    {
        if (!sConfigMgr->GetOption<bool>("IPLimit.Enabled", false))
            return;

        _updateTimer += diff;

        uint32 checkInterval = sConfigMgr->GetOption<uint32>("IPLimit.CheckInterval", 30000);
        if (_updateTimer < checkInterval)
            return;

        _updateTimer = 0;

        uint32 maxPlayersPerIP = sConfigMgr->GetOption<uint32>("IPLimit.MaxPlayersPerIP", 5);

        LOG_INFO("server.loading",
            "IP Limit check tick -- maxPerIP={} whitelistEntries={}",
            maxPlayersPerIP, _whitelist.size());

        std::unordered_map<std::string, std::vector<WorldSession*>> ipSessions;

        WorldSessionMgr::SessionMap const& sessions = sWorldSessionMgr->GetAllSessions();

        for (auto const& sessionPair : sessions)
        {
            WorldSession* session = sessionPair.second;
            if (!session)
                continue;

            if (!session->GetPlayer())
                continue;

            std::string const& ip = session->GetRemoteAddress();

            if (IsWhitelisted(ip))
            {
                LOG_INFO("server.loading",
                    "IP Limit: skipping player '{}' from whitelisted IP {}",
                    session->GetPlayer()->GetName(), ip);
                continue;
            }

            ipSessions[ip].push_back(session);
        }

        for (auto const& entry : ipSessions)
        {
            std::string const& ip = entry.first;
            std::vector<WorldSession*> const& sessionList = entry.second;

            LOG_INFO("server.loading",
                "IP Limit: {} has {} session(s) (limit {})",
                ip, sessionList.size(), maxPlayersPerIP);

            if (sessionList.size() > maxPlayersPerIP)
            {
                LOG_INFO("server.loading",
                    "IP Limit: kicking all {} sessions from {} (limit {})",
                    sessionList.size(), ip, maxPlayersPerIP);

                for (WorldSession* session : sessionList)
                {
                    Player* player = session->GetPlayer();
                    if (!player)
                        continue;

                    LOG_INFO("server.loading",
                        "IP Limit: kicking player {} (GUID: {})",
                        player->GetName(), player->GetGUID().GetCounter());

                    std::string kickReason =
                        "IP limit exceeded: " + std::to_string(maxPlayersPerIP) +
                        " players max per IP";
                    session->KickPlayer(kickReason);
                }
            }
        }
    }

private:
    uint32 _updateTimer = 0;
    std::vector<std::string> _whitelist;

    void LoadWhitelist()
    {
        _whitelist.clear();

        std::string whitelistStr = sConfigMgr->GetOption<std::string>("IPLimit.Whitelist", "");
        if (whitelistStr.empty())
            return;

        std::stringstream ss(whitelistStr);
        std::string token;
        while (std::getline(ss, token, ','))
        {
            size_t start = token.find_first_not_of(" \t");
            size_t end   = token.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos)
                _whitelist.push_back(token.substr(start, end - start + 1));
        }
    }

    bool IsWhitelisted(std::string const& ip) const
    {
        for (std::string const& w : _whitelist)
            if (w == ip)
                return true;
        return false;
    }
};

void Addmod_ip_limiterScripts()
{
    new IPLimiter_WorldScript();
}
