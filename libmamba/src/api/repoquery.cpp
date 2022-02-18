// Copyright (c) 2019, QuantStack and Mamba Contributors
//
// Distributed under the terms of the BSD 3-Clause License.
//
// The full license is in the file LICENSE, distributed with this software.

#include "mamba/api/configuration.hpp"
#include "mamba/api/channel_loader.hpp"

#include "mamba/api/repoquery.hpp"

namespace mamba
{
    void repoquery(QueryType type,
                   QueryResultFormat format,
                   bool use_local,
                   const std::string& query)
    {
        std::cout << "FORMAT: " << int(format) << std::endl;
        auto& ctx = Context::instance();
        auto& config = Configuration::instance();
        config.at("use_target_prefix_fallback").set_value(true);
        config.at("target_prefix_checks")
            .set_value(MAMBA_ALLOW_EXISTING_PREFIX | MAMBA_ALLOW_MISSING_PREFIX);
        config.load();

        MPool pool;

        // bool installed = (type == QueryType::kDepends) || (type == QueryType::kWhoneeds);
        MultiPackageCache package_caches(ctx.pkgs_dirs);
        std::vector<MRepo> repos;
        if (use_local)
        {
            auto prefix_data = PrefixData(ctx.target_prefix);
            prefix_data.load();
            repos.push_back(MRepo(pool, prefix_data));
            Console::stream() << "Loaded current active prefix: " << ctx.target_prefix << std::endl;
        }
        else
        {
            repos = load_channels(pool, package_caches, 0);
        }

        Query q(pool);
        if (type == QueryType::kSEARCH)
        {
            if (ctx.json)
            {
                std::cout << q.find(query).groupby("name").json().dump(4);
            }
            else
            {
                std::cout << "\n" << std::endl;
                auto res = q.find(query);
                switch (format)
                {
                    case QueryResultFormat::kJSON:
                        std::cout << res.json().dump(4);
                        break;
                    case QueryResultFormat::kPRETTY:
                        res.pretty(std::cout);
                        break;
                    default:
                        res.groupby("name").table(std::cout);
                }
            }
        }
        else if (type == QueryType::kDEPENDS)
        {
            auto res = q.depends(query, format == QueryResultFormat::kTREE);
            switch (format)
            {
                case QueryResultFormat::kTREE:
                    res.tree(std::cout);
                    break;
                case QueryResultFormat::kJSON:
                    std::cout << res.json().dump(4);
                    break;
                case QueryResultFormat::kTABLE:
                    res.sort("name").table(std::cout);
            }
        }
        else if (type == QueryType::kWHONEEDS)
        {
            auto res = q.whoneeds(query, format == QueryResultFormat::kTREE);
            switch (format)
            {
                case QueryResultFormat::kTREE:
                    res.tree(std::cout);
                    break;
                case QueryResultFormat::kJSON:
                    std::cout << res.json().dump(4);
                    break;
                case QueryResultFormat::kTABLE:
                    res.sort("name").table(
                        std::cout,
                        { "Name", "Version", "Build", concat("Depends:", query), "Channel" });
            }
        }

        config.operation_teardown();
    }
}
