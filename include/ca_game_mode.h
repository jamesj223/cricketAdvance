#ifndef CA_GAME_MODE_H
#define CA_GAME_MODE_H

namespace ca
{
    enum class game_mode
    {
        QUICK_PLAY,     // one batter, bowlers change every over, bat until you're out
        PRACTICE,       // the nets: pick the bowler, endless, wickets just counted
    };
}

#endif
