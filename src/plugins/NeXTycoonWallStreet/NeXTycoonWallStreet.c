/*****************************************************************************
 * NeXTycoon Wall Street & Stock Exchange Plugin
 * Universal Tycoon Native Plugin Architecture
 *
 * Simulates real-time Wall Street high-frequency trading for the theme park,
 * ticker $NXT, quarterly shareholder dividends, macro economic cycles,
 * breaking analyst rating news broadcasts, and venture capital liquidity.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "../../openrct2/scripting/NeXTycoonPluginApi.h"

static const NxApi* g_api = NULL;

typedef struct
{
    double stock_price;
    double prev_price;
    double all_time_high;
    double all_time_low;
    int32_t market_regime; // 0 = Bear, 1 = Neutral, 2 = Bull, 3 = Hyper Growth
    uint32_t last_eval_tick;
    uint32_t last_regime_tick;
    uint32_t last_dividend_tick;
    uint32_t last_log_tick;
    int32_t quarter;
    int64_t total_dividends_paid;
} WallStreetState;

static WallStreetState g_state;

static const char* RegimeToString(int32_t regime)
{
    switch (regime)
    {
        case 0:  return "BEAR MARKET";
        case 1:  return "NEUTRAL";
        case 2:  return "BULL MARKET";
        case 3:  return "HYPER GROWTH";
        default: return "NORMAL";
    }
}

NX_EXPORT int32_t nx_plugin_init(const NxApi* api, NxPluginInfo* out_info)
{
    if (api == NULL || out_info == NULL)
    {
        return -1;
    }

    g_api = api;

    out_info->name = "NeXTycoon Wall Street & Stock Exchange";
    out_info->version = "1.0.0";
    out_info->author = "NeXTycoon High-Frequency Trading Lab";
    out_info->plugin_type = NX_PLUGIN_INTRANSIENT;
    out_info->min_api_version = 124;
    out_info->target_api_version = 125;

    memset(&g_state, 0, sizeof(g_state));
    g_state.stock_price = 100.0;
    g_state.prev_price = 100.0;
    g_state.all_time_high = 100.0;
    g_state.all_time_low = 100.0;
    g_state.market_regime = 2; // Start in Bull Market
    g_state.quarter = 1;

    srand((unsigned int)time(NULL));

    return 0;
}

NX_EXPORT void nx_plugin_start(void)
{
    if (g_api == NULL)
        return;

    g_api->log_info("[WallStreet] NeXTycoon Wall Street & Stock Exchange plugin v1.0.0 initialised!");
    g_api->log_info("[WallStreet] Symbol $NXT listed on NeXTycoon Exchange (NYSE). Live trading active.");

    if (g_api->post_news != NULL)
    {
        g_api->post_news(2, "{GOLD}[WALL STREET] {WHITE}Theme park shares {LIGHTBLUE}$NXT {WHITE}IPO launched! Welcome to Wall Street!");
    }
}

NX_EXPORT void nx_plugin_update(void)
{
    if (g_api == NULL)
        return;

    if (g_api->is_game_paused && g_api->is_game_paused())
        return;

    uint32_t ticks = g_api->get_current_ticks ? g_api->get_current_ticks() : 0;

    // Initialise tick benchmarks on first run
    if (g_state.last_eval_tick == 0)
    {
        g_state.last_eval_tick = ticks;
        g_state.last_regime_tick = ticks;
        g_state.last_dividend_tick = ticks;
        g_state.last_log_tick = ticks;
        return;
    }

    // 1. EVALUATE STOCK PRICE TICK (~every 80 game ticks / ~2 seconds)
    if (ticks - g_state.last_eval_tick >= 80)
    {
        g_state.last_eval_tick = ticks;

        int32_t rating = g_api->get_park_rating ? g_api->get_park_rating() : 500;
        int32_t guests = g_api->get_guest_count ? g_api->get_guest_count() : 100;
        int64_t park_val = g_api->get_park_value ? g_api->get_park_value() : 25000;
        int32_t rides = g_api->get_ride_count ? g_api->get_ride_count() : 4;

        // Economic fundamentals valuation
        double fair_value = 20.0 + (rating * 0.12) + (guests * 0.05) + (rides * 5.0) + (park_val / 50000.0);

        // Macro regime drift
        double regime_drift = 0.0;
        switch (g_state.market_regime)
        {
            case 0: regime_drift = -0.012; break; // Bear
            case 1: regime_drift =  0.001; break; // Neutral
            case 2: regime_drift =  0.015; break; // Bull
            case 3: regime_drift =  0.028; break; // Hyper Growth
        }

        // Brownian motion stochastic noise (-4% to +4%)
        double noise = ((rand() % 2001) - 1000) / 25000.0;

        g_state.prev_price = g_state.stock_price;
        g_state.stock_price = g_state.stock_price * (1.0 + regime_drift + noise);

        // Pull price toward fundamental fair value
        g_state.stock_price = (g_state.stock_price * 0.93) + (fair_value * 0.07);

        if (g_state.stock_price < 5.00)
            g_state.stock_price = 5.00;

        if (g_state.stock_price > g_state.all_time_high)
        {
            g_state.all_time_high = g_state.stock_price;
            // All-Time High Headline!
            if (g_api->post_news != NULL && (rand() % 4 == 0))
            {
                char ath_msg[256];
                snprintf(ath_msg, sizeof(ath_msg), "{YELLOW}[WALL STREET ATH] {WHITE}$NXT hits all-time high of {GREEN}$%.2f{WHITE}! Market cap is soaring!", g_state.stock_price);
                g_api->post_news(2, ath_msg);
            }
        }
    }

    // 2. MACRO ECONOMIC CYCLE SHIFT (~every 2400 ticks / ~60 seconds)
    if (ticks - g_state.last_regime_tick >= 2400)
    {
        g_state.last_regime_tick = ticks;

        int32_t prev_regime = g_state.market_regime;
        int32_t roll = rand() % 100;
        if (roll < 20)
            g_state.market_regime = 0; // 20% Bear
        else if (roll < 50)
            g_state.market_regime = 1; // 30% Neutral
        else if (roll < 85)
            g_state.market_regime = 2; // 35% Bull
        else
            g_state.market_regime = 3; // 15% Hyper Growth

        if (g_state.market_regime != prev_regime && g_api->post_news != NULL)
        {
            char reg_msg[256];
            if (g_state.market_regime == 3)
            {
                snprintf(reg_msg, sizeof(reg_msg), "{GOLD}[WALL STREET] {WHITE}Economic Boom! Wall Street enters {GOLD}HYPER GROWTH{WHITE}! $NXT surges!");
                g_api->post_news(2, reg_msg);
            }
            else if (g_state.market_regime == 2)
            {
                snprintf(reg_msg, sizeof(reg_msg), "{GREEN}[WALL STREET] {WHITE}Market update: {GREEN}BULL MARKET{WHITE} confirmed! Institutional buyers piling in!");
                g_api->post_news(2, reg_msg);
            }
            else if (g_state.market_regime == 0)
            {
                snprintf(reg_msg, sizeof(reg_msg), "{RED}[WALL STREET] {WHITE}Market alert: {RED}BEAR MARKET{WHITE} cycle begins. Keep park rating high to weather the storm!");
                g_api->post_news(3, reg_msg);
            }
        }
    }

    // 3. QUARTERLY SHAREHOLDER DIVIDEND & CASH BONUS (~every 1200 ticks / ~30 seconds)
    if (ticks - g_state.last_dividend_tick >= 1200)
    {
        g_state.last_dividend_tick = ticks;

        int32_t rating = g_api->get_park_rating ? g_api->get_park_rating() : 500;
        int32_t guests = g_api->get_guest_count ? g_api->get_guest_count() : 100;

        if (rating >= 550)
        {
            // Calculate dividend yield based on stock price and guest volume
            int64_t dividend = (int64_t)(g_state.stock_price * 18.0 + guests * 3.0);
            if (dividend < 2500)
                dividend = 2500;
            if (dividend > 35000)
                dividend = 35000;

            g_state.total_dividends_paid += dividend;

            char div_msg[256];
            snprintf(div_msg, sizeof(div_msg), "{GREEN}[WALL STREET] {WHITE}Q%d Earnings Beat! $NXT stock grants {GREEN}+$%lld{WHITE} quarterly shareholder dividend!", g_state.quarter, (long long)dividend);

            if (g_api->grant_park_bonus != NULL)
            {
                g_api->grant_park_bonus(dividend * 10, "Quarterly Shareholder Dividend Yield");
            }
            else if (g_api->set_park_cash != NULL && g_api->get_park_cash != NULL)
            {
                g_api->set_park_cash(g_api->get_park_cash() + dividend * 10);
            }

            if (g_api->post_news != NULL)
            {
                g_api->post_news(1, div_msg);
            }

            if (g_api->log_info != NULL)
            {
                g_api->log_info(div_msg);
            }

            g_state.quarter++;
        }
        else if (rating < 400 && g_api->post_news != NULL)
        {
            char warn_msg[256];
            snprintf(warn_msg, sizeof(warn_msg), "{RED}[WALL STREET DOWNGRADE] {WHITE}Credit agencies downgrade $NXT! Park rating (%d) below institutional threshold!", rating);
            g_api->post_news(3, warn_msg);
            g_state.quarter++;
        }
    }

    // 4. PERIODIC CONSOLE TICKER LOG (~every 400 ticks / ~10 seconds)
    if (ticks - g_state.last_log_tick >= 400)
    {
        g_state.last_log_tick = ticks;

        double pct_change = ((g_state.stock_price - g_state.prev_price) / g_state.prev_price) * 100.0;
        char ticker_buf[256];
        snprintf(ticker_buf, sizeof(ticker_buf),
            "[WallStreet Ticker] $NXT: $%.2f (%+.1f%%) | High: $%.2f | Cycle: %s | Total Dividends: $%lld",
            g_state.stock_price, pct_change, g_state.all_time_high, RegimeToString(g_state.market_regime), (long long)g_state.total_dividends_paid);

        if (g_api->log_info != NULL)
        {
            g_api->log_info(ticker_buf);
        }
    }
}

NX_EXPORT void nx_plugin_shutdown(void)
{
    if (g_api != NULL)
    {
        g_api->log_info("[WallStreet] NeXTycoon Wall Street plugin safely shut down. Final trade recorded.");
    }
}
