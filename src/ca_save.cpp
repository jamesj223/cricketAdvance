#include "ca_save.h"

#include "bn_sram.h"

namespace ca
{

namespace
{
    // What actually goes into SRAM: the tag, then the data. Must be trivially copyable
    // (plain bytes: no pointers, no virtual functions), because it's copied byte by byte.
    struct stored
    {
        char magic[8];
        save_data data;
    };

    constexpr char MAGIC[8] = "CRADV01";

    bool magic_matches(const stored& block)
    {
        for(int i = 0; i < 8; ++i)
        {
            if(block.magic[i] != MAGIC[i])
            {
                return false;
            }
        }

        return true;
    }
}

save_data load_save()
{
    stored block;
    bn::sram::read(block);

    if(! magic_matches(block))
    {
        return save_data();     // blank cart: start fresh
    }

    // Belt and braces: never trust a value that indexes an array.
    if(int(block.data.scheme) < 0 || int(block.data.scheme) >= control_scheme_count)
    {
        block.data.scheme = control_scheme::BASIC;
    }

    return block.data;
}

void store_save(const save_data& data)
{
    stored block;

    for(int i = 0; i < 8; ++i)
    {
        block.magic[i] = MAGIC[i];
    }

    block.data = data;
    bn::sram::write(block);
}

bool record_innings(save_data& data, control_scheme scheme, int runs, int balls)
{
    if(runs <= 0)
    {
        return false;
    }

    best_score& best = data.best[int(scheme)];

    if(runs > best.runs || (runs == best.runs && balls < best.balls))
    {
        best.runs = runs;
        best.balls = balls;
        return true;
    }

    return false;
}

}
